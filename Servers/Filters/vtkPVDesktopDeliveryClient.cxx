/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDesktopDeliveryClient.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVDesktopDeliveryClient.h"
#include "vtkPVDesktopDeliveryServer.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDoubleArray.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"

#include "vtkImageCompressor.h"
#include "vtkSquirtCompressor.h"
#include "vtkZlibImageCompressor.h"

#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#if defined vtkPVDesktopDeliveryTIME
  #include <vtksys/ios/iostream>
  #include <vtksys/ios/sstream>
  #include <sys/time.h>
  using namespace std;
#endif

//-----------------------------------------------------------------------------

static void vtkPVDesktopDeliveryClientReceiveImageCallback(vtkObject *,
                                                           unsigned long,
                                                           void *clientdata,
                                                           void *)
{
  vtkPVDesktopDeliveryClient *self
    = reinterpret_cast<vtkPVDesktopDeliveryClient *>(clientdata);
  self->ReceiveImageFromServer();
}

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPVDesktopDeliveryClient);

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryClient::vtkPVDesktopDeliveryClient()
{
  #if defined vtkPVDesktopDeliveryTIME
  // Mark creation time to reduce digits when reporting time stamps.
  timeval wtime;
  gettimeofday(&wtime,0);
  this->CreationTime=wtime.tv_sec+wtime.tv_usec/1.0E6;
  #endif

  this->UseCompositing = 0;
  this->RemoteDisplay = 1;
  this->ReceivedImageFromServer = 1;
  this->Id = 0;
  this->ServerProcessId = 0;
  this->AnnotationLayer = 1;
  this->WindowPosition[0] = this->WindowPosition[1] = 0;
  this->GUISize[0] = this->GUISize[1] = 0;
  this->RemoteImageProcessingTime = 0.0;
  this->TransferTime = 0.0;

  this->GUISizeCompact[0] = this->GUISizeCompact[1] = 0;
  this->ViewSizeCompact[0] = this->ViewSizeCompact[1] = 0;
  this->ViewPositionCompact[0] = this->ViewPositionCompact[1] = 0;

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(vtkPVDesktopDeliveryClientReceiveImageCallback);
  this->ReceiveImageCallback = cbc;

  // I am the root, the other process is the satellite.
  this->RootProcessId = 0;
  this->ServerProcessId = 1;
}

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryClient::~vtkPVDesktopDeliveryClient()
{
  this->ReceiveImageCallback->Delete();
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::SetUseCompositing(int v)
{
  this->Superclass::SetUseCompositing(v);

  if (this->RemoteDisplay)
    {
    this->SetParallelRendering(v);
    }
}

//----------------------------------------------------------------------------
// Called only on the client.
float vtkPVDesktopDeliveryClient::GetZBufferValue(int x, int y)
{
  float z;

  if (this->UseCompositing == 0)
    {
    // This could cause a problem between setting this ivar and rendering.
    // We could always composite, and always consider client z.
    float *pz;
    pz = this->RenderWindow->GetZbufferData(x, y, x, y);
    z = *pz;
    delete [] pz;
    return z;
    }
  
  // TODO:
  // This first int is to check for byte swapping.
//  int pArg[3];
//  pArg[0] = 1;
//  pArg[1] = x;
//  pArg[2] = y;
//  this->ClientController->TriggerRMI(1, (void*)pArg, sizeof(int)*3, 
//                                vtkClientCompositeManager::GATHER_Z_RMI_TAG);
//  this->ClientController->Receive(&z, 1, 1, vtkClientCompositeManager::CLIENT_Z_TAG);
  z = 1.0;
  return z;
}



//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::ComputeVisiblePropBounds(vtkRenderer *ren,
                                                          double bounds[6])
{
  if (this->ParallelRendering && this->Controller)
    {
    this->Controller->TriggerRMI(this->ServerProcessId, &this->Id, sizeof(int),
                                 vtkPVDesktopDeliveryServer::WINDOW_ID_RMI_TAG);
    }

  this->Superclass::ComputeVisiblePropBounds(ren, bounds);
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::CollectWindowInformation(vtkMultiProcessStream& stream)
{
  this->Superclass::CollectWindowInformation(stream);

  vtkPVDesktopDeliveryServer::WindowGeometry winGeoInfo;

  // If GUISizeCompact has been set to a non-zero value
  // then we are using multi display i.e. tiledisplay, so we'll use the
  // compact values to eliminate gaps on the server tile display.
  if (this->GUISizeCompact[0] != 0 && this->GUISizeCompact[1] != 0)
    {
    winGeoInfo.GUISize[0] = this->GUISizeCompact[0];
    winGeoInfo.GUISize[1] = this->GUISizeCompact[1];
    winGeoInfo.ViewSize[0] = this->ViewSizeCompact[0];
    winGeoInfo.ViewSize[1] = this->ViewSizeCompact[1];
    winGeoInfo.Position[0] = this->ViewPositionCompact[0];
    winGeoInfo.Position[1] = this->ViewPositionCompact[1];

    // Flip Y possition to lower left to make things easier for server.
    winGeoInfo.Position[1] =
      winGeoInfo.GUISize[1] - winGeoInfo.Position[1] - winGeoInfo.ViewSize[1];
    }
  else  // Use regular values
    {
    winGeoInfo.GUISize[0] = this->GUISize[0];
    winGeoInfo.GUISize[1] = this->GUISize[1];
    if ((this->GUISize[0] == 0) || (this->GUISize[1] == 0))
      {
      winGeoInfo.GUISize[0] = this->RenderWindow->GetActualSize()[0];
      winGeoInfo.GUISize[1] = this->RenderWindow->GetActualSize()[1];
      }
    // This ensures that the server side uses this->FullImageSize as the
    // ClientWindowSize (look at
    // vtkPVDesktopDeliveryServer::ProcessWindowInformation()).
    winGeoInfo.ViewSize[0] = 0;
    winGeoInfo.ViewSize[1] = 0;

    // Flip Y possition to lower left to make things easier for server.
    winGeoInfo.Position[0] = this->WindowPosition[0];
    winGeoInfo.Position[1]
      = (  winGeoInfo.GUISize[1]
        - this->WindowPosition[1] - this->RenderWindow->GetActualSize()[1] );
    }

  winGeoInfo.Id = this->Id;
  winGeoInfo.AnnotationLayer = this->AnnotationLayer;
  winGeoInfo.Save(stream);
}


//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::CollectRendererInformation(vtkRenderer *renderer,
  vtkMultiProcessStream& stream)
{
  this->Superclass::CollectRendererInformation(renderer, stream);

  // The server needs to shift around the viewport and then resize it.  To do
  // this, it needs the original viewport.  Undo the "helpful" resizing of the
  // superclass.
  double viewport[4];
  renderer->GetViewport(viewport);
  viewport[0] *= this->ImageReductionFactor;
  viewport[1] *= this->ImageReductionFactor;
  viewport[2] *= this->ImageReductionFactor;
  viewport[3] *= this->ImageReductionFactor;
  stream << viewport[0] << viewport[1] << viewport[2] << viewport[3];
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::PreRenderProcessing()
{
  // Get remote display flag
  this->Controller->Receive(&this->RemoteDisplay, 1, this->ServerProcessId,
                            vtkPVDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if (this->ImageReductionFactor > 1)
    {
    // Since we're not really doing parallel rendering, restore the renderer
    // viewports.
    vtkRendererCollection *rens = this->GetRenderers();
    vtkRenderer *ren;
    int i;
    for (rens->InitTraversal(), i = 0; (ren = rens->GetNextItem()); i++)
      {
      ren->SetViewport(this->Viewports->GetTuple(i));
      }
    }

  this->ReceivedImageFromServer = 0;

  // Establish a callback so that the image from the server is retrieved
  // before we draw renderers that are annotation.
  vtkRendererCollection *allren = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *ren;
  for (allren->InitTraversal(cookie);
       (ren = allren->GetNextRenderer(cookie)) != NULL; )
    {
    if (ren->GetLayer() >= this->AnnotationLayer)
      {
      ren->AddObserver(vtkCommand::StartEvent, this->ReceiveImageCallback);
      }
    }

  // Turn swap buffers off before the render so the end render method has a
  // chance to add to the back buffer.
  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOff();
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::PostRenderProcessing()
{
  vtkTimerLog::MarkStartEvent("Receiving");

  this->ReceiveImageFromServer();

  this->Timer->StopTimer();
  this->RenderTime += this->Timer->GetElapsedTime();

  vtkRendererCollection *allren = this->RenderWindow->GetRenderers();
  vtkCollectionSimpleIterator cookie;
  vtkRenderer *ren;
  for (allren->InitTraversal(cookie);
       (ren = allren->GetNextRenderer(cookie)) != NULL; )
    {
    ren->RemoveObservers(vtkCommand::StartEvent, this->ReceiveImageCallback);
    }

  // Swap buffers here.
  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOn();
    }
  this->RenderWindow->Frame();

  vtkTimerLog::MarkEndEvent("Receiving");
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::ReceiveImageFromServer()
{
  #if defined vtkPVDesktopDeliveryTIME
  // Generate a report that can be used for benchmarking
  // compressor.
  vtkstd::ostringstream os;
  const int colw=25;
  #endif

  if (this->ReceivedImageFromServer) return;

  this->ReceivedImageFromServer = 1;

  vtkPVDesktopDeliveryServer::ImageParams ip;
  int comm_success =
    this->Controller->Receive(reinterpret_cast<int *>(&ip),
                              vtkPVDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                              this->ServerProcessId,
                              vtkPVDesktopDeliveryServer::IMAGE_PARAMS_TAG);

  // Adjust render time for actual render on server.
  this->Timer->StopTimer();
  this->RenderTime += this->Timer->GetElapsedTime();

  if (comm_success && ip.RemoteDisplay)
    {
    // Receive image.
    this->Timer->StartTimer();
    this->ReducedImageSize[0] = ip.ImageSize[0];
    this->ReducedImageSize[1] = ip.ImageSize[1];
    this->ReducedImage->SetNumberOfComponents(ip.NumberOfComponents);
    if ( this->FullImageSize[0] == this->ReducedImageSize[0]
      && this->FullImageSize[1] == this->ReducedImageSize[1] )
      {
      this->FullImage->SetNumberOfComponents(ip.NumberOfComponents);
      this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
                                         * this->FullImageSize[1]);
      this->FullImageUpToDate = 1;
      this->ReducedImage->SetArray(this->FullImage->GetPointer(0),
                                   this->FullImage->GetSize(), 1);
      }
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
                                          * this->ReducedImageSize[1]);

    if (this->CompressionEnabled)
      {
      // Allocate buffer.
      this->CompressorBuffer->SetNumberOfComponents(1);
      this->CompressorBuffer->SetNumberOfTuples(ip.BufferSize);

      // Pull compressed image.
      this->Controller->Receive(
          this->CompressorBuffer->GetPointer(0),
          ip.BufferSize,
          this->ServerProcessId,
          vtkPVDesktopDeliveryServer::IMAGE_TAG);

      // Decompress the image.
      this->Compressor->SetLossLessMode(this->LossLessCompression);
      this->Compressor->SetInput(this->CompressorBuffer);
      this->Compressor->SetOutput(this->ReducedImage);
      this->Compressor->Decompress();
      this->Compressor->SetInput(0);
      this->Compressor->SetOutput(0);

      #if defined vtkPVDesktopDeliveryTIME
      // Add compression ratio to the report.
      size_t compImSize=this->CompressorBuffer->GetNumberOfTuples();
      size_t redImSize
      =this->ReducedImage->GetNumberOfTuples()*this->ReducedImage->GetNumberOfComponents();
      size_t fullImSize
      =this->FullImage->GetNumberOfTuples()*this->FullImage->GetNumberOfComponents();
      double cRat=(double)redImSize/(double)compImSize;
      double effCRat=(double)fullImSize/(double)compImSize;
      char comprId=this->Compressor->GetClassName()[3];
      os << setw(3) << comprId
         << setw(3) << this->LossLessCompression
         << setw(colw) << fullImSize
         << setw(colw) << redImSize
         << setw(colw) << compImSize
         << setw(colw) << cRat
         << setw(colw) << effCRat;
      #endif
      }
    else
      {
      this->Controller->Receive(this->ReducedImage->GetPointer(0),
                                ip.BufferSize, this->ServerProcessId,
                                vtkPVDesktopDeliveryServer::IMAGE_TAG);
      }
    this->ReducedImageUpToDate = 1;
    this->RenderWindowImageUpToDate = 0;

    this->Timer->StopTimer();
    this->TransferTime = this->Timer->GetElapsedTime();
    }
  else
    {
    // No remote display means no transfer time.
    this->TransferTime = 0.0;

    // Leave the image in the window alone.
    this->RenderWindowImageUpToDate = 1;
    }

  #if defined vtkPVDesktopDeliveryTIME
  // Add a time stamp to the report and send the report to terminal.
  timeval wtime;
  gettimeofday(&wtime,0);
  double timestamp=wtime.tv_sec+wtime.tv_usec/1.0E6-this->CreationTime;
  os << setw(colw) << scientific << setprecision(15) << timestamp;
  cerr << os.str() << endl;
  #endif

  vtkPVDesktopDeliveryServer::TimingMetrics tm;
  this->Controller->Receive(reinterpret_cast<double *>(&tm),
                            vtkPVDesktopDeliveryServer::TIMING_METRICS_SIZE,
                            this->ServerProcessId,
                            vtkPVDesktopDeliveryServer::TIMING_METRICS_TAG);
  this->RemoteImageProcessingTime = tm.ImageProcessingTime;

  this->WriteFullImage();

  this->Timer->StartTimer();
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::SetImageReductionFactorForUpdateRate(double desiredUpdateRate)
{
  this->Superclass::SetImageReductionFactorForUpdateRate(desiredUpdateRate);
  vtkErrorMacro("This method is defunct and should not be called.");
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ServerProcessId: " << this->ServerProcessId << endl;

  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "On" : "Off") << endl;
  os << indent << "RemoteImageProcessingTime: "
     << this->RemoteImageProcessingTime << endl;
  os << indent << "TransferTime: " << this->TransferTime << endl;
  os << indent << "Id: " << this->Id << endl;
  os << indent << "AnnotationLayer: " << this->AnnotationLayer << endl;
  os << indent << "WindowPosition: "
     << this->WindowPosition[0] << ", " << this->WindowPosition[1] << endl;
  os << indent << "GUISize: "
     << this->GUISize[0] << ", " << this->GUISize[1] << endl;
}
