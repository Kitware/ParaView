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
#include "vtkKWFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVComparativeVis.h"
#include "vtkPVComparativeVisWidget.h"
#include "vtkPVTrackEditor.h"
#include "vtkPVWindow.h"

#include <vtkstd/vector>
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVComparativeVisDialog );
vtkCxxRevisionMacro(vtkPVComparativeVisDialog, "1.1");

int vtkPVComparativeVisDialogCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

struct vtkPVComparativeVisDialogInternals
{
  typedef 
  vtkstd::vector<vtkSmartPointer<vtkPVComparativeVisWidget> > WidgetsType;
  WidgetsType Widgets;
};

class vtkCueSelectionCommand : public vtkCommand
{
public:
  void Execute(vtkObject *caller, unsigned long eid, void *callData)
  {
    this->Dialog->CueSelected(vtkPVComparativeVisWidget::SafeDownCast(caller));
  }

  vtkPVComparativeVisDialog* Dialog;
};

//-----------------------------------------------------------------------------
vtkPVComparativeVisDialog::vtkPVComparativeVisDialog()
{
  this->Internal = new vtkPVComparativeVisDialogInternals;
  this->SetStyleToOkCancel();

  this->TrackEditor = vtkPVTrackEditor::New();
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisDialog::~vtkPVComparativeVisDialog()
{
  delete this->Internal;

  this->TrackEditor->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CueSelected(vtkPVComparativeVisWidget* wid)
{
  wid->ShowCueEditor(this->TrackEditor);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::InitializeToDefault()
{
  vtkPVComparativeVisWidget* w1 = vtkPVComparativeVisWidget::New();
  this->Internal->Widgets.push_back(w1);
  vtkCueSelectionCommand* command = new vtkCueSelectionCommand;
  command->Dialog = this;
  w1->AddObserver(vtkCommand::WidgetModifiedEvent, command);
  command->Delete();
  w1->Delete();

  vtkPVComparativeVisWidget* w2 = vtkPVComparativeVisWidget::New();
  this->Internal->Widgets.push_back(w2);
  command = new vtkCueSelectionCommand;
  command->Dialog = this;
  w2->AddObserver(vtkCommand::WidgetModifiedEvent, command);
  command->Delete();
  w2->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::Create(vtkKWApplication *app, const char *args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVComparativeVisDialog already created");
    return;
    }

  this->Superclass::Create(app,args);

  vtkPVComparativeVisDialogInternals::WidgetsType::iterator iter =
    this->Internal->Widgets.begin();
  for (; iter != this->Internal->Widgets.end(); iter++)
    {
    vtkPVComparativeVisWidget* wid = iter->GetPointer();
    wid->SetParent(this->TopFrame);
    wid->Create(app, "");
    this->Script("pack %s -side top", wid->GetWidgetName());
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVAnimationManager* pvAM = pvWin->GetAnimationManager(); 
  this->TrackEditor->SetAnimationManager(pvAM);
  this->TrackEditor->ShowKeyFrameLabelOff();

  this->TrackEditor->SetParent(this->TopFrame);
  this->TrackEditor->Create(app, "");
  this->Script("pack %s -side top -expand t -fill x", 
               this->TrackEditor->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CopyToVisualization(vtkPVComparativeVis* cv)
{
  if(!cv)
    {
    return;
    }

  vtkPVComparativeVisDialogInternals::WidgetsType::iterator iter =
    this->Internal->Widgets.begin();
  for (; iter != this->Internal->Widgets.end(); iter++)
    {
    iter->GetPointer()->CopyToVisualization(cv);
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::InitializeFromVisualization(
  vtkPVComparativeVis* cv)
{
  if (!cv)
    {
    return;
    }

  unsigned int numCues = cv->GetNumberOfProperties();
  for (unsigned int i=0; i<numCues; i++)
    {
    vtkPVComparativeVisWidget* wid = vtkPVComparativeVisWidget::New();
    vtkCueSelectionCommand* command = new vtkCueSelectionCommand;
    command->Dialog = this;
    wid->AddObserver(vtkCommand::WidgetModifiedEvent, command);
    command->Delete();
    this->Internal->Widgets.push_back(wid);
    wid->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::CopyFromVisualization(vtkPVComparativeVis* cv)
{
  if (!cv)
    {
    return;
    }

  unsigned int numCues = cv->GetNumberOfProperties();
  for (unsigned int i=0; i<numCues; i++)
    {
    vtkPVComparativeVisWidget* wid = this->Internal->Widgets[i];
    wid->CopyFromVisualization(cv->GetAnimationCue(i));
    }
}

//----------------------------------------------------------------------------
void vtkPVComparativeVisDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
