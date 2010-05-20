/*=========================================================================

Program:   ParaView
Module:    vtkDesktopDeliveryServer.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkDesktopDeliveryServer.h"
#include "vtkDoubleArray.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSquirtCompressor.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"

#include <assert.h>

static void SatelliteStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event),
                                 void *clientData, void *);
static void SatelliteEndRender(vtkObject *caller,
                               unsigned long vtkNotUsed(event),
                               void *clientData, void *);
static void SatelliteStartParallelRender(vtkObject *caller,
                                         unsigned long vtkNotUsed(event),
                                         void *clientData, void *);
static void SatelliteEndParallelRender(vtkObject *caller,
                                       unsigned long vtkNotUsed(event),
                                       void *clientData, void *);

vtkStandardNewMacro(vtkDesktopDeliveryServer);

//----------------------------------------------------------------------------
vtkDesktopDeliveryServer::vtkDesktopDeliveryServer()
{
  this->ParallelRenderManager = NULL;
  this->RemoteDisplay = 1;
  this->SquirtBuffer = vtkUnsignedCharArray::New();
}

//----------------------------------------------------------------------------
vtkDesktopDeliveryServer::~vtkDesktopDeliveryServer()
{
  this->SetParallelRenderManager(NULL);
  this->SquirtBuffer->Delete();
}

//----------------------------------------------------------------------------
void
vtkDesktopDeliveryServer::SetController(vtkMultiProcessController *controller)
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
    this->RootProcessId = 1 - this->Controller->GetLocalProcessId();
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer
::SetParallelRenderManager(vtkParallelRenderManager *prm)
{
  if (this->ParallelRenderManager == prm)
    {
    return;
    }
  this->Modified();

  if (this->ParallelRenderManager)
    {
    // Remove all observers.
    this->ParallelRenderManager->RemoveObserver(this->StartParallelRenderTag);
    this->ParallelRenderManager->RemoveObserver(this->EndParallelRenderTag);

    // Delete the reference.
    this->ParallelRenderManager->UnRegister(this);
    this->ParallelRenderManager = NULL;
    }

  this->ParallelRenderManager = prm;
  if (this->ParallelRenderManager)
    {
    // Create a reference.
    this->ParallelRenderManager->Register(this);

    if (this->RemoteDisplay)
      {
      // No need to write the image back on the render server.
      this->ParallelRenderManager->WriteBackImagesOff();
      }
    else
      {
      // Presumably someone is viewing the remote screen, perhaps in a
      // tile display.
      this->ParallelRenderManager->WriteBackImagesOn();
      }

    // Attach observers.
    vtkCallbackCommand *cbc;

    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::SatelliteStartParallelRender);
    cbc->SetClientData((void *)this);
    this->StartParallelRenderTag
      = this->ParallelRenderManager->AddObserver(vtkCommand::StartEvent, cbc);
    // ParallelRenderManager will really delete cbc when observer is removed.
    cbc->Delete();

    cbc = vtkCallbackCommand::New();
    cbc->SetCallback(::SatelliteEndParallelRender);
    cbc->SetClientData((void *)this);
    this->EndParallelRenderTag
      = this->ParallelRenderManager->AddObserver(vtkCommand::EndEvent, cbc);
    // ParallelRenderManager will really delete cbc when observer is removed.
    cbc->Delete();

    // Remove observers to RenderWindow.  We use the prm instead.
    if (this->ObservingRenderWindow)
      {
      this->RenderWindow->RemoveObserver(this->StartRenderTag);
      this->RenderWindow->RemoveObserver(this->EndRenderTag);
      this->ObservingRenderWindow = false;
      }
    }
  else
    {
    // Apparently we added and then removed a ParallelRenderManager.
    // Restore RenderWindow observers.
    if (this->RenderWindow)
      {
      vtkCallbackCommand *cbc;
        
      vtkRendererCollection *rens = this->GetRenderers();
      vtkRenderer *ren;
      rens->InitTraversal();
      ren = rens->GetNextItem();
      if (ren)
        {
        this->ObservingRenderWindow = true;
        
        cbc= vtkCallbackCommand::New();
        cbc->SetCallback(::SatelliteStartRender);
        cbc->SetClientData((void*)this);
        this->StartRenderTag
          = ren->AddObserver(vtkCommand::StartEvent,cbc);
        cbc->Delete();
        
        cbc = vtkCallbackCommand::New();
        cbc->SetCallback(::SatelliteEndRender);
        cbc->SetClientData((void*)this);
        this->EndRenderTag
          = ren->AddObserver(vtkCommand::EndEvent,cbc);
        cbc->Delete();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::SetRenderWindow(vtkRenderWindow *renWin)
{
  this->Superclass::SetRenderWindow(renWin);

  if (this->ObservingRenderWindow && this->ParallelRenderManager)
    {
    vtkRendererCollection *rens = this->GetRenderers();
    vtkRenderer *ren;
    rens->InitTraversal();
    ren = rens->GetNextItem();
    if (ren)
      {
      // Don't need the observers we just attached.
      ren->RemoveObserver(this->StartRenderTag);
      ren->RemoveObserver(this->EndRenderTag);
      this->ObservingRenderWindow = false;
      }
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::SetRemoteDisplay(int flag)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting RemoteDisplay to " << flag);
  if (this->RemoteDisplay != flag)
    {
    this->RemoteDisplay = flag;
    this->Modified();

    if (this->ParallelRenderManager)
      {
      if (this->RemoteDisplay)
        {
        // No need to write the image back on the render server.
        this->ParallelRenderManager->WriteBackImagesOff();
        }
      else
        {
        // Presumably someone is viewing the remote screen, perhaps in a
        // tile display.
        this->ParallelRenderManager->WriteBackImagesOn();
        }
      }
    }
}

//----------------------------------------------------------------------------
bool vtkDesktopDeliveryServer::ProcessWindowInformation(vtkMultiProcessStream& stream)
{
  if (!this->Superclass::ProcessWindowInformation(stream))
    {
    return false;
    }

  vtkDesktopDeliveryServer::SquirtOptions squirt_options; 
  if (!squirt_options.Restore(stream))
    {
    vtkErrorMacro("Failed to read SquirtOptions.");
    return false;
    }

  this->Squirt = squirt_options.Enabled;
  this->SquirtCompressionLevel = squirt_options.CompressLevel;
  return true;
}  

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::PreRenderProcessing()
{
  vtkDebugMacro("PreRenderProcessing");

  // Send remote display flag.
  this->Controller->Send(&this->RemoteDisplay, 1, this->RootProcessId,
                         vtkDesktopDeliveryServer::REMOTE_DISPLAY_TAG);

  if (this->ParallelRenderManager)
    {
    // If we are chaining this to another parallel render manager, restore
    // the renderers so that the other parallel render manager may render
    // the desired image size.
    if (this->ImageReductionFactor > 1)
      {
      vtkRendererCollection *rens = this->GetRenderers();
      // Just grab first renderer because that is all that the superclass
      // really does anything with right now.
      rens->InitTraversal();
      vtkRenderer *ren = rens->GetNextItem();
      double *viewport = this->Viewports->GetPointer(0);
      ren->SetViewport(viewport);
      }

    // Make sure the prm has the correct image reduction factor.
    if (  this->ParallelRenderManager->GetMaxImageReductionFactor()
          < this->ImageReductionFactor)
      {
      this->ParallelRenderManager
        ->SetMaxImageReductionFactor(this->ImageReductionFactor);
      }
    this->ParallelRenderManager
      ->SetImageReductionFactor(this->ImageReductionFactor);
    this->ParallelRenderManager->AutoImageReductionFactorOff();

    // Pass UseCompositing flag.
    this->ParallelRenderManager->SetUseCompositing(this->UseCompositing);
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::PostRenderProcessing()
{
  vtkDebugMacro("PostRenderProcessing");

  vtkDesktopDeliveryServer::ImageParams ip;
  ip.RemoteDisplay = this->RemoteDisplay;

  if (ip.RemoteDisplay)
    {
    this->ReadReducedImage();
    ip.NumberOfComponents = this->ReducedImage->GetNumberOfComponents();
    ip.SquirtCompressed = this->Squirt && (ip.NumberOfComponents == 4);
    ip.ImageSize[0] = this->ReducedImageSize[0];
    ip.ImageSize[1] = this->ReducedImageSize[1];

    if (ip.SquirtCompressed)
      {
      this->SquirtCompress(this->ReducedImage, this->SquirtBuffer);
      ip.NumberOfComponents = 4;
      ip.BufferSize
        = ip.NumberOfComponents*this->SquirtBuffer->GetNumberOfTuples();
      this->Controller->Send((int *)(&ip),
                             vtkDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                             this->RootProcessId,
                             vtkDesktopDeliveryServer::IMAGE_PARAMS_TAG);
      this->Controller->Send(this->SquirtBuffer->GetPointer(0), ip.BufferSize,
                             this->RootProcessId,
                             vtkDesktopDeliveryServer::IMAGE_TAG);
      }
    else
      {
      ip.BufferSize
        = ip.NumberOfComponents*this->ReducedImage->GetNumberOfTuples();
      this->Controller->Send((int *)(&ip),
                             vtkDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                             this->RootProcessId,
                             vtkDesktopDeliveryServer::IMAGE_PARAMS_TAG);
      this->Controller->Send(this->ReducedImage->GetPointer(0), ip.BufferSize,
                             this->RootProcessId,
                             vtkDesktopDeliveryServer::IMAGE_TAG);
      }
    }
  else
    {
    this->Controller->Send((int *)(&ip),
                           vtkDesktopDeliveryServer::IMAGE_PARAMS_SIZE,
                           this->RootProcessId,
                           vtkDesktopDeliveryServer::IMAGE_PARAMS_TAG);
    }

  // Send timing metrics
  vtkDesktopDeliveryServer::TimingMetrics tm;
  if (this->ParallelRenderManager)
    {
    tm.ImageProcessingTime
      = this->ParallelRenderManager->GetImageProcessingTime();
    }
  else
    {
    tm.ImageProcessingTime = 0.0;
    }

  this->Controller->Send((double *)(&tm),
                         vtkDesktopDeliveryServer::TIMING_METRICS_SIZE,
                         this->RootProcessId,
                         vtkDesktopDeliveryServer::TIMING_METRICS_TAG);

  // If another parallel render manager has already made an image, don't
  // clober it.
  if (this->ParallelRenderManager)
    {
    this->RenderWindowImageUpToDate = true;
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::SetRenderWindowSize()
{
  if (this->RemoteDisplay)
    {
    this->Superclass::SetRenderWindowSize();
    }
  else
    {
    this->RenderWindow->Start();
    int *size = this->RenderWindow->GetActualSize();
    this->FullImageSize[0] = size[0];
    this->FullImageSize[1] = size[1];
    this->ReducedImageSize[0] = (int)(size[0]/this->ImageReductionFactor);
    this->ReducedImageSize[1] = (int)(size[1]/this->ImageReductionFactor);
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::ReadReducedImage()
{
  if (this->ParallelRenderManager)
    {
    int *size = this->ParallelRenderManager->GetReducedImageSize();
    if ( this->ReducedImageSize[0] != size[0]
      || this->ReducedImageSize[1] != size[1] )
      {
      vtkDebugMacro(<< "Coupled parallel render manager reports unexpected reduced image size\n"
                    << "Expected size: " << this->ReducedImageSize[0] << " "
                    << this->ReducedImageSize[1] << "\n"
                    << "Reported size: " << size[0] << " " << size[1]);
      if ( this->ReducedImageSize[0] == this->FullImageSize[0]
        && this->ReducedImageSize[1] == this->FullImageSize[1] )
        {
        vtkWarningMacro(<< "The coupled render manager has apparently resized the window.\n"
                        << "Operation will still work normally, but the client may waste many cycles\n"
                        << "resizing the resulting window.");
        }
      this->ReducedImageSize[0] = size[0];
      this->ReducedImageSize[1] = size[1];
      }
    this->ParallelRenderManager->GetReducedPixelData(this->ReducedImage);
    this->ReducedImageUpToDate = true;
    }
  else
    {
    this->Superclass::ReadReducedImage();
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::LocalComputeVisiblePropBounds(vtkRenderer *ren,
                                                             double bounds[6])
{
  if (this->ParallelRenderManager)
    {
    this->ParallelRenderManager->ComputeVisiblePropBounds(ren, bounds);
    }
  else
    {
    this->Superclass::LocalComputeVisiblePropBounds(ren, bounds);
    }
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::SquirtCompress(vtkUnsignedCharArray *in,
                                              vtkUnsignedCharArray *out)
{
  vtkSquirtCompressor *compressor = vtkSquirtCompressor::New();
  compressor->SetInput(in);
  compressor->SetSquirtLevel(this->SquirtCompressionLevel);
  compressor->SetOutput(out);
  compressor->Compress();
  compressor->Delete();
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ParallelRenderManager: "
     << this->ParallelRenderManager << endl;
  os << indent << "RemoteDisplay: "
     << (this->RemoteDisplay ? "on" : "off") << endl;
}


//----------------------------------------------------------------------------
static void SatelliteStartRender(vtkObject *caller,
                                 unsigned long vtkNotUsed(event),
                                 void *clientData, void *)
{
  vtkDesktopDeliveryServer *self = (vtkDesktopDeliveryServer *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteStartRender();
}

//----------------------------------------------------------------------------
static void SatelliteEndRender(vtkObject *caller,
                               unsigned long vtkNotUsed(event),
                               void *clientData, void *)
{
  vtkDesktopDeliveryServer *self = (vtkDesktopDeliveryServer *)clientData;
  if (caller != self->GetRenderWindow())
    {
    vtkGenericWarningMacro("vtkDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteEndRender();
}

//----------------------------------------------------------------------------
static void SatelliteStartParallelRender(vtkObject *caller,
                                         unsigned long vtkNotUsed(event),
                                         void *clientData, void *)
{
  vtkDesktopDeliveryServer *self = (vtkDesktopDeliveryServer *)clientData;
  if (caller != self->GetParallelRenderManager())
    {
    vtkGenericWarningMacro("vtkDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteStartRender();
}

//----------------------------------------------------------------------------
static void SatelliteEndParallelRender(vtkObject *caller,
                                       unsigned long vtkNotUsed(event),
                                       void *clientData, void *)
{
  vtkDesktopDeliveryServer *self = (vtkDesktopDeliveryServer *)clientData;
  if (caller != self->GetParallelRenderManager())
    {
    vtkGenericWarningMacro("vtkDesktopDeliveryServer caller mismatch");
    return;
    }
  self->SatelliteEndRender();
}

//----------------------------------------------------------------------------
void vtkDesktopDeliveryServer::SquirtOptions::Save(vtkMultiProcessStream& stream)
{
  stream << vtkDesktopDeliveryServer::SQUIRT_OPTIONS_TAG 
    << this->Enabled << this->CompressLevel;
}

//----------------------------------------------------------------------------
bool vtkDesktopDeliveryServer::SquirtOptions::Restore(vtkMultiProcessStream& stream)
{
  int tag;
  stream >> tag;
  if (tag != vtkDesktopDeliveryServer::SQUIRT_OPTIONS_TAG)
    {
    return false;
    }
  stream >> this->Enabled >> this->CompressLevel;
  return true;
}
