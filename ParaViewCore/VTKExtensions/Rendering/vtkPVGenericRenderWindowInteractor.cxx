/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGenericRenderWindowInteractor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGenericRenderWindowInteractor.h"

#include "vtkCommand.h"
#include "vtkInteractorObserver.h"
#include "vtkObjectFactory.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVRenderViewProxy.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkPVConfig.h"

#ifdef PARAVIEW_ENABLE_QT_SUPPORT
#include "QVTKInteractor.h"
#endif

// this class wraps QVTKInteractor to provide an easy API to support
// delayed-switch-out-of-ineractive-render mode. Look at BUG #10232 for details.
class vtkPVGenericRenderWindowInteractorTimer : public
#ifdef PARAVIEW_ENABLE_QT_SUPPORT
                                                     QVTKInteractor
#else
                                                     vtkRenderWindowInteractor
#endif
{
  vtkPVGenericRenderWindowInteractor* Target;

public:
  static vtkPVGenericRenderWindowInteractorTimer* New();
#ifdef PARAVIEW_ENABLE_QT_SUPPORT
  vtkTypeMacro(vtkPVGenericRenderWindowInteractorTimer, QVTKInteractor);
#else
  vtkTypeMacro(vtkPVGenericRenderWindowInteractorTimer,
    vtkRenderWindowInteractor);
#endif

  void Timeout(unsigned long timeout)
    {
    this->CleanTimer();
    if (timeout > 0)
      {
      this->Target->InvokeEvent(
        vtkPVGenericRenderWindowInteractor::BeginDelayNonInteractiveRenderEvent);
      this->TimerId = this->CreateOneShotTimer(timeout);
      }
    if (this->TimerId == 0)
      {
      this->Target->SetForceInteractiveRender(false);
      this->Target->InvokeEvent(
        vtkPVGenericRenderWindowInteractor::EndDelayNonInteractiveRenderEvent);
      this->Target->Render();
      }
    }

  void CleanTimer()
    {
    if (this->TimerId > 0)
      {
      this->DestroyTimer(this->TimerId);
      }
    this->TimerId = 0;
    }


  void SetTarget(vtkPVGenericRenderWindowInteractor* target)
    { this->Target = target; }

#ifdef PARAVIEW_ENABLE_QT_SUPPORT
  virtual void TimerEvent(int timerId)
    {
    if(timerId == this->TimerId && this->Target)
      {
      bool need_render = this->Target->InteractiveRenderHappened;
      this->Target->SetForceInteractiveRender(false);
      this->Target->InvokeEvent(
        vtkPVGenericRenderWindowInteractor::EndDelayNonInteractiveRenderEvent);
      if (need_render)
        {
        this->Target->Render();
        }
      }
    this->Superclass::TimerEvent(timerId);
    this->CleanTimer();
    }
#endif
protected:
  vtkPVGenericRenderWindowInteractorTimer()
    {
    this->Target = 0;
    this->TimerId = 0;
    }
  ~vtkPVGenericRenderWindowInteractorTimer()
    {
    this->CleanTimer();
    }

  int TimerId;
};

vtkStandardNewMacro(vtkPVGenericRenderWindowInteractorTimer);

//-----------------------------------------------------------------------------
class vtkPVGenericRenderWindowInteractorObserver : public vtkCommand
{
public:
  static vtkPVGenericRenderWindowInteractorObserver* New()
    { return new vtkPVGenericRenderWindowInteractorObserver(); }

  void SetTarget(vtkPVGenericRenderWindowInteractor* target)
    {
    this->Target = target;
    }

  virtual void Execute(vtkObject* vtkNotUsed(caller), unsigned long event, void*
    vtkNotUsed(data))
    {
    if (this->Target)
      {
      if (event == vtkCommand::StartInteractionEvent)
        {
        this->Target->SetInteractiveRenderEnabled(1);
        }
      else if (event == vtkCommand::EndInteractionEvent)
        {
        if (this->Target->GetInteractiveRenderEnabled())
          {
          this->Target->SetInteractiveRenderEnabled(0);
          // This call will call vtkPVGenericRenderWindowInteractor::Render()
          // hence we don't call it explicitly here.
          }
        }
      }
    }

protected:
  vtkPVGenericRenderWindowInteractorObserver()
    {
    this->Target = 0;
    }
  vtkPVGenericRenderWindowInteractor* Target;
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVGenericRenderWindowInteractor);
vtkCxxSetObjectMacro(vtkPVGenericRenderWindowInteractor,Renderer,vtkRenderer);

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::vtkPVGenericRenderWindowInteractor()
{
  this->PVRenderView = NULL;
  this->Renderer = NULL;
  this->InteractiveRenderEnabled = 0;
  this->Observer = vtkPVGenericRenderWindowInteractorObserver::New();
  this->Observer->SetTarget(this);

  this->CenterOfRotation[0] = this->CenterOfRotation[1]
    = this->CenterOfRotation[2] = 0;

  this->Timer = vtkPVGenericRenderWindowInteractorTimer::New();
  this->Timer->SetTarget(this);
  this->ForceInteractiveRender = false;
  this->NonInteractiveRenderDelay = 2000;
  this->InteractiveRenderHappened = false;
}

