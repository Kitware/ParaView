/*=========================================================================

Module:    vtkPVComparativeVisPropertyWidget.cxx

Copyright (c) Kitware, Inc.
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPVComparativeVisPropertyWidget.h"

#include "vtkEventForwarderCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryLabeled.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
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
vtkStandardNewMacro( vtkPVComparativeVisPropertyWidget );
vtkCxxRevisionMacro(vtkPVComparativeVisPropertyWidget, "1.1");

int vtkPVComparativeVisPropertyWidgetCommand(ClientData cd, Tcl_Interp *interp,
                                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVComparativeVisPropertyWidget::vtkPVComparativeVisPropertyWidget()
{
  this->CommandFunction = vtkPVComparativeVisPropertyWidgetCommand;

  this->TrackSelector = vtkPVActiveTrackSelector::New();
  this->NumberOfFramesEntry = vtkKWEntryLabeled::New();

  // Forward the WidgetModifiedEvent the track editor signals.
  vtkEventForwarderCommand* ef = vtkEventForwarderCommand::New();
  ef->SetTarget(this);
  this->TrackSelector->AddObserver(vtkCommand::WidgetModifiedEvent, ef);
  ef->Delete();

  this->LastCueEditor = 0;
  this->LastCue = 0;
}

//----------------------------------------------------------------------------
vtkPVComparativeVisPropertyWidget::~vtkPVComparativeVisPropertyWidget()
{
  this->TrackSelector->Delete();
  this->NumberOfFramesEntry->Delete();
  if (this->LastCueEditor)
    {
    this->LastCueEditor->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::Create(
  vtkKWApplication *app, const char *)
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
  this->NumberOfFramesEntry->GetWidget()->SetValue(5);
  this->NumberOfFramesEntry->GetWidget()->SetWidth(3);
  this->NumberOfFramesEntry->SetLabelText("Number of Frames:");;
  this->Script("pack %s -side left", 
               this->NumberOfFramesEntry->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::CopyToVisualization(
  vtkPVComparativeVis* cv)
{
  if (this->LastCueEditor)
    {
    int numFrames = 1;
    int value = this->NumberOfFramesEntry->GetWidget()->GetValueAsInt();
    if (value > 0)
      {
      numFrames = value;
      }
    this->LastCueEditor->SetDuration(numFrames);
    cv->AddProperty(
      this->LastCue, this->LastCueEditor, numFrames);  
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::CopyFromVisualization(
  vtkPVAnimationCue* acue, vtkPVSimpleAnimationCue* cue, unsigned int numValues)
{
  this->TrackSelector->SelectCue(acue);
  this->LastCue = acue;
  this->LastCueEditor = cue;
  cue->Register(this);
  this->NumberOfFramesEntry->GetWidget()->SetValue(static_cast<int>(numValues));
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::ShowCueEditor(vtkPVTrackEditor* trackE)
{
  vtkPVAnimationCue* selectedCue = this->TrackSelector->GetCurrentCue();
  if (selectedCue)
    {
    // Store the cue selected by the track selector. This cue is actually
    // the one in the ParaView animation track list. Therefore, we also
    // make a copy and use that in the comparative vis. This is so because
    // we do not want to modify a cue in the ParaView animation editor.
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

      // Create 2 default keyframes.
      this->LastCueEditor->AppendNewKeyFrame();      
      }
    trackE->SetAnimationCue(this->LastCueEditor);
    trackE->GetTitleLabel()->SetText(selectedCue->GetTextRepresentation());
    }
  else
    {
    trackE->SetAnimationCue(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

