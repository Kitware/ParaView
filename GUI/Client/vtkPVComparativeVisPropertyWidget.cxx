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
#include "vtkPVKeyFrame.h"
#include "vtkPVSimpleAnimationCue.h"
#include "vtkPVSource.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVWindow.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMComparativeVisProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisPropertyWidget );
vtkCxxRevisionMacro(vtkPVComparativeVisPropertyWidget, "1.14");

vtkCxxSetObjectMacro(vtkPVComparativeVisPropertyWidget, TrackEditor, vtkPVTrackEditor);

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

  this->CueEditor = 0;
  this->LastCue = 0;
  this->TrackEditor = 0;
}

//----------------------------------------------------------------------------
vtkPVComparativeVisPropertyWidget::~vtkPVComparativeVisPropertyWidget()
{
  this->TrackSelector->Delete();
  this->NumberOfFramesEntry->Delete();
  if (this->CueEditor)
    {
    this->CueEditor->Delete();
    }
  this->SetTrackEditor(0);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget
  this->Superclass::CreateWidget();

  vtkPVApplication* pvApp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 

  this->TrackSelector->SetParent(this);
  this->TrackSelector->SetPackHorizontally(1);
  this->TrackSelector->Create();
  this->TrackSelector->ShallowCopy(pvAM->GetActiveTrackSelector(), 1);
  this->TrackSelector->SetFocusCurrentCue(0);
  this->TrackSelector->GetSourceMenuButton()->SetWidth(15);
  this->TrackSelector->GetPropertyMenuButton()->SetWidth(20);
  this->Script("pack %s -side left", this->TrackSelector->GetWidgetName());

  this->NumberOfFramesEntry->SetParent(this);
  this->NumberOfFramesEntry->Create();
  this->NumberOfFramesEntry->GetWidget()->SetValueAsInt(5);
  this->NumberOfFramesEntry->GetWidget()->SetWidth(3);
  this->NumberOfFramesEntry->SetLabelText("Number of Frames:");;
  this->Script("pack %s -side left", 
               this->NumberOfFramesEntry->GetWidgetName());

  this->CueEditor = vtkPVSimpleAnimationCue::New();
  this->CueEditor->SetDuration(4);
  this->CueEditor->SetKeyFrameParent(this->TrackEditor->GetPropertiesFrame());
  this->CueEditor->SetApplication(pvApp);
  this->CueEditor->Create();
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
  vtkSMComparativeVisProxy* cv)
{
  if (this->CueEditor && this->LastCue)
    {
    int numFrames = 1;
    int value = this->NumberOfFramesEntry->GetWidget()->GetValueAsInt();
    if (value > 0)
      {
      numFrames = value;
      }
    this->CueEditor->SetDuration(numFrames-1);
    int numKeyFrames = this->CueEditor->GetNumberOfKeyFrames();
    vtkPVKeyFrame* keyFrame = this->CueEditor->GetKeyFrame(
      numKeyFrames-1);
    if (keyFrame)
      {
      // The last key frame is always at the end.
      // We have to set the normalized time.
      keyFrame->SetKeyTime(1.0);
      }
 
    vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
    
    vtkSMAnimationCueProxy* acp = vtkSMAnimationCueProxy::SafeDownCast(
      pm->NewProxy("animation", "AnimationCue"));
    acp->CloneCopy(this->CueEditor->GetCueProxy());
    vtkSMProxyProperty::SafeDownCast(
      cv->GetProperty("Cues"))->AddProxy(
        this->CueEditor->GetCueProxy());
    acp->Delete();

    vtkSMIntVectorProperty* numProps = vtkSMIntVectorProperty::SafeDownCast(
      cv->GetProperty("NumberOfFramesInCue"));
    numProps->SetElement(numProps->GetNumberOfElements(), numFrames);

    vtkSMStringVectorProperty* sourceNames = 
      vtkSMStringVectorProperty::SafeDownCast(cv->GetProperty("SourceNames"));
    vtkPVSource* source = this->LastCue->GetPVSource();
    if (source)
      {
      sourceNames->SetElement(sourceNames->GetNumberOfElements(), 
                              source->GetName());
      }
    else
      {
      sourceNames->SetElement(sourceNames->GetNumberOfElements(), 0);
      }

    vtkSMStringVectorProperty* sourceTclNames = 
      vtkSMStringVectorProperty::SafeDownCast(cv->GetProperty("SourceTclNames"));
    if (source)
      {
      sourceTclNames->SetElement(sourceTclNames->GetNumberOfElements(), 
                                 source->GetTclName());
      }
    else
      {
      sourceTclNames->SetElement(sourceTclNames->GetNumberOfElements(), 0);
      }
    cv->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::CopyFromVisualization(
  unsigned int propIdx, vtkSMComparativeVisProxy* proxy)

{
  if (this->TrackSelector->SelectCue(proxy->GetSourceName(propIdx), 
                                     vtkSMAnimationCueProxy::SafeDownCast(
                                       proxy->GetCue(propIdx))))
    {
    this->LastCue = this->TrackSelector->GetCurrentCue();
    }
  else
    {
    this->LastCue = 0;
    }
  this->NumberOfFramesEntry->GetWidget()->SetValueAsInt(
    static_cast<int>(proxy->GetNumberOfFramesInCue(propIdx)));

  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  vtkSMAnimationCueProxy* acp = vtkSMAnimationCueProxy::SafeDownCast(
    pm->NewProxy("animation", "AnimationCue"));
  acp->CloneCopy(proxy->GetCue(propIdx));
  this->CueEditor->SetCueProxy(acp);
  acp->Delete();

  this->TrackEditor->SetAnimationCue(0);
  this->TrackEditor->SetAnimationCue(this->CueEditor);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::ShowCueEditor()
{
  vtkPVAnimationCue* selectedCue = this->TrackSelector->GetCurrentCue();
  if (selectedCue)
    {
    // Store the cue selected by the track selector. This cue is actually
    // the one in the ParaView animation track list. Therefore, we also
    // make a copy and use that in the comparative vis. This is so because
    // we do not want to modify a cue in the ParaView animation editor.
    if (this->LastCue != selectedCue)
      {
      this->LastCue = selectedCue;

      this->CueEditor->RemoveAllKeyFrames();

      this->CueEditor->SetAnimatedProxy(
        this->LastCue->GetAnimatedProxy());
      this->CueEditor->SetAnimatedPropertyName(
        this->LastCue->GetAnimatedPropertyName());
      this->CueEditor->SetAnimatedDomainName(
        this->LastCue->GetAnimatedDomainName());
      this->CueEditor->SetAnimatedElement(
        this->LastCue->GetAnimatedElement());

      // Create 2 default keyframes.
      this->CueEditor->AppendNewKeyFrame();      
      }
    this->TrackEditor->SetAnimationCue(this->CueEditor);
    this->TrackEditor->GetTitleLabel()->SetText(
      selectedCue->GetTextRepresentation());
    }
  else
    {
    this->TrackEditor->SetAnimationCue(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisPropertyWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TrackEditor: ";
  if (this->TrackEditor)
    {
    this->TrackEditor->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

