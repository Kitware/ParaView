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
#include "vtkKWEntryLabeled.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWLabel.h"
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkKWTkUtilities.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVis.h"
#include "vtkPVComparativeVisPropertyWidget.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVWindow.h"

#include <vtkstd/vector>
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisDialog );
vtkCxxRevisionMacro(vtkPVComparativeVisDialog, "1.4");

int vtkPVComparativeVisDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

int vtkPVComparativeVisDialog::NumberOfVisualizationsCreated = 0;
const int vtkPVComparativeVisDialog::DialogWidth = 700;
const int vtkPVComparativeVisDialog::DialogHeight = 400;

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

  // Radio buttongs
  typedef vtkstd::vector<vtkSmartPointer<vtkKWRadioButton> > 
      RadioButtonsType;
  RadioButtonsType RadioButtons;
};

class vtkCueSelectionCommand : public vtkCommand
{
public:
  void Execute(vtkObject *caller, unsigned long, void*)
  {
    this->Dialog->CueSelected(
      vtkPVComparativeVisPropertyWidget::SafeDownCast(caller));
  }

  vtkPVComparativeVisDialog* Dialog;
};

//-----------------------------------------------------------------------------
vtkPVComparativeVisDialog::vtkPVComparativeVisDialog()
{
  this->Internal = new vtkPVComparativeVisDialogInternals;

  this->MainFrame = vtkKWFrame::New();
  this->TrackEditor = vtkPVTrackEditor::New();
  this->NameEntry = vtkKWEntryLabeled::New();
  this->VisualizationListFrame = vtkKWFrameLabeled::New();
  this->CloseButton = vtkKWPushButton::New();
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisDialog::~vtkPVComparativeVisDialog()
{
  delete this->Internal;

  this->MainFrame->Delete();
  this->TrackEditor->Delete();
  this->NameEntry->Delete();
  this->VisualizationListFrame->Delete();
  this->CloseButton->Delete();
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
    this->Internal->Widgets[i]->ShowCueEditor(this->TrackEditor);
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CueSelected(
  vtkPVComparativeVisPropertyWidget* wid)
{
  wid->ShowCueEditor(this->TrackEditor);
  unsigned int numWids = this->Internal->Widgets.size();
  for (unsigned int i=0; i<numWids; i++)
    {
    if (this->Internal->Widgets[i] == wid)
      {
      this->Internal->RadioButtons[i]->SetState(1);
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::NewPropertyWidget()
{
  vtkKWFrame* f1 = vtkKWFrame::New();
  this->Internal->PropertyFrames.push_back(f1);
  f1->SetParent(this->VisualizationListFrame->GetFrame());
  f1->Create(this->GetApplication());
  this->Script("pack %s -side top -pady 2 -padx 4", f1->GetWidgetName());
  
  vtkKWRadioButton* r1 = vtkKWRadioButton::New();
  this->Internal->RadioButtons.push_back(r1);
  r1->SetParent(f1);
  r1->SetVariableName("vtkPVComparativeVisDialogVar");
  r1->Create(this->GetApplication());
  unsigned int value = this->Internal->RadioButtons.size() - 1;
  r1->SetValue(value);
  ostrstream comm;
  comm << "CueSelected " << value << ends;
  r1->SetCommand(this, comm.str());
  delete[] comm.str();
  this->Script("pack %s -side left", r1->GetWidgetName());

  vtkPVComparativeVisPropertyWidget* w1 = 
    vtkPVComparativeVisPropertyWidget::New();
  this->Internal->Widgets.push_back(w1);
  vtkCueSelectionCommand* command = new vtkCueSelectionCommand;
  command->Dialog = this;
  w1->AddObserver(vtkCommand::WidgetModifiedEvent, command);
  command->Delete();
  w1->SetParent(f1);
  w1->Create(this->GetApplication());
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

  // Create two property widgets by default
  this->NewPropertyWidget();
  this->NewPropertyWidget();

  vtkPVComparativeVisPropertyWidget* wid = this->Internal->Widgets[0];
  wid->ShowCueEditor(this->TrackEditor);

  // Choose the first widget by default
  this->CueSelected(static_cast<unsigned int>(0));
  this->Internal->RadioButtons[0]->SetState(1);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVComparativeVisDialog already created");
    return;
    }

  this->Superclass::Create(app);

  this->MainFrame->SetParent(this);
  this->MainFrame->Create(app);
  this->Script("pack %s -expand t -fill both -padx 5 -pady 5", 
               this->MainFrame->GetWidgetName());

  this->NameEntry->SetParent(this->MainFrame);
  this->NameEntry->Create(app);
  this->NameEntry->SetLabelText("Visualization Name:");
  vtkKWTkUtilities::ChangeFontWeightToBold(this->NameEntry->GetLabel());
  this->Script("pack %s -side top -fill x -anchor n -pady 5", 
               this->NameEntry->GetWidgetName());

  this->VisualizationListFrame->SetParent(this->MainFrame);
  this->VisualizationListFrame->Create(app);
  this->VisualizationListFrame->SetLabelText("Comparative Vis Properties");
  this->Script(
    "pack %s -side top -fill x -anchor n -pady 5", 
    this->VisualizationListFrame->GetWidgetName());

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 
  this->TrackEditor->SetAnimationManager(pvAM);

  this->TrackEditor->SetParent(this->MainFrame);
  this->TrackEditor->Create(app);
  this->Script("pack %s -side top -expand t -fill both", 
               this->TrackEditor->GetWidgetName());

  this->CloseButton->SetParent(this->MainFrame);
  this->CloseButton->Create(app);
  this->CloseButton->SetCommand(this, "OK");
  this->CloseButton->SetText("Done");
  this->Script("pack %s -side top -fill x -pady 5", 
               this->CloseButton->GetWidgetName());

  this->SetSize(vtkPVComparativeVisDialog::DialogWidth, 
                vtkPVComparativeVisDialog::DialogHeight);
  this->SetResizable(0, 0);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CopyToVisualization(vtkPVComparativeVis* cv)
{
  if(!cv)
    {
    return;
    }

  cv->RemoveAllProperties();
  vtkPVComparativeVisDialogInternals::WidgetsType::iterator iter =
    this->Internal->Widgets.begin();
  for (; iter != this->Internal->Widgets.end(); iter++)
    {
    iter->GetPointer()->CopyToVisualization(cv);
    }

  cv->SetName(this->NameEntry->GetWidget()->GetValue());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CopyFromVisualization(vtkPVComparativeVis* cv)
{
  if (!cv)
    {
    return;
    }

  this->VisualizationListFrame->GetFrame()->UnpackChildren();
  this->Internal->Widgets.clear();
  this->Internal->RadioButtons.clear();
  this->Internal->PropertyFrames.clear();

  unsigned int numCues = cv->GetNumberOfProperties();
  for (unsigned int i=0; i<numCues; i++)
    {
    this->NewPropertyWidget();
    vtkPVComparativeVisPropertyWidget* wid = this->Internal->Widgets[i];
    wid->CopyFromVisualization(cv->GetAnimationCue(i), 
                               cv->GetCue(i), 
                               cv->GetNumberOfPropertyValues(i));
    }

  // We want at least 2 property widgets
  if (numCues < 2)
    {
    for (unsigned int i=numCues; i<2; i++)
      {
      this->NewPropertyWidget();
      }
    }
    
  this->NameEntry->GetWidget()->SetValue(cv->GetName());
  // Choose the first widget by default
  this->CueSelected(static_cast<unsigned int>(0));
  this->Internal->RadioButtons[0]->SetState(1);
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
