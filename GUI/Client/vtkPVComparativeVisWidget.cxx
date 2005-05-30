/*=========================================================================

Module:    vtkPVComparativeVisWidget.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVComparativeVisWidget.h"

#include "vtkEventForwarderCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVActiveTrackSelector.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVis.h"
#include "vtkPVSimpleAnimationCue.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVWindow.h"
#include "vtkSMAnimationCueProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisWidget );
vtkCxxRevisionMacro(vtkPVComparativeVisWidget, "1.2");

int vtkPVComparativeVisWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVComparativeVisWidget::vtkPVComparativeVisWidget()
{
  this->CommandFunction = vtkPVComparativeVisWidgetCommand;

  this->TrackSelector = vtkPVActiveTrackSelector::New();
  this->NumberOfFramesEntry = vtkKWEntry::New();

  vtkEventForwarderCommand* ef = vtkEventForwarderCommand::New();
  ef->SetTarget(this);
  this->TrackSelector->AddObserver(vtkCommand::WidgetModifiedEvent, ef);
  ef->Delete();

  this->LastCueEditor = 0;
  this->LastCue = 0;
}

//----------------------------------------------------------------------------
vtkPVComparativeVisWidget::~vtkPVComparativeVisWidget()
{
  this->TrackSelector->Delete();
  this->NumberOfFramesEntry->Delete();
  if (this->LastCueEditor)
    {
    this->LastCueEditor->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisWidget::Create(vtkKWApplication *app, const char *)
{
  if (!this->Superclass::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  this->TrackSelector->SetParent(this);
  this->TrackSelector->SetPackHorizontally(1);
  this->TrackSelector->Create(app, "");
  this->TrackSelector->ShallowCopy(pvAM->GetActiveTrackSelector());
  this->TrackSelector->SetFocusCurrentCue(0);
  this->TrackSelector->GetSourceMenuButton()->SetWidth(15);
  this->TrackSelector->GetPropertyMenuButton()->SetWidth(20);
  this->Script("pack %s -side left", this->TrackSelector->GetWidgetName());

  this->NumberOfFramesEntry->SetParent(this);
  this->NumberOfFramesEntry->Create(app, "");
  this->NumberOfFramesEntry->SetValue(5);
  this->Script("pack %s -side left", this->NumberOfFramesEntry->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisWidget::CopyToVisualization(vtkPVComparativeVis* cv)
{
  if (this->LastCueEditor)
    {
    int numFrames = 1;
    if (this->NumberOfFramesEntry->GetValueAsInt() > 0)
      {
      numFrames = this->NumberOfFramesEntry->GetValueAsInt();
      }
    this->LastCueEditor->SetDuration(numFrames);
    cv->AddProperty(
      this->LastCue, this->LastCueEditor->GetCueProxy(), numFrames);  
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisWidget::CopyFromVisualization(vtkPVAnimationCue* cue)
{
  this->TrackSelector->SelectCue(cue);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisWidget::ShowCueEditor(vtkPVTrackEditor* trackE)
{
  vtkPVAnimationCue* selectedCue = this->TrackSelector->GetCurrentCue();
  if (selectedCue)
    {
    if (this->LastCue != selectedCue || !this->LastCueEditor)
      {
      if (this->LastCueEditor)
        {
        this->LastCueEditor->Delete();
        this->LastCueEditor = 0;
        }
      this->LastCue = selectedCue;
      this->LastCueEditor = vtkPVSimpleAnimationCue::New();
      this->LastCueEditor->SetDuration(5);
      this->LastCueEditor->SetKeyFrameParent(trackE->GetPropertiesFrame());
      this->LastCueEditor->CreateWidget(this->GetApplication(), 0, 0);

      this->LastCueEditor->SetAnimatedProxy(
        this->LastCue->GetAnimatedProxy());
      this->LastCueEditor->SetAnimatedPropertyName(
        this->LastCue->GetAnimatedPropertyName());
      this->LastCueEditor->SetAnimatedDomainName(
        this->LastCue->GetAnimatedDomainName());
      this->LastCueEditor->SetAnimatedElement(
        this->LastCue->GetAnimatedElement());

      this->LastCueEditor->AppendNewKeyFrame();
      }
    trackE->SetAnimationCue(this->LastCueEditor);
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

