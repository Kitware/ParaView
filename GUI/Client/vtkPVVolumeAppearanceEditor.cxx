/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVolumeAppearanceEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVolumeAppearanceEditor.h"

#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVolumeAppearanceEditor);
vtkCxxRevisionMacro(vtkPVVolumeAppearanceEditor, "1.2");

int vtkPVVolumeAppearanceEditorCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::vtkPVVolumeAppearanceEditor()
{
  this->CommandFunction = vtkPVVolumeAppearanceEditorCommand;
  this->PVRenderView = NULL;

  this->BackButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVVolumeAppearanceEditor::~vtkPVVolumeAppearanceEditor()
{
  this->BackButton->Delete();
  this->BackButton = NULL;
  
  this->SetPVRenderView(NULL);

}
//----------------------------------------------------------------------------
// No register count because of reference loop.
void vtkPVVolumeAppearanceEditor::SetPVRenderView(vtkPVRenderView *rv)
{
  this->PVRenderView = rv;
}
//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::Create(vtkKWApplication *app)
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("PVColorMap already created");
    return;
    }
  // Superclass create takes a KWApplication, but we need a PVApplication.
  if (pvApp == NULL)
    {
    vtkErrorMacro("Need a PV application");
    return;
    }
  
  this->SetApplication(app);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  

  // Back button

  this->BackButton->SetParent(this);
  this->BackButton->Create(this->Application, "-text {Back}");
  this->BackButton->SetCommand(this, "BackButtonCallback");

  this->Script("pack %s -side top -anchor n -fill x -padx 2 -pady 2", 
               this->BackButton->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::BackButtonCallback()
{
  if (this->PVRenderView == NULL)
    {
    return;
    }

  // This has a side effect of gathering and display information.
  this->PVRenderView->GetPVWindow()->GetCurrentPVSource()->GetPVOutput()->UpdateProperties();
  this->PVRenderView->GetPVWindow()->ShowCurrentSourceProperties();
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ResetScalarRange()
{
  this->ResetScalarRangeInternal();
  this->AddTraceEntry("$kw(%s) ResetScalarRange", this->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::ResetScalarRangeInternal()
{
}


//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->BackButton);
}

//----------------------------------------------------------------------------
void vtkPVVolumeAppearanceEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

