/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeVisDialog.h"

#include "vtkCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVisPropertyWidget.h"
#include "vtkPVSource.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVWindow.h"
#include "vtkSMComparativeVisProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"

#include <vtkstd/vector>
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisDialog );
vtkCxxRevisionMacro(vtkPVComparativeVisDialog, "1.16");

int vtkPVComparativeVisDialog::NumberOfVisualizationsCreated = 0;
const int vtkPVComparativeVisDialog::DialogWidth = 700;
const int vtkPVComparativeVisDialog::DialogHeight = 600;

// Private implementation
struct vtkPVComparativeVisDialogInternals
{
  // Frames containing the property widgets and radio buttongs
  typedef vtkstd::vector<vtkSmartPointer<vtkKWFrame> > 
      FramesType;
  FramesType PropertyFrames;

  // Property widgets
  typedef vtkstd::vector<vtkSmartPointer<vtkPVComparativeVisPropertyWidget> > 
      WidgetsType;
  WidgetsType Widgets;

  // Radio buttons
  typedef vtkstd::vector<vtkSmartPointer<vtkKWRadioButton> > 
      RadioButtonsType;
  RadioButtonsType RadioButtons;

  typedef vtkstd::vector<vtkSmartPointer<vtkCommand> > ObserversType;
  ObserversType Observers;
};

class vtkPVCVCueSelectionCommand : public vtkCommand
{
public:
  void Execute(vtkObject *caller, unsigned long, void*)
  {
    if (this->Dialog)
      {
      this->Dialog->CueSelected(
        vtkPVComparativeVisPropertyWidget::SafeDownCast(caller));
      }
  }

  vtkPVCVCueSelectionCommand() : Dialog(0) {}

  vtkPVComparativeVisDialog* Dialog;
};

class vtkPVCVSourceDeletedCommand : public vtkCommand
{
public:
  void Execute(vtkObject*, unsigned long, void* callData)
  {
    if (this->Widget)
      {
      this->Widget->RemovePVSource((vtkPVSource*)callData);
      }
  }

  vtkPVCVSourceDeletedCommand(): Widget(0) {}

  vtkPVComparativeVisPropertyWidget* Widget;
};