//----------------------------------------------------------------------------
vtkPVGenericRenderWindowInteractor::~vtkPVGenericRenderWindowInteractor()
{
  this->Observer->SetTarget(0);
  this->Observer->Delete();

  this->Timer->CleanTimer();
  this->Timer->SetTarget(0);
  this->Timer->Delete();

  this->SetPVRenderView(NULL);
  this->SetRenderer(NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetCenterOfRotation(double x,
  double y, double z)
{
  if (this->CenterOfRotation[0] != x ||
    this->CenterOfRotation[1] != y ||
    this->CenterOfRotation[2] != z)
    {
    this->CenterOfRotation[0] = x;
    this->CenterOfRotation[1] = y;
    this->CenterOfRotation[2] = z;
    vtkPVInteractorStyle* style = vtkPVInteractorStyle::SafeDownCast(
      this->GetInteractorStyle());
    // Pass center of rotation.
    if (style)
      {
      style->SetCenterOfRotation(this->CenterOfRotation);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetInteractorStyle(
  vtkInteractorObserver *style)
{
  if (this->GetInteractorStyle())
    {
    this->GetInteractorStyle()->RemoveObserver(this->Observer);
    }

  this->Superclass::SetInteractorStyle(style);

  // Pass center of rotation.
  if (vtkPVInteractorStyle::SafeDownCast(style))
    {
    vtkPVInteractorStyle::SafeDownCast(style)->SetCenterOfRotation(
      this->CenterOfRotation);
    }

  if (this->GetInteractorStyle())
    {
    this->GetInteractorStyle()->AddObserver(
      vtkCommand::StartInteractionEvent, this->Observer);
    this->GetInteractorStyle()->AddObserver(
      vtkCommand::EndInteractionEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetPVRenderView(vtkPVRenderViewProxy *view)
{
  if (this->PVRenderView != view)
    {
    if(this->PVRenderView)
      {
      this->PVRenderView->UnRegister(this);
      }
    // to avoid circular references
    this->PVRenderView = view;
    if (this->PVRenderView != NULL)
      {
      this->PVRenderView->Register(this);
      }
    }
}

//----------------------------------------------------------------------------
vtkRenderer* vtkPVGenericRenderWindowInteractor::FindPokedRenderer(int,int)
{
  if (this->Renderer == NULL)
    {
    vtkErrorMacro("Renderer has not been set.");
    }

  return this->Renderer;
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::Render()
{
  if ( this->PVRenderView == NULL || this->RenderWindow == NULL)
    { // The case for interactors on the satellite processes.
    return;
    }

  // This should fix the problem of the plane widget render 
  if (this->InteractiveRenderEnabled)
    {
    this->InvokeEvent(vtkCommand::InteractionEvent);
    this->PVRenderView->Render();
    this->InteractiveRenderHappened =
      this->PVRenderView->LastRenderWasInteractive();
    }
  else if (this->ForceInteractiveRender && this->InteractiveRenderHappened)
    {
    this->PVRenderView->Render();
    }
  else
    {
    this->InteractiveRenderHappened = false;
    this->PVRenderView->EventuallyRender();
    }
}


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::SetInteractiveRenderEnabled(int val)
{
  if (this->InteractiveRenderEnabled == val)
    {
    return;
    }
  this->Modified();
  this->InteractiveRenderEnabled = val;
  // when switch to non-interactive mode, we set ForceInteractiveRender to ON.
  // Then it is cleared on timeout by vtkPVGenericRenderWindowInteractorTimer.
  this->SetForceInteractiveRender(val == 0);
  this->Timer->CleanTimer();
  if (val == 0)
    {
    // switch to non-interactive render.
    this->Timer->Timeout(
      this->PVRenderView->LastRenderWasInteractive()?
      this->NonInteractiveRenderDelay : 0);
    }
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnLeftPress(int x, int y, 
                                                     int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMiddlePress(int x, int y, 
                                                       int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnRightPress(int x, int y, 
                                                      int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnLeftRelease(int x, int y, 
                                                       int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMiddleRelease(int x, int y, 
                                                         int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnRightRelease(int x, int y, 
                                                        int control, int shift)
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, control, shift);
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnMove(int x, int y) 
{
  this->SetEventInformation(x, this->RenderWindow->GetSize()[1]-y, 
                            this->ControlKey, this->ShiftKey,
                            this->KeyCode, this->RepeatCount,
                            this->KeySym);
  this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
}

//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::OnKeyPress(char keyCode, int x, int y) 
{
  this->SetEventPosition(x, this->RenderWindow->GetSize()[1]-y);
  this->KeyCode = keyCode;
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
void vtkPVGenericRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PVRenderView: " << this->GetPVRenderView() << endl;
  os << indent << "InteractiveRenderEnabled: " 
     << this->InteractiveRenderEnabled << endl;
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "CenterOfRotation: " << this->CenterOfRotation[0] << ", "
    << this->CenterOfRotation[1] << ", " << this->CenterOfRotation[2] << endl;
}
