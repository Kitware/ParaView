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
#include "vtkSquirtCompressor.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

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

vtkCxxRevisionMacro(vtkPVDesktopDeliveryClient, "1.8");
vtkStandardNewMacro(vtkPVDesktopDeliveryClient);

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryClient::vtkPVDesktopDeliveryClient()
{
  this->Squirt = 0;
  this->SquirtCompressionLevel = 5;
  this->SquirtBuffer = vtkUnsignedCharArray::New();
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

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(vtkPVDesktopDeliveryClientReceiveImageCallback);
  this->ReceiveImageCallback = cbc;
}

//----------------------------------------------------------------------------
vtkPVDesktopDeliveryClient::~vtkPVDesktopDeliveryClient()
{
  this->SquirtBuffer->Delete();
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
void vtkPVDesktopDeliveryClient::SetController(
                                          vtkMultiProcessController *controller)
{
  vtkDebugMacro("SetController");

  if (controller && (controller->GetNumberOfProcesses() != 2))
    {
    vtkErrorMacro("vtkDesktopDelivery needs controller with 2 processes");
    return;
    }

  this->Superclass::SetController(controller);

  if (this->Controller)
    {
    this->RootProcessId = this->Controller->GetLocalProcessId();
    this->ServerProcessId = 1 - this->RootProcessId;
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
  if ((this->GUISize[0] == 0) || (this->GUISize[1] == 0))
    {
    winGeoInfo.GUISize[0] = this->RenderWindow->GetActualSize()[0];
    winGeoInfo.GUISize[1] = this->RenderWindow->GetActualSize()[1];
    }
  else
    {
    winGeoInfo.GUISize[0] = this->GUISize[0];
    winGeoInfo.GUISize[1] = this->GUISize[1];
    }
  // Flip Y possition to lower left to make things easier for server.
  winGeoInfo.Position[0] = this->WindowPosition[0];
  winGeoInfo.Position[1]
    = (  winGeoInfo.GUISize[1]
       - this->WindowPosition[1] - this->RenderWindow->GetActualSize()[1] );
  winGeoInfo.Id = this->Id;
  winGeoInfo.AnnotationLayer = this->AnnotationLayer;
  winGeoInfo.Save(stream);

  vtkPVDesktopDeliveryServer::SquirtOptions squirtOptions;
  squirtOptions.Enabled = this->Squirt;
  squirtOptions.CompressLevel = this->SquirtCompressionLevel;
  squirtOptions.Save(stream);
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
}

//-----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::ReceiveImageFromServer()
{
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

    if (ip.SquirtCompressed)
      {
      this->SquirtBuffer->SetNumberOfComponents(ip.NumberOfComponents);
      this->SquirtBuffer->SetNumberOfTuples(  ip.BufferSize
                                            / ip.NumberOfComponents);
      this->Controller->Receive(this->SquirtBuffer->GetPointer(0),
                                ip.BufferSize, this->ServerProcessId,
                                vtkPVDesktopDeliveryServer::IMAGE_TAG);
      this->SquirtDecompress(this->SquirtBuffer, this->ReducedImage);
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
  if (this->Squirt)
    {
    if (this->ImageReductionFactor == 1)
      {
      this->SetSquirtCompressionLevel(0);
      }
    else
      {
      this->SetSquirtCompressionLevel(5);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::SquirtDecompress(vtkUnsignedCharArray *in,
                                                vtkUnsignedCharArray *out)
{
  vtkSquirtCompressor *compressor = vtkSquirtCompressor::New();
  compressor->SetInput(in);
  compressor->SetOutput(out);
  compressor->Decompress();
  compressor->Delete();
}

//----------------------------------------------------------------------------
void vtkPVDesktopDeliveryClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ServerProcessId: " << this->ServerProcessId << endl;

  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "On" : "Off") << endl;
  os << indent << "Squirt: "
     << (this->Squirt? "On" : "Off") << endl;

  os << indent << "RemoteImageProcessingTime: "
     << this->RemoteImageProcessingTime << endl;
  os << indent << "TransferTime: " << this->TransferTime << endl;
  os << indent << "SquirtCompressionLevel: " << this->SquirtCompressionLevel << endl;
  os << indent << "Id: " << this->Id << endl;
  os << indent << "AnnotationLayer: " << this->AnnotationLayer << endl;
  os << indent << "WindowPosition: "
     << this->WindowPosition[0] << ", " << this->WindowPosition[1] << endl;
  os << indent << "GUISize: "
     << this->GUISize[0] << ", " << this->GUISize[1] << endl;
}
