/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraKeyFrame.h"

#include "vtkCamera.h"
#include "vtkKWApplication.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWThumbWheel.h"
#include "vtkObjectFactory.h"
#include "vtkPVTraceHelper.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMCameraKeyFrameProxy.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkPVCameraKeyFrame);
vtkCxxRevisionMacro(vtkPVCameraKeyFrame, "1.5");
//------------------------------------------------------------------------------
vtkPVCameraKeyFrame::vtkPVCameraKeyFrame()
{
  this->PositionLabel = vtkKWLabel::New();
  this->FocalPointLabel = vtkKWLabel::New();
  this->ViewUpLabel = vtkKWLabel::New();
  this->ViewAngleLabel = vtkKWLabel::New();
  this->CaptureCurrentCamera = vtkKWPushButton::New();

  for (int i=0; i < 3; i++)
    {
    this->PositionWheels[i] = vtkKWThumbWheel::New();
    this->FocalPointWheels[i] = vtkKWThumbWheel::New();
    this->ViewUpWheels[i] = vtkKWThumbWheel::New();
    }
  this->ViewAngleWheel = vtkKWThumbWheel::New();

  this->SetKeyFrameProxyXMLName("CameraKeyFrame");
  this->DetermineKeyFrameProxyName();
}

//------------------------------------------------------------------------------
vtkPVCameraKeyFrame::~vtkPVCameraKeyFrame()
{
  this->PositionLabel->Delete();
  this->FocalPointLabel->Delete();
  this->ViewUpLabel->Delete();
  this->ViewAngleLabel->Delete();
  this->CaptureCurrentCamera->Delete();

  for (int i=0; i < 3; i++)
    {
    this->PositionWheels[i]->Delete();
    this->FocalPointWheels[i]->Delete();
    this->ViewUpWheels[i]->Delete();
    }
  this->ViewAngleWheel->Delete();
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::ChildCreate()
{
  this->Superclass::ChildCreate();

  this->PositionLabel->SetParent(this);
  this->PositionLabel->Create();
  this->PositionLabel->SetText("Position:");
  this->FocalPointLabel->SetParent(this);
  this->FocalPointLabel->Create();
  this->FocalPointLabel->SetText("Focal Point:");
  this->ViewUpLabel->SetParent(this);
  this->ViewUpLabel->Create();
  this->ViewUpLabel->SetText("View Up:");
  this->ViewAngleLabel->SetParent(this);
  this->ViewAngleLabel->Create();
  this->ViewAngleLabel->SetText("View Angle:");

  for (int i=0; i < 3; i++)
    {
    this->PositionWheels[i]->SetParent(this);
    this->PositionWheels[i]->PopupModeOn();
    this->PositionWheels[i]->SetResolution(0.01);
    this->PositionWheels[i]->Create();
    this->PositionWheels[i]->DisplayEntryOn();
    this->PositionWheels[i]->DisplayLabelOff();
    this->PositionWheels[i]->DisplayEntryAndLabelOnTopOff();
    this->PositionWheels[i]->ExpandEntryOn();
    this->PositionWheels[i]->SetEntryCommand(this, "PositionChangedCallback");
    this->PositionWheels[i]->SetEndCommand(this, "PositionChangedCallback");

    this->FocalPointWheels[i]->SetParent(this);
    this->FocalPointWheels[i]->PopupModeOn();
    this->FocalPointWheels[i]->SetResolution(0.01);
    this->FocalPointWheels[i]->Create();
    this->FocalPointWheels[i]->DisplayEntryOn();
    this->FocalPointWheels[i]->DisplayLabelOff();
    this->FocalPointWheels[i]->DisplayEntryAndLabelOnTopOff();
    this->FocalPointWheels[i]->ExpandEntryOn();
    this->FocalPointWheels[i]->SetEntryCommand(this, "FocalPointChangedCallback");
    this->FocalPointWheels[i]->SetEndCommand(this, "FocalPointChangedCallback");

    this->ViewUpWheels[i]->SetParent(this);
    this->ViewUpWheels[i]->PopupModeOn();
    this->ViewUpWheels[i]->SetResolution(0.01);
    this->ViewUpWheels[i]->Create();
    this->ViewUpWheels[i]->DisplayEntryOn();
    this->ViewUpWheels[i]->DisplayLabelOff();
    this->ViewUpWheels[i]->DisplayEntryAndLabelOnTopOff();
    this->ViewUpWheels[i]->ExpandEntryOn();
    this->ViewUpWheels[i]->SetEntryCommand(this, "ViewUpChangedCallback");
    this->ViewUpWheels[i]->SetEndCommand(this, "ViewUpChangedCallback");
    }
  
  this->ViewAngleWheel->SetParent(this);
  this->ViewAngleWheel->PopupModeOn();
  this->ViewAngleWheel->SetMinimumValue(0.00000001);
  this->ViewAngleWheel->SetMaximumValue(179);
  this->ViewAngleWheel->SetResolution(0.01);
  this->ViewAngleWheel->Create();
  this->ViewAngleWheel->DisplayEntryOn();
  this->ViewAngleWheel->DisplayLabelOff();
  this->ViewAngleWheel->DisplayEntryAndLabelOnTopOff();
  this->ViewAngleWheel->ExpandEntryOn();
  this->ViewAngleWheel->SetEntryCommand(this, "ViewAngleChangedCallback");
  this->ViewAngleWheel->SetEndCommand(this, "ViewAngleChangedCallback");

  this->CaptureCurrentCamera->SetParent(this);
  this->CaptureCurrentCamera->Create();
  this->CaptureCurrentCamera->SetText("Capture");
  this->CaptureCurrentCamera->SetBalloonHelpString(
    "Capture the current camera properties");
  this->CaptureCurrentCamera->SetCommand(this, "CaptureCurrentCameraCallback");

  this->Script("grid %s %s %s %s x -sticky w", 
    this->PositionLabel->GetWidgetName(), 
    this->PositionWheels[0]->GetWidgetName(),
    this->PositionWheels[1]->GetWidgetName(),
    this->PositionWheels[2]->GetWidgetName());
  this->Script("grid %s %s %s %s x -sticky w", 
    this->FocalPointLabel->GetWidgetName(),
    this->FocalPointWheels[0]->GetWidgetName(),
    this->FocalPointWheels[1]->GetWidgetName(),
    this->FocalPointWheels[2]->GetWidgetName());
  this->Script("grid %s %s %s %s x -sticky w", 
    this->ViewUpLabel->GetWidgetName(),
    this->ViewUpWheels[0]->GetWidgetName(),
    this->ViewUpWheels[1]->GetWidgetName(),
    this->ViewUpWheels[2]->GetWidgetName());
  this->Script("grid %s %s x x x -sticky w", 
    this->ViewAngleLabel->GetWidgetName(),
    this->ViewAngleWheel->GetWidgetName());
  this->Script("grid x %s x x x -sticky w",
    this->CaptureCurrentCamera->GetWidgetName());
  
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetKeyValue(vtkSMProxy* cameraProxy)
{
  if (!cameraProxy)
    {
    vtkErrorMacro("Keyframe value cannot be set to NULL.");
    return;
    }
  vtkSMDoubleVectorProperty* sdvp;
  cameraProxy->UpdatePropertyInformation();

  const char* names[] = { "Position", "FocalPoint", "ViewUp", "ViewAngle",  0 };
  const char* snames[] = { "CameraPositionInfo", "CameraFocalPointInfo", 
    "CameraViewUpInfo",  "CameraViewAngleInfo", 0 };
  for (int i=0; names[i] && snames[i]; i++)
    {
    sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
      cameraProxy->GetProperty(snames[i]));
    if (!sdvp)
      {
      vtkErrorMacro("Failed to find property " << snames[i]);
      continue;
      }
    this->SetProperty(names[i], sdvp);
    }
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetProperty(const char* name, 
  vtkSMDoubleVectorProperty* sdvp)
{
  this->SetProperty(name, sdvp->GetElements());
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetPositionWithTrace(double x, double y, double z)
{
  this->SetPosition(x, y, z);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetPositionWithTrace %f %f %f",
    this->GetTclName(), x, y, z);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetPosition(double x, double y, double z)
{
  this->SetProperty("Position", x, y, z);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetFocalPointWithTrace(double x, double y, double z)
{
  this->SetFocalPoint(x, y, z);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetFocalPointWithTrace %f %f %f",
    this->GetTclName(), x, y, z);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetFocalPoint(double x, double y, double z)
{
  this->SetProperty("FocalPoint", x, y, z);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetViewUpWithTrace(double x, double y, double z)
{
  this->SetViewUp(x, y, z);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetViewUpWithTrace %f %f %f",
    this->GetTclName(), x, y, z);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetViewUp(double x, double y, double z)
{
  this->SetProperty("ViewUp", x,y,z);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetViewAngleWithTrace(double a)
{
  this->SetViewAngle(a);
  this->GetTraceHelper()->AddEntry("$kw(%s) SetViewAngleWithTrace %f",
    this->GetTclName(), a);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetViewAngle(double angle)
{
  this->SetProperty("ViewAngle", angle);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetProperty(const char* name, double val)
{
  this->SetProperty(name, &val);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetProperty(const char* name, double x, double y, 
  double z)
{
  double val[3];
  val[0] = x;
  val[1] = y;
  val[2] = z;
  this->SetProperty(name, val);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SetProperty(const char* name, const double* data)
{
  if (!this->KeyFrameProxy)
    {
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty(name));
  if (!dvp)
    {
    vtkErrorMacro("Failed to find property " << name);
    return;
    }
  dvp->SetElements(data);
  this->KeyFrameProxy->UpdateVTKObjects();
  this->UpdateValuesFromProxy();
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::InitializeKeyValueUsingCurrentState()
{
  if (!this->AnimationCueProxy)
    {
    vtkErrorMacro("AnimationCueProxy must be set.");
    return;
    }
  
  vtkSMProxy* proxy = this->AnimationCueProxy->GetAnimatedProxy();
  this->SetKeyValue(proxy);
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::UpdateValuesFromProxy()
{
  this->Superclass::UpdateValuesFromProxy();

  vtkSMCameraKeyFrameProxy* kf = vtkSMCameraKeyFrameProxy::SafeDownCast(
    this->KeyFrameProxy);
  if (!kf)
    {
    vtkErrorMacro("Invalid internal proxy. Must be vtkSMCameraKeyFrameProxy.");
    return;
    }

  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty("Position"));
  
  if (dvp)
    {
    double* value = dvp->GetElements();
    this->PositionWheels[0]->SetValue(value[0]);
    this->PositionWheels[1]->SetValue(value[1]);
    this->PositionWheels[2]->SetValue(value[2]);
    }
  else
    {
    vtkErrorMacro("Failed to find property Position.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty("FocalPoint"));
  if (dvp)
    {
    double* value = dvp->GetElements();
    this->FocalPointWheels[0]->SetValue(value[0]);
    this->FocalPointWheels[1]->SetValue(value[1]);
    this->FocalPointWheels[2]->SetValue(value[2]);
    }
  else
    {
    vtkErrorMacro("Failed to find property FocalPoint.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty("ViewUp"));
  if (dvp)
    {
    double* value = dvp->GetElements();
    this->ViewUpWheels[0]->SetValue(value[0]);
    this->ViewUpWheels[1]->SetValue(value[1]);
    this->ViewUpWheels[2]->SetValue(value[2]);
    }
  else
    {
    vtkErrorMacro("Failed to find property ViewUp.");
    }

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->KeyFrameProxy->GetProperty("ViewAngle"));
  if (dvp)
    {
    this->ViewAngleWheel->SetValue(dvp->GetElement(0));
    }
  else
    {
    vtkErrorMacro("Failed to find property ViewAngle.");
    }
    
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::CaptureCurrentCameraCallback()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) CaptureCurrentCameraCallback",
    this->GetTclName());
  this->InitializeKeyValueUsingCurrentState();
  this->UpdateValuesFromProxy();
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::PositionChangedCallback()
{
  this->SetPositionWithTrace(this->PositionWheels[0]->GetValue(),
    this->PositionWheels[1]->GetValue(), this->PositionWheels[2]->GetValue());
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::FocalPointChangedCallback()
{
  this->SetFocalPointWithTrace(this->FocalPointWheels[0]->GetValue(),
    this->FocalPointWheels[1]->GetValue(),
    this->FocalPointWheels[2]->GetValue());
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::ViewUpChangedCallback()
{
  this->SetViewUpWithTrace(this->ViewUpWheels[0]->GetValue(),
    this->ViewUpWheels[1]->GetValue(),
    this->ViewUpWheels[2]->GetValue());
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::ViewAngleChangedCallback()
{
  this->SetViewAngleWithTrace(this->ViewAngleWheel->GetValue());
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);
  int i;
  
  *file << "$kw(" << this->GetTclName() << ") SetPosition";
  for (i=0; i < 3; i++)
    {
    *file << " " << this->PositionWheels[i]->GetValue();
    }
  *file << endl;

  *file << "$kw(" << this->GetTclName() << ") SetFocalPoint";
  for (i=0; i < 3; i++)
    {
    *file << " " << this->FocalPointWheels[i]->GetValue();
    }
  *file << endl;

  *file << "$kw(" << this->GetTclName() << ") SetViewUp";
  for (i=0; i < 3; i++)
    {
    *file << " " << this->ViewUpWheels[i]->GetValue();
    }
  *file << endl;

  *file << "$kw(" << this->GetTclName() << ") SetViewAngle "
    << this->ViewAngleWheel->GetValue() << endl;
}

//------------------------------------------------------------------------------
void vtkPVCameraKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

