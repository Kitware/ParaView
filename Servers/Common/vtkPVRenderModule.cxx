/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRenderModule.h"
#include "vtkToolkits.h"

#include "vtkClientServerStream.h"
#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVProcessModule.h"
#include "vtkPVConfig.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkFloatArray.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPVRenderModule, "1.1");

//===========================================================================
//***************************************************************************
class vtkPVRenderModuleObserver : public vtkCommand
{
public:
  static vtkPVRenderModuleObserver *New() 
    {return new vtkPVRenderModuleObserver;};

  vtkPVRenderModuleObserver()
    {
      this->PVRenderModule = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void*)
    {
      if ( this->PVRenderModule )
        {
        if (event == vtkCommand::StartEvent &&
            vtkRenderer::SafeDownCast(wdg))
          {
          this->PVRenderModule->StartRenderEvent();
          }
        }
    }

  vtkPVRenderModule* PVRenderModule;
};
//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVRenderModule::vtkPVRenderModule()
{
  this->ProcessModule = NULL;
  this->TotalVisibleMemorySizeValid = 0;
  
  this->Displays = vtkCollection::New();

  this->Renderer = 0;
  this->Renderer2D = 0;
  this->RenderWindow = 0;
  this->RendererID.ID     = 0;
  this->Renderer2DID.ID   = 0;
  this->RenderWindowID.ID = 0;
  this->InteractiveRenderTime    = 0;
  this->StillRenderTime          = 0;

  this->ResetCameraClippingRangeTag = 0;

  this->RenderInterruptsEnabled = 1;

  this->Observer = vtkPVRenderModuleObserver::New();
  this->Observer->PVRenderModule = this;
}

