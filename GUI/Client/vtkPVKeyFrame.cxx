/*=========================================================================

  Program:   ParaView
  Module:    vtkPVKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVKeyFrame.h"

#include "vtkCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWThumbWheel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVWindow.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMKeyFrameProxy.h"
#include "vtkSMObject.h"
#include "vtkSMProxyManager.h"

vtkCxxRevisionMacro(vtkPVKeyFrame, "1.30");
vtkCxxSetObjectMacro(vtkPVKeyFrame, AnimationScene, vtkPVAnimationScene);
//*****************************************************************************
class vtkPVKeyFrameObserver : public vtkCommand
{
public:
  static vtkPVKeyFrameObserver* New()
    {
    return new vtkPVKeyFrameObserver;
    }
  void SetTarget(vtkPVKeyFrame* t)
    {
    this->Target = t;
    }
  virtual void Execute(vtkObject* obj, unsigned long event, void* calldata)
    {
    if (this->Target)
      {
      this->Target->ExecuteEvent(obj, event, calldata);
      }
    }
protected:
  vtkPVKeyFrameObserver()
    {
    this->Target = NULL;
    }
  vtkPVKeyFrame* Target;
};
//*****************************************************************************
//Helper methods to down cast the property and set value.
inline static int DoubleVectPropertySetElement(vtkSMProxy *proxy, 
  const char* propertyname, double val, int index = 0)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    proxy->GetProperty(propertyname));
  if (!dvp)
    {
    return 0;
    }
  return dvp->SetElement(index, val);
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame::vtkPVKeyFrame()
{
  this->Observer = vtkPVKeyFrameObserver::New();
  this->Observer->SetTarget(this);
  this->KeyFrameProxy = NULL;
  this->KeyFrameProxyName = NULL;
  this->KeyFrameProxyXMLName = NULL;
  this->TimeLabel = vtkKWLabel::New();
  this->TimeThumbWheel = vtkKWThumbWheel::New();
  this->TimeBounds[0] = -0.1;
  this->TimeBounds[1] = 1.1;
  this->AnimationCueProxy = NULL;
  this->Name = NULL;
  this->AnimationScene = 0;
  this->Duration = 1.0;
  this->TimeChangeable = 1;
  this->BlankTimeEntry = 0;
  this->BlockUpdates = 0;
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame::~vtkPVKeyFrame()
{
  this->Observer->SetTarget(NULL);
  this->Observer->Delete();
  this->SetKeyFrameProxy(0);
  this->SetKeyFrameProxyName(0);

  this->SetAnimationCueProxy(NULL);
  this->SetKeyFrameProxyXMLName(0);

  this->TimeLabel->Delete();
  this->TimeThumbWheel->Delete();
  this->SetName(NULL);
  this->SetAnimationScene(0);
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::DetermineKeyFrameProxyName()
{
  static int proxyNum = 0;
  ostrstream str;
  str << "vtkPVKeyFrame_" << this->KeyFrameProxyXMLName << proxyNum << ends;
  this->SetKeyFrameProxyName(str.str());
  str.rdbuf()->freeze(0);
  proxyNum++;
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::CreateWidget()
{
  if (!this->KeyFrameProxyXMLName)
    {
    vtkErrorMacro("KeyFrameProxyXMLName must be set before calling Create");
    return;
    }

  if (!this->AnimationCueProxy)
    {
    vtkErrorMacro("AnimationCueProxy must be set before calling Create");
    return;
    }

  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
  this->ChildCreate();

  if (!this->KeyFrameProxy)
    {
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    vtkSMKeyFrameProxy* kf  = vtkSMKeyFrameProxy::SafeDownCast(
      pxm->NewProxy("animation_keyframes", this->KeyFrameProxyXMLName));
    this->SetKeyFrameProxy(kf);
    kf->Delete();
    }
  
  if (!this->KeyFrameProxy)
    {
    vtkErrorMacro("Failed to create proxy " << this->KeyFrameProxyXMLName);
    return;
    }
  
  this->KeyFrameProxy->UpdateVTKObjects(); // creates the proxy.
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SetKeyFrameProxy(vtkSMKeyFrameProxy* kf)
{
  if (this->KeyFrameProxy == kf)
    {
    return;
    }
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  
  if (this->KeyFrameProxy)
    {
    this->KeyFrameProxy->RemoveObserver(this->Observer);
    pxm->UnRegisterProxy("animation_keyframes", this->KeyFrameProxyName);
    }
  
  vtkSetObjectBodyMacro(KeyFrameProxy, vtkSMKeyFrameProxy, kf);

  if (this->KeyFrameProxy)
    {
    pxm->RegisterProxy("animation_keyframes", 
      this->KeyFrameProxyName, this->KeyFrameProxy);
    this->KeyFrameProxy->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    this->UpdateValuesFromProxy();
    }
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::ChildCreate()
{
  this->TimeLabel->SetParent(this);
  this->TimeLabel->Create();
  this->TimeLabel->SetText("Time:");
  
  this->TimeThumbWheel->SetParent(this);
  this->TimeThumbWheel->PopupModeOn();
  this->TimeThumbWheel->SetResolution(0.01);
  this->TimeThumbWheel->Create();
  this->TimeThumbWheel->DisplayEntryOn();
  this->TimeThumbWheel->DisplayLabelOff();
  this->TimeThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->TimeThumbWheel->ExpandEntryOn();
  this->TimeThumbWheel->SetEntryCommand(this, "TimeChangedCallback");
  this->TimeThumbWheel->SetEndCommand(this, "TimeChangedCallback");

  this->Script("grid %s %s x x x -sticky w",
    this->TimeLabel->GetWidgetName(),
    this->TimeThumbWheel->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0",
    this->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 0",
    this->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 0",
    this->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 0",
    this->GetWidgetName());
  this->Script("grid columnconfigure %s 4 -weight 2",
    this->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::PrepareForDisplay()
{
  this->UpdateValuesFromProxy();
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::UpdateValuesFromProxy()
{
  if (this->BlankTimeEntry && !this->TimeChangeable)
    {
    this->TimeThumbWheel->GetEntry()->SetValue("");
    }
  else
    {
    this->TimeThumbWheel->SetValue(
      this->GetRelativeTime(this->KeyFrameProxy->GetKeyTime()));
    }
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::TimeChangedCallback(double value)
{
  double ntime = this->GetNormalizedTime(value);

  if (ntime < 0 || ntime <= this->TimeBounds[0] ||
    ntime > 1 || ntime >= this->TimeBounds[1])
    {
    // invalid change.
    this->TimeThumbWheel->SetValue(
      this->GetRelativeTime(this->KeyFrameProxy->GetKeyTime()));
    }
  else
    {
    this->SetKeyTimeWithTrace(ntime);
    }
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SetDuration(double duration)
{
  if (duration != this->Duration)
    {
    this->Duration = duration;
    this->Modified();
    }
  if (this->TimeThumbWheel && this->TimeThumbWheel->GetEntry())
    {
    double ntime = this->GetNormalizedTime(
      this->TimeThumbWheel->GetEntry()->GetValueAsDouble());
    this->SetKeyTime(ntime);
    }
}

//-----------------------------------------------------------------------------
double vtkPVKeyFrame::GetNormalizedTime(double rtime)
{
  double duration = this->Duration;
  if (this->AnimationScene)
    {
    duration = this->AnimationScene->GetDuration();
    }
  if (duration == 0)
    {
    vtkErrorMacro("Scene durantion is 0");
    return 0;
    }
  return rtime / duration;
}

//-----------------------------------------------------------------------------
double vtkPVKeyFrame::GetRelativeTime(double ntime)
{
  double duration = this->Duration;
  if (this->AnimationScene)
    {
    duration = this->AnimationScene->GetDuration();
    }
  return duration*ntime;
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::ExecuteEvent(vtkObject* , unsigned long event, void* )
{
  switch(event)
    {
  case vtkCommand::ModifiedEvent:
    if (!this->BlockUpdates)
      {
      this->UpdateValuesFromProxy();
      }
    break;
    }
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::ClearTimeBounds()
{
  this->TimeBounds[0] = -0.1;
  this->TimeBounds[1] = 1.1;
  this->TimeThumbWheel->ClampMinimumValueOff();
  this->TimeThumbWheel->ClampMaximumValueOff();
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SetTimeMinimumBound(double min)
{
  this->TimeBounds[0] = min;
  this->TimeThumbWheel->SetMinimumValue(this->GetRelativeTime(min));
  this->TimeThumbWheel->ClampMinimumValueOn();
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SetTimeMaximumBound(double max)
{
  this->TimeBounds[1] = max;
  this->TimeThumbWheel->SetMaximumValue(this->GetRelativeTime(max));
  this->TimeThumbWheel->ClampMaximumValueOn();
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SetKeyTimeWithTrace(double time)
{
  this->SetKeyTime(time);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetKeyTimeWithTrace %f", 
    this->GetTclName(), time);
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SetKeyTime(double time)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  DoubleVectPropertySetElement(this->KeyFrameProxy, "KeyTime", time);
  this->KeyFrameProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
double vtkPVKeyFrame::GetKeyTime()
{
  if (!this->KeyFrameProxy)
    {
    return 0;
    }
  return this->KeyFrameProxy->GetKeyTime();
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::Copy(vtkPVKeyFrame* fromKF)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  this->SetKeyTime(fromKF->GetKeyTime());
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  if (this->TimeChangeable)
    {
    this->PropagateEnableState(this->TimeThumbWheel);
    }
  else
    {
    this->TimeThumbWheel->SetEnabled(0);
    }
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::SaveState(ofstream* file)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  *file << "#State of a Key Frame " <<endl;
  *file << "$kw(" << this->GetTclName() << ") SetKeyTime "
    << this->GetKeyTime() << endl;
}

//-----------------------------------------------------------------------------
void vtkPVKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << ((this->Name)? this->Name : NULL) << endl;
  os << indent << "KeyFrameProxyXMLName: " << (this->KeyFrameProxyXMLName?
    this->KeyFrameProxyXMLName : "NULL") << endl;
  os << indent << "KeyFrameProxyName: " << (this->KeyFrameProxyName?
    this->KeyFrameProxyName : "NULL") << endl;
  os << indent << "KeyFrameProxy: " << this->KeyFrameProxy << endl;
  os << indent << "AnimationCueProxy: " << this->AnimationCueProxy << endl;
  os << indent << "AnimationScene: " << this->AnimationScene << endl;
  os << indent << "Duration: " << this->Duration << endl;
  os << indent;
  if(this->TimeChangeable)
    {
    os << "True";
    }
  else
    {
    os << "False";
    }
  os << endl;
  os << indent << this->BlankTimeEntry << endl;
}

