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
#include "vtkKWEntryWithLabel.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVActiveTrackSelector.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVis.h"
#include "vtkPVKeyFrame.h"
#include "vtkPVSimpleAnimationCue.h"
#include "vtkPVSource.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVWindow.h"
#include "vtkSMAnimationCueProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisPropertyWidget );
vtkCxxRevisionMacro(vtkPVComparativeVisPropertyWidget, "1.7");

//----------------------------------------------------------------------------
vtkPVComparativeVisPropertyWidget::vtkPVComparativeVisPropertyWidget()
{
  this->TrackSelector = vtkPVActiveTrackSelector::New();
  this->NumberOfFramesEntry = vtkKWEntryWithLabel::New();

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
void vtkPVComparativeVisPropertyWidget::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  this->TrackSelector->SetParent(this);
  this->TrackSelector->SetPackHorizontally(1);
  this->TrackSelector->Create(app);
  this->TrackSelector->ShallowCopy(pvAM->GetActiveTrackSelector());
  this->TrackSelector->SetFocusCurrentCue(0);
  this->TrackSelector->GetSourceMenuButton()->SetWidth(15);
  this->TrackSelector->GetPropertyMenuButton()->SetWidth(20);
  this->Script("pack %s -side left", this->TrackSelector->GetWidgetName());

  this->NumberOfFramesEntry->SetParent(this);
  this->NumberOfFramesEntry->Create(app);
  this->NumberOfFramesEntry->GetWidget()->SetValue(5);
  this->NumberOfFramesEntry->GetWidget()->SetWidth(3);
  this->NumberOfFramesEntry->SetLabelText("Number of Frames:");;
  this->Script("pack %s -side left", 
               this->NumberOfFramesEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::RemovePVSource(vtkPVSource* source)
{
  if (this->TrackSelector)
    {
    this->TrackSelector->RemoveSource(source);
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::CopyToVisualization(
  vtkPVComparativeVis* cv)
{
  if (this->LastCueEditor && this->LastCue)
    {
    int numFrames = 1;
    int value = this->NumberOfFramesEntry->GetWidget()->GetValueAsInt();
    if (value > 0)
      {
      numFrames = value;
      }
    this->LastCueEditor->SetDuration(numFrames-1);
    int numKeyFrames = this->LastCueEditor->GetNumberOfKeyFrames();
    vtkPVKeyFrame* keyFrame = this->LastCueEditor->GetKeyFrame(
      numKeyFrames-1);
    if (keyFrame)
      {
      // The last key frame is always at the end.
      // We have to set the normalized time.
      keyFrame->SetKeyTime(1.0);
      }
    
    cv->AddProperty(
      this->LastCue, this->LastCueEditor, numFrames);  
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::CopyFromVisualization(
  vtkPVAnimationCue* acue, vtkPVSimpleAnimationCue* cue, unsigned int numValues)
{
  if (this->TrackSelector->SelectCue(acue))
    {
    this->LastCue = acue;
    }
  else
    {
    this->LastCue = 0;
    }
  this->NumberOfFramesEntry->GetWidget()->SetValue(static_cast<int>(numValues));

  if (cue == this->LastCueEditor)
    {
    return;
    }
  if (this->LastCueEditor)
    {
    this->LastCueEditor->Delete();
    this->LastCueEditor = 0;
    }
  this->LastCueEditor = cue;
  cue->Register(this);
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
      this->LastCueEditor->SetDuration(4);
      this->LastCueEditor->SetKeyFrameParent(trackE->GetPropertiesFrame());
      this->LastCueEditor->Create(this->GetApplication());

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

