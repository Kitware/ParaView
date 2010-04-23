/*=========================================================================

  Program:   ParaView
  Module:    vtkDesktopDeliveryClient.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDesktopDeliveryClient.h"
#include "vtkDesktopDeliveryServer.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCubeSource.h"
#include "vtkDoubleArray.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRendererCollection.h"
#include "vtkRenderWindow.h"
#include "vtkSquirtCompressor.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

//-----------------------------------------------------------------------------

static void vtkDesktopDeliveryClientReceiveImageCallback(vtkObject *,
                                                         unsigned long,
                                                         void *clientdata,
                                                         void *)
{
  vtkDesktopDeliveryClient *self
    = reinterpret_cast<vtkDesktopDeliveryClient *>(clientdata);
  self->ReceiveImageFromServer();
}

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkDesktopDeliveryClient);

//----------------------------------------------------------------------------
vtkDesktopDeliveryClient::vtkDesktopDeliveryClient()
{
  this->ReplaceActors = 1;
  this->Squirt = 0;
  this->SquirtCompressionLevel = 5;
  this->SquirtBuffer = vtkUnsignedCharArray::New();
  this->UseCompositing = 0;
  this->RemoteDisplay = 1;
  this->ReceivedImageFromServer = 1;

  vtkCallbackCommand *cbc = vtkCallbackCommand::New();
  cbc->SetClientData(this);
  cbc->SetCallback(vtkDesktopDeliveryClientReceiveImageCallback);
  this->ReceiveImageCallback = cbc;
}

//----------------------------------------------------------------------------
vtkDesktopDeliveryClient::~vtkDesktopDeliveryClient()
{
  this->SquirtBuffer->Delete();
  this->ReceiveImageCallback->Delete();
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::SetUseCompositing(int v)
{
  this->Superclass::SetUseCompositing(v);

  if (this->RemoteDisplay)
    {
    this->SetParallelRendering(v);
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::SetController(vtkMultiProcessController *controller)
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
void vtkDesktopDeliveryClient::SetRenderWindow(vtkRenderWindow *renWin)
{
  //Make sure the renWin has at least one renderer
  if (renWin)
    {
    vtkRendererCollection *rens = renWin->GetRenderers();
    if (rens->GetNumberOfItems() < 1)
      {
      vtkRenderer *ren = vtkRenderer::New();
      renWin->AddRenderer(ren);
      ren->Delete();
      }
    }

  this->Superclass::SetRenderWindow(renWin);
}

//----------------------------------------------------------------------------
// Called only on the client.
float vtkDesktopDeliveryClient::GetZBufferValue(int x, int y)
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

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::CollectWindowInformation(vtkMultiProcessStream& stream)
{
  this->Superclass::CollectWindowInformation(stream);

  vtkDesktopDeliveryServer::SquirtOptions squirt_options;
  squirt_options.Enabled = this->Squirt;
  squirt_options.CompressLevel = this->SquirtCompressionLevel;
  squirt_options.Save(stream);
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::PreRenderProcessing()
{
  // Get remote display flag
  this->Controller->Receive(&this->RemoteDisplay, 1, this->ServerProcessId,
                            vtkDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

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

  if (!this->SyncRenderWindowRenderers)
    {
    // Establish a callback so that the image from the server is retrieved
    // before we draw renderers that we are not synced with.  This will fail if
    // a non-synced renderer is on a layer equal or less than a synced renderer.
    vtkRendererCollection *allren = this->RenderWindow->GetRenderers();
    vtkCollectionSimpleIterator cookie;
    vtkRenderer *ren;
    for (allren->InitTraversal(cookie);
         (ren = allren->GetNextRenderer(cookie)) != NULL; )
      {
      if (!this->Renderers->IsItemPresent(ren))
        {
        ren->AddObserver(vtkCommand::StartEvent, this->ReceiveImageCallback);
        }
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
void vtkDesktopDeliveryClient::PostRenderProcessing()
{
  this->ReceiveImageFromServer();

  this->Timer->StopTimer();
  this->RenderTime += this->Timer->GetElapsedTime();

  if (!this->SyncRenderWindowRenderers)
    {
    vtkRendererCollection *allren = this->RenderWindow->GetRenderers();
    vtkCollectionSimpleIterator cookie;
    vtkRenderer *ren;
    for (allren->InitTraversal(cookie);
         (ren = allren->GetNextRenderer(cookie)) != NULL; )
      {
      ren->RemoveObservers(vtkCommand::StartEvent, this->ReceiveImageCallback);
      }
    }

  // Swap buffers here.
  if (this->UseBackBuffer)
    {
    this->RenderWindow->SwapBuffersOn();
    }
  this->RenderWindow->Frame();
}

//-----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::ReceiveImageFromServer()
{
  if (this->ReceivedImageFromServer) return;

  this->ReceivedImageFromServer = 1;

  vtkDesktopDeliveryServer::ImageParams ip;
  int comm_success =
    this->Controller->Receive((int *)(&ip),
                              vtkDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                              this->ServerProcessId,
                              vtkDesktopDeliveryServer::IMAGE_PARAMS_TAG);

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
      this->FullImageUpToDate = true;
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
                                vtkDesktopDeliveryServer::IMAGE_TAG);
      this->SquirtDecompress(this->SquirtBuffer, this->ReducedImage);
      }
    else
      {
      this->Controller->Receive(this->ReducedImage->GetPointer(0),
                                ip.BufferSize, this->ServerProcessId,
                                vtkDesktopDeliveryServer::IMAGE_TAG);
      }
    this->ReducedImageUpToDate = true;
    this->RenderWindowImageUpToDate = false;

    this->Timer->StopTimer();
    this->TransferTime = this->Timer->GetElapsedTime();
    }
  else
    {
    // No remote display means no transfer time.
    this->TransferTime = 0.0;

    // Leave the image in the window alone.
    this->RenderWindowImageUpToDate = true;
    }

  vtkDesktopDeliveryServer::TimingMetrics tm;
  this->Controller->Receive((double *)(&tm),
    vtkDesktopDeliveryServer::TIMING_METRICS_SIZE,
    this->ServerProcessId,
    vtkDesktopDeliveryServer::TIMING_METRICS_TAG);
  this->RemoteImageProcessingTime = tm.ImageProcessingTime;

  this->WriteFullImage();

  this->Timer->StartTimer();
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::ComputeVisiblePropBounds(vtkRenderer *ren,
                                                        double bounds[6])
{
  this->Superclass::ComputeVisiblePropBounds(ren, bounds);

  if (this->ReplaceActors)
    {
    vtkDebugMacro("Replacing actors.");

    ren->GetActors()->RemoveAllItems();

    vtkCubeSource* source = vtkCubeSource::New();
    source->SetBounds(bounds);

    vtkPolyDataMapper* mapper = vtkPolyDataMapper::New();
    mapper->SetInput(source->GetOutput());

    vtkActor* actor = vtkActor::New();
    actor->SetMapper(mapper);

    ren->AddActor(actor);
    source->Delete();
    mapper->Delete();
    actor->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::SetImageReductionFactorForUpdateRate(double desiredUpdateRate)
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
void vtkDesktopDeliveryClient::SquirtDecompress(vtkUnsignedCharArray *in,
                                                vtkUnsignedCharArray *out)
{
  vtkSquirtCompressor *compressor = vtkSquirtCompressor::New();
  compressor->SetInput(in);
  compressor->SetOutput(out);
  compressor->Decompress();
  compressor->Delete();
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ServerProcessId: " << this->ServerProcessId << endl;

  os << indent << "ReplaceActors: "
     << (this->ReplaceActors ? "On" : "Off") << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "On" : "Off") << endl;
  os << indent << "Squirt: "
     << (this->Squirt? "On" : "Off") << endl;

  os << indent << "RemoteImageProcessingTime: "
     << this->RemoteImageProcessingTime << endl;
  os << indent << "TransferTime: " << this->TransferTime << endl;
  os << indent << "SquirtCompressionLevel: " << this->SquirtCompressionLevel << endl;
}