//-----------------------------------------------------------------------------
vtkPVComparativeVisDialog::vtkPVComparativeVisDialog()
{
  this->Internal = new vtkPVComparativeVisDialogInternals;

  this->MainFrame = vtkKWFrame::New();
  this->TrackEditor = vtkPVTrackEditor::New();
  this->TrackEditor->SetFixedTimeKeyframeFlag(
    vtkPVTrackEditor::FIRST_KEYFRAME_TIME_NOTCHANGABLE |
    vtkPVTrackEditor::LAST_KEYFRAME_TIME_NOTCHANGABLE);
  this->NameEntry = vtkKWEntryWithLabel::New();
  this->VisualizationListFrame = vtkKWFrameWithLabel::New();
  
  this->NumberOfFramesFrame = vtkKWFrame::New();
  this->NumberOfXFramesEntry = vtkKWEntryWithLabel::New();
  this->NumberOfYFramesEntry = vtkKWEntryWithLabel::New();

  this->ButtonFrame = vtkKWFrame::New();
  this->OKButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisDialog::~vtkPVComparativeVisDialog()
{
  vtkPVWindow* window = vtkPVApplication::SafeDownCast(
    this->GetApplication())->GetMainWindow();

  if (window)
    {
    vtkPVComparativeVisDialogInternals::ObserversType::iterator iter =
      this->Internal->Observers.begin();
    for(; iter !=  this->Internal->Observers.end(); iter++)
      {
      window->RemoveObserver(*iter);
      }
    }

  delete this->Internal;

  this->TrackEditor->Delete();
  this->NameEntry->Delete();
  this->VisualizationListFrame->Delete();
  this->NumberOfFramesFrame->Delete();
  this->NumberOfXFramesEntry->Delete();
  this->NumberOfYFramesEntry->Delete();
  this->ButtonFrame->Delete();
  this->OKButton->Delete();
  this->CancelButton->Delete();
  this->MainFrame->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CueSelected(unsigned int i)
{
  if (i >= this->Internal->Widgets.size())
    {
    return;
    }
  if (this->Internal->Widgets[i])
    {
    this->Internal->Widgets[i]->ShowCueEditor();
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CueSelected(
  vtkPVComparativeVisPropertyWidget* wid)
{
  wid->ShowCueEditor();
  unsigned int numWids = this->Internal->Widgets.size();
  for (unsigned int i=0; i<numWids; i++)
    {
    if (this->Internal->Widgets[i] == wid)
      {
      this->Internal->RadioButtons[i]->SetSelectedState(1);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::NewPropertyWidget()
{
  vtkKWFrame* f1 = vtkKWFrame::New();
  this->Internal->PropertyFrames.push_back(f1);
  f1->SetParent(this->VisualizationListFrame->GetFrame());
  f1->Create();
  this->Script("pack %s -side top -pady 2 -padx 4", f1->GetWidgetName());
  
  vtkKWRadioButton* r1 = vtkKWRadioButton::New();
  this->Internal->RadioButtons.push_back(r1);
  r1->SetParent(f1);
  r1->Create();
  r1->SetVariableName("vtkPVComparativeVisDialogVar");
  unsigned int value = this->Internal->RadioButtons.size() - 1;
  r1->SetValueAsInt(value);
  ostrstream comm;
  comm << "CueSelected " << value << ends;
  r1->SetCommand(this, comm.str());
  delete[] comm.str();
  this->Script("pack %s -side left", r1->GetWidgetName());

  vtkPVComparativeVisPropertyWidget* w1 = 
    vtkPVComparativeVisPropertyWidget::New();
  this->Internal->Widgets.push_back(w1);

  w1->SetTrackEditor(this->TrackEditor);

  vtkPVCVCueSelectionCommand* command = new vtkPVCVCueSelectionCommand;
  command->Dialog = this;
  w1->AddObserver(vtkCommand::WidgetModifiedEvent, command);
  command->Delete();

  vtkPVCVSourceDeletedCommand* dcommand = new vtkPVCVSourceDeletedCommand;
  dcommand->Widget = w1;
  vtkPVWindow* window = vtkPVApplication::SafeDownCast(
    this->GetApplication())->GetMainWindow();
  window->AddObserver(vtkKWEvent::SourceDeletedEvent, dcommand);
  this->Internal->Observers.push_back(dcommand);
  dcommand->Delete();

  w1->SetParent(f1);
  w1->Create();
  this->Script("pack %s -side left", w1->GetWidgetName());

  f1->Delete();
  r1->Delete();
  w1->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::InitializeToDefault()
{
  vtkPVComparativeVisDialog::NumberOfVisualizationsCreated++;

  ostrstream visName;
  visName << "Comparative Vis " 
          << vtkPVComparativeVisDialog::NumberOfVisualizationsCreated
          << ends;
  this->NameEntry->GetWidget()->SetValue(visName.str());
  delete[] visName.str();

  this->VisualizationListFrame->GetFrame()->UnpackChildren();
  this->Internal->PropertyFrames.clear();
  this->Internal->RadioButtons.clear();
  this->Internal->Widgets.clear();

  this->NumberOfXFramesEntry->GetWidget()->SetValueAsInt(5);
  this->NumberOfYFramesEntry->GetWidget()->SetValueAsInt(5);

  // Create two property widgets by default
  this->NewPropertyWidget();
  this->NewPropertyWidget();
  
  vtkPVComparativeVisPropertyWidget* wid = this->Internal->Widgets[0];
  wid->ShowCueEditor();

  // Choose the first widget by default
  this->CueSelected(static_cast<unsigned int>(0));
  this->Internal->RadioButtons[0]->SetSelectedState(1);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CreateWidget()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVComparativeVisDialog already created");
    return;
    }

  this->Superclass::CreateWidget();

  this->MainFrame->SetParent(this);
  this->MainFrame->Create();
  this->Script("pack %s -expand t -fill both -padx 5 -pady 5", 
               this->MainFrame->GetWidgetName());

  this->NameEntry->SetParent(this->MainFrame);
  this->NameEntry->Create();
  this->NameEntry->SetLabelText("Visualization Name:");
  vtkKWTkUtilities::ChangeFontWeightToBold(this->NameEntry->GetLabel());
  this->Script("pack %s -side top -fill x -anchor n -pady 5", 
               this->NameEntry->GetWidgetName());

  this->NumberOfFramesFrame->SetParent(this->MainFrame);
  this->NumberOfFramesFrame->Create();
  this->Script(
    "pack %s -side top -fill x -anchor n -pady 5", 
    this->NumberOfFramesFrame->GetWidgetName());

  this->NumberOfXFramesEntry->SetParent(this->NumberOfFramesFrame);
  this->NumberOfXFramesEntry->Create();
  this->NumberOfXFramesEntry->SetLabelText("Number of X Frames:");
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->NumberOfXFramesEntry->GetLabel());

  this->NumberOfYFramesEntry->SetParent(this->NumberOfFramesFrame);
  this->NumberOfYFramesEntry->Create();
  this->NumberOfYFramesEntry->SetLabelText("Number of Y Frames:");
  vtkKWTkUtilities::ChangeFontWeightToBold(
    this->NumberOfYFramesEntry->GetLabel());

  this->Script(
    "pack %s -side left", 
    this->NumberOfXFramesEntry->GetWidgetName());
  this->Script(
    "pack %s -side left -padx 5", 
    this->NumberOfYFramesEntry->GetWidgetName());

  this->VisualizationListFrame->SetParent(this->MainFrame);
  this->VisualizationListFrame->Create();
  this->VisualizationListFrame->SetLabelText("Comparative Vis Properties");
  this->Script(
    "pack %s -side top -fill x -anchor n -pady 5", 
    this->VisualizationListFrame->GetWidgetName());

  this->TrackEditor->SetParent(this->MainFrame);
  this->TrackEditor->Create();
  this->Script("pack %s -side top -expand t -fill both", 
               this->TrackEditor->GetWidgetName());

  this->ButtonFrame->SetParent(this->MainFrame);
  this->ButtonFrame->Create();
  this->Script("pack %s -side top -fill x -pady 5", 
               this->ButtonFrame->GetWidgetName());
  
  this->OKButton->SetParent(this->ButtonFrame);
  this->OKButton->Create();
  this->OKButton->SetCommand(this, "OK");
  this->OKButton->SetText("OK");
  this->Script("pack %s -side left -fill x -expand t", 
               this->OKButton->GetWidgetName());

  this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->Create();
  this->CancelButton->SetCommand(this, "Cancel");
  this->CancelButton->SetText("Cancel");
  this->Script("pack %s -side left -fill x -expand t", 
               this->CancelButton->GetWidgetName());

  this->SetSize(vtkPVComparativeVisDialog::DialogWidth, 
                vtkPVComparativeVisDialog::DialogHeight);
  this->SetResizable(0, 0);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CopyToVisualization(
  vtkSMComparativeVisProxy* cv)
{
  if(!cv)
    {
    return;
    }

  cv->RemoveAllCache();
  cv->RemoveAllCues();
  vtkSMProxyProperty::SafeDownCast(
    cv->GetProperty("Cues"))->RemoveAllProxies();
  vtkSMIntVectorProperty::SafeDownCast(
    cv->GetProperty("NumberOfFramesInCue"))->SetNumberOfElements(0);
  vtkSMStringVectorProperty::SafeDownCast(
    cv->GetProperty("SourceNames"))->SetNumberOfElements(0);
  vtkSMStringVectorProperty::SafeDownCast(
    cv->GetProperty("SourceTclNames"))->SetNumberOfElements(0);
  cv->UpdateVTKObjects();

  vtkSMIntVectorProperty::SafeDownCast(
    cv->GetProperty("NumberOfXFrames"))->SetElement(
      0, this->NumberOfXFramesEntry->GetWidget()->GetValueAsInt());
  vtkSMIntVectorProperty::SafeDownCast(
    cv->GetProperty("NumberOfYFrames"))->SetElement(
      0, this->NumberOfYFramesEntry->GetWidget()->GetValueAsInt());

  vtkPVComparativeVisDialogInternals::WidgetsType::iterator iter =
    this->Internal->Widgets.begin();
  for (; iter != this->Internal->Widgets.end(); iter++)
    {
    iter->GetPointer()->CopyToVisualization(cv);
    }

  cv->SetVisName(this->NameEntry->GetWidget()->GetValue());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CopyFromVisualization(
  vtkSMComparativeVisProxy* cv)
{
  if (!cv)
    {
    return;
    }

  this->VisualizationListFrame->GetFrame()->UnpackChildren();
  this->Internal->Widgets.clear();
  this->Internal->RadioButtons.clear();
  this->Internal->PropertyFrames.clear();

  unsigned int numCues = cv->GetNumberOfCues();
  for (unsigned int i=0; i<numCues; i++)
    {
    this->NewPropertyWidget();
    vtkPVComparativeVisPropertyWidget* wid = this->Internal->Widgets[i];
    wid->CopyFromVisualization(i ,cv);
    }

  // We want at least 2 property widgets
  if (numCues < 2)
    {
    for (unsigned int i=numCues; i<2; i++)
      {
      this->NewPropertyWidget();
      }
    }

  this->NumberOfXFramesEntry->GetWidget()->SetValueAsInt(
    cv->GetNumberOfXFrames());
  this->NumberOfYFramesEntry->GetWidget()->SetValueAsInt(
    cv->GetNumberOfYFrames());
    
  this->NameEntry->GetWidget()->SetValue(cv->GetVisName());
  // Choose the first widget by default
  this->CueSelected(static_cast<unsigned int>(0));
  this->Internal->RadioButtons[0]->SetSelectedState(1);
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
