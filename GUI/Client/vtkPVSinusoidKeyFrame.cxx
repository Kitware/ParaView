/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSinusoidKeyFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVSinusoidKeyFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWThumbWheel.h"
#include "vtkSMSinusoidKeyFrameProxy.h"

vtkStandardNewMacro(vtkPVSinusoidKeyFrame);
vtkCxxRevisionMacro(vtkPVSinusoidKeyFrame, "1.2");

//-----------------------------------------------------------------------------
vtkPVSinusoidKeyFrame::vtkPVSinusoidKeyFrame()
{
  this->SetKeyFrameProxyXMLName("SinusoidKeyFrame");
  this->PhaseLabel = vtkKWLabel::New();
  this->FrequencyLabel = vtkKWLabel::New();
  this->OffsetLabel = vtkKWLabel::New();
  this->PhaseThumbWheel = vtkKWThumbWheel::New();
  this->FrequencyThumbWheel = vtkKWThumbWheel::New();
  this->OffsetThumbWheel = vtkKWThumbWheel::New();
}

//-----------------------------------------------------------------------------
vtkPVSinusoidKeyFrame::~vtkPVSinusoidKeyFrame()
{
  this->PhaseLabel->Delete();
  this->FrequencyLabel->Delete();
  this->OffsetLabel->Delete();
  this->PhaseThumbWheel->Delete();
  this->FrequencyThumbWheel->Delete();
  this->OffsetThumbWheel->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::ChildCreate(vtkKWApplication* app)
{
  this->Superclass::ChildCreate(app);

  this->PhaseLabel->SetParent(this);
  this->PhaseLabel->Create(app, 0);
  this->PhaseLabel->SetLabel("Phase:");

  this->PhaseThumbWheel->SetParent(this);
  this->PhaseThumbWheel->PopupModeOn();
  this->PhaseThumbWheel->SetValue(0.0);
  this->PhaseThumbWheel->SetResolution(0.01);
  this->PhaseThumbWheel->Create(app, NULL);
  this->PhaseThumbWheel->DisplayEntryOn();
  this->PhaseThumbWheel->DisplayLabelOff();
  this->PhaseThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->PhaseThumbWheel->ExpandEntryOn();
  this->PhaseThumbWheel->SetBalloonHelpString("Specify the phase of the parameter's"
    " sine waveform in degrees.");
  this->PhaseThumbWheel->GetEntry()->BindCommand(this, "PhaseChangedCallback");
  this->PhaseThumbWheel->SetEndCommand(this, "PhaseChangedCallback");

  this->FrequencyLabel->SetParent(this);
  this->FrequencyLabel->Create(app, 0);
  this->FrequencyLabel->SetLabel("Frequency:");

  this->FrequencyThumbWheel->SetParent(this);
  this->FrequencyThumbWheel->PopupModeOn();
  this->FrequencyThumbWheel->SetValue(0.0);
  this->FrequencyThumbWheel->SetMinimumValue(0);
  this->FrequencyThumbWheel->ClampMinimumValueOn();
  this->FrequencyThumbWheel->SetResolution(0.01);
  this->FrequencyThumbWheel->Create(app, NULL);
  this->FrequencyThumbWheel->DisplayEntryOn();
  this->FrequencyThumbWheel->DisplayLabelOff();
  this->FrequencyThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->FrequencyThumbWheel->ExpandEntryOn();
  this->FrequencyThumbWheel->SetBalloonHelpString("Specify the number of waveform "
    "cycles until the next key frame.");
  this->FrequencyThumbWheel->GetEntry()->BindCommand(this, "FrequencyChangedCallback");
  this->FrequencyThumbWheel->SetEndCommand(this, "FrequencyChangedCallback");

  this->OffsetLabel->SetParent(this);
  this->OffsetLabel->Create(app, 0);
  this->OffsetLabel->SetLabel("Amplitude:");

  this->OffsetThumbWheel->SetParent(this);
  this->OffsetThumbWheel->PopupModeOn();
  this->OffsetThumbWheel->SetValue(0.0);
  this->OffsetThumbWheel->SetMinimumValue(0);
  this->OffsetThumbWheel->ClampMinimumValueOn();
  this->OffsetThumbWheel->SetResolution(0.01);
  this->OffsetThumbWheel->Create(app, NULL);
  this->OffsetThumbWheel->DisplayEntryOn();
  this->OffsetThumbWheel->DisplayLabelOff();
  this->OffsetThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->OffsetThumbWheel->ExpandEntryOn();
  this->OffsetThumbWheel->SetBalloonHelpString("Specify the positive offset for the crest "
    "of the sine waveform.");
  this->OffsetThumbWheel->GetEntry()->BindCommand(this, "OffsetChangedCallback");
  this->OffsetThumbWheel->SetEndCommand(this, "OffsetChangedCallback");

  this->Script("grid %s %s -sticky ew",
    this->PhaseLabel->GetWidgetName(),
    this->PhaseThumbWheel->GetWidgetName());
  this->Script("grid %s %s -sticky ew",
    this->FrequencyLabel->GetWidgetName(),
    this->FrequencyThumbWheel->GetWidgetName());
  this->Script("grid %s %s -sticky ew",
    this->OffsetLabel->GetWidgetName(),
    this->OffsetThumbWheel->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::PhaseChangedCallback()
{
  this->SetPhase(this->PhaseThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::OffsetChangedCallback()
{
  this->SetOffset(this->OffsetThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::FrequencyChangedCallback()
{
  this->SetFrequency(this->FrequencyThumbWheel->GetEntry()->GetValueAsFloat());
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::SetPhase(double base)
{
  vtkSMSinusoidKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    SetPhase(base);
  this->AddTraceEntry("$kw(%s) SetPhase %f", this->GetTclName(), base);
}

//-----------------------------------------------------------------------------
double vtkPVSinusoidKeyFrame::GetPhase()
{
  return vtkSMSinusoidKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)
    ->GetPhase();
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::SetFrequency(double p)
{ 
  vtkSMSinusoidKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    SetFrequency(p);
  this->AddTraceEntry("$kw(%s) SetFrequency %f", this->GetTclName(), p);
}

//-----------------------------------------------------------------------------
double vtkPVSinusoidKeyFrame::GetFrequency()
{
  return vtkSMSinusoidKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    GetFrequency();
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::SetOffset(double p)
{
  vtkSMSinusoidKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    SetOffset(p);
  this->AddTraceEntry("$kw(%s) SetOffset %f", this->GetTclName(), p);
}

//-----------------------------------------------------------------------------
double vtkPVSinusoidKeyFrame::GetOffset()
{
  return vtkSMSinusoidKeyFrameProxy::SafeDownCast(this->KeyFrameProxy)->
    GetOffset();
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::UpdateValuesFromProxy()
{
  this->Superclass::UpdateValuesFromProxy();

  vtkSMSinusoidKeyFrameProxy* proxy = vtkSMSinusoidKeyFrameProxy::
    SafeDownCast(this->KeyFrameProxy);
  this->PhaseThumbWheel->SetValue(proxy->GetPhase());
  this->FrequencyThumbWheel->SetValue(proxy->GetFrequency());
  this->OffsetThumbWheel->SetValue(proxy->GetOffset());
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::SaveState(ofstream* file)
{
  this->Superclass::SaveState(file);
  *file << "$kw(" << this->GetTclName() << ") SetFrequency "
    << this->GetFrequency() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetOffset "
    << this->GetOffset() << endl;
  *file << "$kw(" << this->GetTclName() << ") SetPhase "
    << this->GetPhase() << endl;
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  this->PropagateEnableState(this->PhaseThumbWheel);
  this->PropagateEnableState(this->FrequencyThumbWheel);
  this->PropagateEnableState(this->OffsetThumbWheel);
}

//-----------------------------------------------------------------------------
void vtkPVSinusoidKeyFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
