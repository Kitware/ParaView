/*=========================================================================

  Program:   ParaView
  Module:    vtkDesktopDeliveryClient2.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDesktopDeliveryClient2.h"
#include "vtkDesktopDeliveryServer.h"

#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkCallbackCommand.h>
#include <vtkCubeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkRendererCollection.h>
#include <vtkCamera.h>
#include <vtkLight.h>
#include <vtkTimerLog.h>
#include <vtkLightCollection.h>
#include <vtkDoubleArray.h>
#include <vtkUnsignedCharArray.h>
#include "vtkMultiProcessController.h"
#include "vtkDoubleArray.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkCamera.h"


//#include <vtkRef.h>

vtkCxxRevisionMacro(vtkDesktopDeliveryClient2, "1.2");
vtkStandardNewMacro(vtkDesktopDeliveryClient2);

vtkDesktopDeliveryClient2::vtkDesktopDeliveryClient2()
{
  this->ReplaceActors = 1;
  this->Squirt = 0;
  this->SquirtCompressionLevel = 5;
  this->SquirtBuffer = vtkUnsignedCharArray::New();
  this->UseCompositing = 0;

  this->CompositeData = vtkImageData::New();
  this->ImageActor = vtkImageActor::New();
  this->SavedCamera = vtkCamera::New();
}

vtkDesktopDeliveryClient2::~vtkDesktopDeliveryClient2()
{
  this->SquirtBuffer->Delete();

  this->ImageActor->Delete();
  this->ImageActor = 0;
  this->SavedCamera->Delete();
  this->SavedCamera = 0;
  this->CompositeData->Delete();
  this->CompositeData = 0;
}

void
vtkDesktopDeliveryClient2::SetController(vtkMultiProcessController *controller)
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

void vtkDesktopDeliveryClient2::SetRenderWindow(vtkRenderWindow *renWin)
{
  //Make sure the renWin has at least one renderer
  if (renWin)
    {
    vtkRendererCollection *rens = renWin->GetRenderers();
    vtkRenderer *ren;
    if (rens->GetNumberOfItems() < 1)
      {
      ren = vtkRenderer::New();
      renWin->AddRenderer(ren);
      ren->Delete();
      }

    // Add the image actor for display remote images.
    rens->InitTraversal();
    ren = rens->GetNextItem();
    ren->AddActor(this->ImageActor);
    }

  this->Superclass::SetRenderWindow(renWin);
}

void vtkDesktopDeliveryClient2::SendWindowInformation()
{
  vtkDesktopDeliveryServer::SquirtOptions squirt_options;
  squirt_options.Enabled = this->Squirt;
  squirt_options.CompressLevel = this->SquirtCompressionLevel;

  this->Controller->Send((int *)(&squirt_options),
             vtkDesktopDeliveryServer::SQUIRT_OPTIONS_SIZE,
             this->ServerProcessId,
             vtkDesktopDeliveryServer::SQUIRT_OPTIONS_TAG);
}

void vtkDesktopDeliveryClient2::PreRenderProcessing()
{
  // Get remote display flag
  this->Controller->Receive(&this->RemoteDisplay, 1, this->ServerProcessId,
                vtkDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if ( ! this->RemoteDisplay)
    {
    if (this->ImageReductionFactor > 1)
      {
      // Since we're not replacing the image, restore the renderer viewports.
      vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
      vtkRenderer *ren;
      int i;
      for (rens->InitTraversal(), i = 0; (ren = rens->GetNextItem()); i++)
        {
        // TODO: Revert back once ren->SetViewport() takes double.
        //double *vp = this->Viewports->GetTuple(i);
        //ren->SetViewport(vp[0], vp[1], vp[2], vp[3]);
        //ren->SetViewport(0.0, 0.0, 1.0, 1.0);
        }
      }
    }

  
  // Stuff moved from post render processing.

  // Adjust render time for actual render on server.
  this->Timer->StartTimer();
  this->Controller->Barrier();
  this->Timer->StopTimer();
  this->RenderTime += this->Timer->GetElapsedTime();

  vtkDesktopDeliveryServer::ImageParams ip;
  this->Controller->Receive((int *)(&ip),
    vtkDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
    this->ServerProcessId,
    vtkDesktopDeliveryServer::IMAGE_PARAMS_TAG);

  if (ip.RemoteDisplay)
    {
    // Receive image.
    this->Timer->StartTimer();
    this->ReducedImage->SetNumberOfComponents(ip.NumberOfComponents);
    if (this->ImageReductionFactor == 1)
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
      this->SquirtBuffer->SetNumberOfTuples(ip.Size/ip.NumberOfComponents);
      this->Controller->Receive(this->SquirtBuffer->GetPointer(0), ip.Size,
        this->ServerProcessId,
        vtkDesktopDeliveryServer::IMAGE_TAG);
      this->SquirtDecompress(this->SquirtBuffer, this->ReducedImage);
      }
    else
      {
      this->Controller->Receive(this->ReducedImage->GetPointer(0), ip.Size,
        this->ServerProcessId,
        vtkDesktopDeliveryServer::IMAGE_TAG);
      }
    this->ReducedImageUpToDate = true;

    this->Timer->StopTimer();
    this->TransferTime = this->Timer->GetElapsedTime();

    if (this->WriteBackImages)
      {

      // This was the old set pixel data way.  It was slower than image actor.
      //if (this->MagnifyImages)
      //  {
      //  this->MagnifyReducedImage();
      //  this->SetRenderWindowPixelData(this->FullImage, this->FullImageSize);
      //  }
      //else
      //  {
      //  this->ReadReducedImage();
      //  this->SetRenderWindowPixelData(this->ReducedImage, this->ReducedImageSize);
      //  }

      this->WriteFullImage();


      this->RenderWindowImageUpToDate = true;
      }
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
}

#include "vtkImageData.h"
#include "vtkDataSetWriter.h"
#include "vtkPointData.h"

void vtkDesktopDeliveryClient2::PostRenderProcessing()
{
}

void vtkDesktopDeliveryClient2::ComputeVisiblePropBounds(vtkRenderer *ren,
                            double bounds[6])
{
  this->Superclass::ComputeVisiblePropBounds(ren, bounds);

  if (this->ReplaceActors)
    {
    vtkDebugMacro("Replacing actors.");

    ren->GetActors()->RemoveAllItems();

    // Convert doubles to float for vtkCubeSource
    // Does anyone know why vtkCubeSource doesn't
    // take doubles for SetBounds?
    double fbounds[6];
    for(int i=0;i<6;++i) fbounds[i]=bounds[i];

    vtkCubeSource* source = vtkCubeSource::New();
    source->SetBounds(fbounds);

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

void vtkDesktopDeliveryClient2
    ::SetImageReductionFactorForUpdateRate(double DesiredUpdateRate)
{
  this->Superclass::SetImageReductionFactorForUpdateRate(DesiredUpdateRate);
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

void vtkDesktopDeliveryClient2::SquirtDecompress(vtkUnsignedCharArray *in,
                        vtkUnsignedCharArray *out)
{
  int count=0;
  int index=0;
  unsigned int current_color;
  unsigned int *_rawColorBuffer;
  unsigned int *_rawCompressedBuffer;

  // Get compressed buffer size
  int CompSize = in->GetNumberOfTuples();

  // Access raw arrays directly
  _rawColorBuffer = (unsigned int *)out->GetPointer(0);
  _rawCompressedBuffer = (unsigned int *)in->GetPointer(0);

  // Go through compress buffer and extract RLE format into color buffer
  for(int i=0; i<CompSize; i++)
    {
    // Get color and count
    current_color = _rawCompressedBuffer[i];

    // Get run length count;
    count = ((unsigned char *)(&current_color))[3];

    // Fixed Alpha
    ((unsigned char *)(&current_color))[3] = 0xFF;

    // Set color
    _rawColorBuffer[index++] = current_color;

    // Blast color into color buffer
    for(int j=0; j < count; j++)
      {
      _rawColorBuffer[index++] = current_color;
      }
    }

  // Save out compression stats.
  vtkTimerLog::FormatAndMarkEvent("Squirt ratio: %f",
                  (float)CompSize/(float)index);
}

void vtkDesktopDeliveryClient2::WriteFullImage()
{
  if (this->RenderWindowImageUpToDate || !this->WriteBackImages)
    {
    return;
    }

  this->CompositeData->Initialize();
  this->CompositeData->GetPointData()->SetScalars(this->ReducedImage);
  this->CompositeData->SetScalarType(VTK_UNSIGNED_CHAR);
  this->CompositeData->SetNumberOfScalarComponents(
    this->ReducedImage->GetNumberOfComponents());

  this->CompositeData->SetDimensions(this->ReducedImageSize[0],
                                     this->ReducedImageSize[1], 1);

  this->ImageActor->VisibilityOn();
  this->ImageActor->SetInput(this->CompositeData);
  this->ImageActor->SetDisplayExtent(0, this->ReducedImageSize[0]-1,
                                     0, this->ReducedImageSize[1]-1, 0, 0);

  // int fixme
  // I would like to change SetRenderWindow to set renderer.  I believe the
  // only time that we use the render window now would be to synchronize
  // the swap buffers for tiled displays.
  // We also need to set the size of the render window, but this
  // could be done using the renderer.
  vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
  rens->InitTraversal();
  vtkRenderer* ren = rens->GetNextItem();
  vtkCamera *cam = ren->GetActiveCamera();
  // Why doesn't camera have a Copy method?
  this->SavedCamera->SetPosition(cam->GetPosition());
  this->SavedCamera->SetFocalPoint(cam->GetFocalPoint());
  this->SavedCamera->SetViewUp(cam->GetViewUp());
  this->SavedCamera->SetParallelProjection(cam->GetParallelProjection());
  this->SavedCamera->SetParallelScale(cam->GetParallelScale());
  this->SavedCamera->SetClippingRange(cam->GetClippingRange());
  cam->ParallelProjectionOn();
  cam->SetParallelScale(
    (this->ReducedImageSize[1]-1.0)*0.5);
  cam->SetPosition((this->ReducedImageSize[0]-1.0)*0.5,
                   (this->ReducedImageSize[1]-1.0)*0.5, 10.0);
  cam->SetFocalPoint((this->ReducedImageSize[0]-1.0)*0.5,
                     (this->ReducedImageSize[1]-1.0)*0.5, 0.0);
  cam->SetViewUp(0.0, 1.0, 0.0);
  cam->SetClippingRange(9.0, 11.0);
}



//-------------------------------------------------------------------------
// I tried to implement PreRenderProcessing instead of StartRender,
// but I got stuck trying to overide the Viewport.
// This method could be simplified alot because many of the superclass
// features are not needed here.
void vtkDesktopDeliveryClient2::StartRender()
{
  vtkParallelRenderManager::RenderWindowInfoInt winInfoInt;
  vtkParallelRenderManager::RenderWindowInfoDouble winInfoDouble;
  vtkParallelRenderManager::RendererInfoInt renInfoInt;
  vtkParallelRenderManager::RendererInfoDouble renInfoDouble;
  vtkParallelRenderManager::LightInfoDouble lightInfoDouble;

  vtkDebugMacro("StartRender");

  if ((this->Controller == NULL) || (this->Lock))
    {
    return;
    }
  this->Lock = 1;

  this->FullImageUpToDate = 0;
  this->ReducedImageUpToDate = 0;
  this->RenderWindowImageUpToDate = 0;

  if (this->FullImage->GetPointer(0) == this->ReducedImage->GetPointer(0))
    {
    // "Un-share" pointer for full/reduced images in case we need separate
    // arrays this run.
    this->ReducedImage->Initialize();
    }

  if (!this->ParallelRendering)
    {
    this->Lock = 0;
    return;
    }

  this->InvokeEvent(vtkCommand::StartEvent, NULL);

  // Used to time the total render (without compositing).
  this->Timer->StartTimer();

  if (this->AutoImageReductionFactor)
    {
    this->SetImageReductionFactorForUpdateRate(
      this->RenderWindow->GetDesiredUpdateRate());
    }

  int id;
  int numProcs = this->Controller->GetNumberOfProcesses();

  // Make adjustments for window size.
  int *tilesize = this->RenderWindow->GetSize();
  // To me, it seems dangerous for RenderWindow to return a size bigger
  // than it actually supports or for GetSize to not return the same values
  // as SetSize.  Yet this is the case when tile rendering is established
  // in RenderWindow.  Correct for this.
  int size[2];
  int *tilescale;
  tilescale = this->RenderWindow->GetTileScale();
  size[0] = tilesize[0]/tilescale[0];  size[1] = tilesize[1]/tilescale[1];
  if ((size[0] == 0) || (size[1] == 0))
    {
    // It helps to have a real window size.
    vtkDebugMacro("Resetting window size to 300x300");
    size[0] = size[1] = 300;
    this->RenderWindow->SetSize(size[0], size[1]);
    }
  this->FullImageSize[0] = size[0];
  this->FullImageSize[1] = size[1];
  //Round up.
  this->ReducedImageSize[0] =
    (int)((size[0]+this->ImageReductionFactor-1)/this->ImageReductionFactor);
  this->ReducedImageSize[1] =
    (int)((size[1]+this->ImageReductionFactor-1)/this->ImageReductionFactor);

  // Collect and distribute information about current state of RenderWindow
  vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
  winInfoInt.FullSize[0] = this->FullImageSize[0];
  winInfoInt.FullSize[1] = this->FullImageSize[1];
  winInfoInt.ReducedSize[0] = this->ReducedImageSize[0];
  winInfoInt.ReducedSize[1] = this->ReducedImageSize[1];
//  winInfoInt.NumberOfRenderers = rens->GetNumberOfItems();
  winInfoInt.NumberOfRenderers = 1;
  winInfoDouble.ImageReductionFactor = this->ImageReductionFactor;
  winInfoInt.UseCompositing = this->UseCompositing;
  winInfoDouble.DesiredUpdateRate = this->RenderWindow->GetDesiredUpdateRate();

  for (id = 0; id < numProcs; id++)
    {
    if (id == this->RootProcessId)
      {
      continue;
      }
    if (this->RenderEventPropagation)
      {
      this->Controller->TriggerRMI(id, NULL, 0,
                   vtkParallelRenderManager::RENDER_RMI_TAG);
      }
    this->Controller->Send((int *)(&winInfoInt), 
                           vtkParallelRenderManager::WIN_INFO_INT_SIZE, 
                           id,
                           vtkParallelRenderManager::WIN_INFO_INT_TAG);
    this->Controller->Send((double *)(&winInfoDouble), 
                           vtkParallelRenderManager::WIN_INFO_DOUBLE_SIZE,
                           id, 
                           vtkParallelRenderManager::WIN_INFO_DOUBLE_TAG);
    this->SendWindowInformation();
    }

  if (this->ImageReductionFactor > 1)
    {
    this->Viewports->SetNumberOfTuples(rens->GetNumberOfItems());
    }
  vtkRenderer *ren;
  ren = rens->GetFirstRenderer();
  
  if (ren)
    {
    ren->GetViewport(renInfoDouble.Viewport);

    // Adjust Renderer viewports to get reduced size image.
    if (this->ImageReductionFactor > 1)
      {
      this->Viewports->SetTuple(0, renInfoDouble.Viewport);
      renInfoDouble.Viewport[0] /= this->ImageReductionFactor;
      renInfoDouble.Viewport[1] /= this->ImageReductionFactor;
      renInfoDouble.Viewport[2] /= this->ImageReductionFactor;
      renInfoDouble.Viewport[3] /= this->ImageReductionFactor;
      // This is the offending call that made me copy
      // the superclasses StartRender method.
      // This ommision is the only change from the superclass method!!!
      //ren->SetViewport(renInfoDouble.Viewport);
      }

    vtkCamera *cam = ren->GetActiveCamera();
    cam->GetPosition(renInfoDouble.CameraPosition);
    cam->GetFocalPoint(renInfoDouble.CameraFocalPoint);
    cam->GetViewUp(renInfoDouble.CameraViewUp);
    cam->GetClippingRange(renInfoDouble.CameraClippingRange);
    renInfoDouble.CameraViewAngle = cam->GetViewAngle();
    ren->GetBackground(renInfoDouble.Background);
    if (cam->GetParallelProjection())
      {
      renInfoDouble.ParallelScale = cam->GetParallelScale();
      }
    else
      {
      renInfoDouble.ParallelScale = 0.0;
      }
    vtkLightCollection *lc = ren->GetLights();
    renInfoInt.NumberOfLights = lc->GetNumberOfItems();

    for (id = 0; id < numProcs; id++)
      {
      if (id == this->RootProcessId)
        {
        continue;
        }
      this->Controller->Send((int *)(&renInfoInt), 
                             vtkParallelRenderManager::REN_INFO_INT_SIZE, 
                             id,
                             vtkParallelRenderManager::REN_INFO_INT_TAG);
      this->Controller->Send((double *)(&renInfoDouble), 
                             vtkParallelRenderManager::REN_INFO_DOUBLE_SIZE,
                             id, 
                             vtkParallelRenderManager::REN_INFO_DOUBLE_TAG);
      }

    vtkLight *light;
    vtkCollectionSimpleIterator lsit;
    for (lc->InitTraversal(lsit); (light = lc->GetNextLight(lsit)); )
      {
      lightInfoDouble.Type = (double)(light->GetLightType());
      light->GetPosition(lightInfoDouble.Position);
      light->GetFocalPoint(lightInfoDouble.FocalPoint);
      
      for (id = 0; id < numProcs; id++)
        {
        if (id == this->RootProcessId) continue;
        this->Controller->Send((double *)(&lightInfoDouble),
                               vtkParallelRenderManager::LIGHT_INFO_DOUBLE_SIZE, 
                               id,
                               vtkParallelRenderManager::LIGHT_INFO_DOUBLE_TAG);
        }
      }

    this->SendRendererInformation(ren);
    }
//    }

  this->PreRenderProcessing();
}

void vtkDesktopDeliveryClient2::EndRender()
{
  this->Superclass::EndRender();

  if (this->UseCompositing)
    {
    // Restore the camera.
    vtkRendererCollection* rens = this->RenderWindow->GetRenderers();
    rens->InitTraversal();
    vtkRenderer* ren = rens->GetNextItem();
    vtkCamera *cam = ren->GetActiveCamera();
    cam->SetPosition(this->SavedCamera->GetPosition());
    cam->SetFocalPoint(this->SavedCamera->GetFocalPoint());
    cam->SetViewUp(this->SavedCamera->GetViewUp());
    cam->SetParallelProjection(this->SavedCamera->GetParallelProjection());
    cam->SetParallelScale(this->SavedCamera->GetParallelScale());
    cam->SetClippingRange(this->SavedCamera->GetClippingRange());
    }
}

void vtkDesktopDeliveryClient2::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ServerProcessId: " << this->ServerProcessId << endl;

  os << indent << "ReplaceActors: "
     << (this->ReplaceActors ? "on" : "off") << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "on" : "off") << endl;
  os << indent << "Squirt: "
     << (this->Squirt? "on" : "off") << endl;

  os << indent << "RemoteImageProcessingTime: "
     << this->RemoteImageProcessingTime << endl;
  os << indent << "TransferTime: " << this->TransferTime << endl;
  os << indent << "SquirtCompressionLevel: " << this->SquirtCompressionLevel << endl;
}