//----------------------------------------------------------------------------
vtkPVRenderModule::~vtkPVRenderModule()
{
  vtkPVProcessModule* pm = this->ProcessModule;
  
  if (this->Renderer && this->ResetCameraClippingRangeTag > 0)
    {
    this->Renderer->RemoveObserver(this->ResetCameraClippingRangeTag);
    this->ResetCameraClippingRangeTag = 0;
    }

  this->Displays->Delete();
  this->Displays = NULL;

  
  if (this->Renderer)
    {
    if (this->ResetCameraClippingRangeTag > 0)
      {
      this->Renderer->RemoveObserver(this->ResetCameraClippingRangeTag);
      this->ResetCameraClippingRangeTag = 0;
      }

    if ( pm )
      {
      pm->DeleteStreamObject(this->RendererID);
      }
    this->RendererID.ID = 0;
    }
  
  if (this->Renderer2D)
    {
    if ( pm )
      {
      pm->DeleteStreamObject(this->Renderer2DID);
      }
    this->Renderer2DID.ID = 0;
    }

  if (this->RenderWindow)
    {
    if ( pm )
      { 
      pm->DeleteStreamObject(this->RenderWindowID);
      }
    this->RenderWindowID.ID = 0;
    }
  if(pm)
    {
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  
  this->Observer->Delete();
  this->Observer = 0;

  this->SetProcessModule(0);
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::StartRenderEvent()
{
  // make the overlay renderer match the view of the 3d renderer
  if (this->Renderer2D)
    {
    this->Renderer2D->GetActiveCamera()->SetClippingRange(
      this->Renderer->GetActiveCamera()->GetClippingRange());
    this->Renderer2D->GetActiveCamera()->SetPosition(
      this->Renderer->GetActiveCamera()->GetPosition());
    this->Renderer2D->GetActiveCamera()->SetFocalPoint(
      this->Renderer->GetActiveCamera()->GetFocalPoint());
    this->Renderer2D->GetActiveCamera()->SetViewUp(
      this->Renderer->GetActiveCamera()->GetViewUp());

    this->Renderer2D->GetActiveCamera()->SetParallelProjection(
      this->Renderer->GetActiveCamera()->GetParallelProjection());
    this->Renderer2D->GetActiveCamera()->SetParallelScale(
      this->Renderer->GetActiveCamera()->GetParallelScale());
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetProcessModule(vtkPVProcessModule *pm)
{
  if (this->ProcessModule)
    {
    if (pm == NULL)
      {
      this->ProcessModule->UnRegister(this);
      this->ProcessModule = NULL;
      return;
      }
    vtkErrorMacro("ProcessModule already set.");
    return;
    }
  if (pm == NULL)
    {
    return;
    }  
  vtkClientServerStream& stream = pm->GetStream();
  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->ProcessModule = pm;
  this->ProcessModule->Register(this);

  this->RendererID = pm->NewStreamObject("vtkRenderer");
  this->Renderer2DID = pm->NewStreamObject("vtkRenderer");
  this->RenderWindowID = pm->NewStreamObject("vtkRenderWindow");
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  this->Renderer = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->RendererID));
  this->Renderer2D = 
    vtkRenderer::SafeDownCast(
      pm->GetObjectFromID(this->Renderer2DID));
  this->RenderWindow = 
    vtkRenderWindow::SafeDownCast(
      pm->GetObjectFromID(this->RenderWindowID));

  if (pm->GetUseStereoRendering())
    {
    pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                    << "StereoCapableWindowOn" 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
                    << "StereoRenderOn" 
                    << vtkClientServerStream::End;
    //pm->GetStream() << vtkClientServerStream::Invoke << this->RenderWindowID 
    //                << "SetStereoTypeToCrystalEyes" 
    //                << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::RENDER_SERVER);
    }

  if (pm->GetUseStereoRendering())
    {
    this->RenderWindow->StereoCapableWindowOn();
    this->RenderWindow->StereoRenderOn();
    }

  // We cannot erase the zbuffer.  We need it for picking.  
  stream << vtkClientServerStream::Invoke
         << this->Renderer2DID << "EraseOff" 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->Renderer2DID << "SetLayer" << 2 
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "SetNumberOfLayers" << 3
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->RendererID
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << this->RenderWindowID << "AddRenderer" << this->Renderer2DID
         << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  this->InitializeObservers();
}

//----------------------------------------------------------------------------
// This is a bit of a pain.  I do ResetCameraClippingRange as a call back
// because the PVInteractorStyles call ResetCameraClippingRange 
// directly on the renderer.  Since they are PV styles, I might
// have them call the render module directly like they do for render.
void vtkPVRenderModuleResetCameraClippingRange(
  vtkObject *caller, unsigned long vtkNotUsed(event),void *clientData, void *)
{
  double bds[6];
  double range1[2];
  double range2[2];

  vtkPVRenderModule *self = (vtkPVRenderModule *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  // Get default clipping range.
  // Includes 3D widgets but not all processes.
  ren->GetActiveCamera()->GetClippingRange(range1);

  self->ComputeVisiblePropBounds(bds);
  ren->ResetCameraClippingRange(bds);
  // Get part clipping range.
  // Includes all process partitions, but not 3d Widgets.
  ren->GetActiveCamera()->GetClippingRange(range2);

  // Merge
  if (range1[0] < range2[0])
    {
    range2[0] = range1[0];
    }
  if (range1[1] > range2[1])
    {
    range2[1] = range1[1];
    }
  // Includes all process partitions and 3D Widgets.
  ren->GetActiveCamera()->SetClippingRange(range2);
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::InitializeObservers()
{   
  // the 2d renderer must be kept in sync with the main renderer
  this->Renderer->AddObserver(
    vtkCommand::StartEvent, this->Observer);  

  // Make sure we have a chance to set the clipping range properly.
  vtkCallbackCommand* cbc;
  cbc = vtkCallbackCommand::New();
  cbc->SetCallback(vtkPVRenderModuleResetCameraClippingRange);
  cbc->SetClientData((void*)this);
  // ren will delete the cbc when the observer is removed.
  this->ResetCameraClippingRangeTag = 
    this->Renderer->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
  cbc->Delete();
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkPVRenderModule::GetRenderWindow()
{
  return this->RenderWindow;
}

//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderModule::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderModule::GetRenderer2D()
{
  return this->Renderer2D;
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::SetBackgroundColor(float r, float g, float b)
{
  vtkPVProcessModule *pm = this->GetProcessModule();
  vtkClientServerStream& stream = pm->GetStream();
  stream << vtkClientServerStream::Invoke << this->RendererID << "SetBackground"
         << r << g << b << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::InteractiveRender()
{
  this->UpdateAllDisplays();

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::StillRender()
{
  this->UpdateAllDisplays();

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->GetProcessModule()->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->SetDesiredUpdateRate(0.002);
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseParallelProjection(int val)
{
  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOff();
    }
}

//----------------------------------------------------------------------------
float vtkPVRenderModule::GetZBufferValue(int x, int y)
{
  vtkFloatArray *array = vtkFloatArray::New();
  float val;

  array->SetNumberOfTuples(1);
  this->RenderWindow->GetZbufferData(x,y, x, y,
                                     array);
  val = array->GetValue(0);
  array->Delete();
  return val;  
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InteractiveRenderTime: " 
     << this->InteractiveRenderTime << endl;
  os << indent << "StillRenderTime: " << this->StillRenderTime << endl;
  os << indent << "RenderInterruptsEnabled: " 
     << (this->RenderInterruptsEnabled ? "on" : "off") << endl;

  if (this->ProcessModule)
    {
    os << indent << "ProcessModule: " << this->ProcessModule << endl;
    }
  else
    {
    os << indent << "ProcessModule: NULL" << endl;
    }


  os << indent << "TotalVisibleMemorySizeValid: " 
     << this->TotalVisibleMemorySizeValid << endl;
}

