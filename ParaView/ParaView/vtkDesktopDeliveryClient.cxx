// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDesktopDeliveryClient.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

#include "vtkDesktopDeliveryClient.h"
#include "vtkDesktopDeliveryServer.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkCallbackCommand.h"
#include "vtkCubeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkRendererCollection.h"
#include "vtkCamera.h"
#include "vtkLight.h"
#include "vtkTimerLog.h"
#include "vtkLightCollection.h"
#include "vtkFloatArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiProcessController.h"

vtkCxxRevisionMacro(vtkDesktopDeliveryClient, "1.1.2.1");
vtkStandardNewMacro(vtkDesktopDeliveryClient);

vtkDesktopDeliveryClient::vtkDesktopDeliveryClient()
{
  this->ReplaceActors = 1;
}

vtkDesktopDeliveryClient::~vtkDesktopDeliveryClient()
{
}

void
vtkDesktopDeliveryClient::SetController(vtkMultiProcessController *controller)
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

void vtkDesktopDeliveryClient::PreRenderProcessing()
{
  // Get remote display flag
  this->Controller->Receive(&this->RemoteDisplay, 1, this->ServerProcessId,
          vtkDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if (this->RemoteDisplay)
    {
    // Turn swap buffers off so we can put the final image into the render
    // window before it is shown to the viewer.
    this->RenderWindow->SwapBuffersOff();
    }
  else
    {
    if (this->ImageReductionFactor > 1)
      {
      // Since we're not replacing the image, restore the renderer viewports.
      vtkRendererCollection *rens = this->RenderWindow->GetRenderers();
      vtkRenderer *ren;
      int i;
      for (rens->InitTraversal(), i = 0; ren = rens->GetNextItem(); i++)
  {
  ren->SetViewport(this->Viewports->GetTuple(i));
  }
      }
    }
}

void vtkDesktopDeliveryClient::PostRenderProcessing()
{
  // Adjust render time for actual render on server.
  this->Timer->StartTimer();
  this->Controller->Barrier();
  this->Timer->StopTimer();
  this->RenderTime += this->Timer->GetElapsedTime();

  if (this->RemoteDisplay)
    {
    // Receive image.
    this->Timer->StartTimer();
    this->ReducedImage->SetNumberOfComponents(3);
    if (this->ImageReductionFactor == 1)
      {
      this->FullImage->SetNumberOfComponents(3);
      this->FullImage->SetNumberOfTuples(  this->FullImageSize[0]
           * this->FullImageSize[1]);
      this->FullImageUpToDate = true;
      this->ReducedImage->SetArray(this->FullImage->GetPointer(0),
           this->FullImage->GetSize(), 1);
      }
    this->ReducedImage->SetNumberOfTuples(  this->ReducedImageSize[0]
            * this->ReducedImageSize[1]);

    this->Controller->Receive(this->ReducedImage->GetPointer(0),
            3*this->ReducedImage->GetNumberOfTuples(),
            this->ServerProcessId,
            vtkDesktopDeliveryServer::IMAGE_TAG);
    this->ReducedImageUpToDate = true;

    this->Timer->StopTimer();
    this->TransferTime = this->Timer->GetElapsedTime();

    if (this->WriteBackImages)
      {
      if (this->MagnifyImages)
  {
  this->MagnifyReducedImage();
  this->RenderWindow->SetPixelData(0, 0, this->FullImageSize[0]-1,
           this->FullImageSize[1]-1,
           this->FullImage, 0);
  }
      else
  {
  this->ReadReducedImage();
  this->RenderWindow->SetPixelData(0, 0, this->ReducedImageSize[0]-1,
           this->ReducedImageSize[1]-1,
           this->ReducedImage, 0);
  }
      this->RenderWindowImageUpToDate = true;
      }

    // Force swap buffers here.
    this->RenderWindow->SwapBuffersOn();
    this->RenderWindow->Frame();
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

void vtkDesktopDeliveryClient::ComputeVisiblePropBounds(vtkRenderer *ren,
              float bounds[6])
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

void vtkDesktopDeliveryClient::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ServerProcessId: " << this->ServerProcessId << endl;

  os << indent << "ReplaceActors: "
     << (this->ReplaceActors ? "on" : "off") << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "on" : "off") << endl;

  os << indent << "RemoteImageProcessingTime: "
     << this->RemoteImageProcessingTime << endl;
  os << indent << "TransferTime: " << this->TransferTime << endl;
}
