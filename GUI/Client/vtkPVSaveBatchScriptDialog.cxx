/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSaveBatchScriptDialog.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSaveBatchScriptDialog.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWWindow.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkTimerLog.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVSaveBatchScriptDialog );
vtkCxxRevisionMacro(vtkPVSaveBatchScriptDialog, "1.24");

//----------------------------------------------------------------------------
vtkPVSaveBatchScriptDialog::vtkPVSaveBatchScriptDialog()
{
  this->FilePath = NULL;
  this->FileRoot = NULL;

  this->ButtonFrame = vtkKWFrame::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();
  
  this->OffScreenCheck = vtkKWCheckButton::New();

  this->SaveImagesCheck = vtkKWCheckButton::New();
  this->ImageFileNameFrame = vtkKWFrame::New();
  this->ImageFileNameEntry = vtkKWEntry::New();
  this->ImageFileNameBrowseButton = vtkKWPushButton::New();

  this->SaveGeometryCheck = vtkKWCheckButton::New();
  this->GeometryFileNameFrame = vtkKWFrame::New();
  this->GeometryFileNameEntry = vtkKWEntry::New();
  this->GeometryFileNameBrowseButton = vtkKWPushButton::New();

  this->SetTitle("Batch File Elements");
}

//----------------------------------------------------------------------------
vtkPVSaveBatchScriptDialog::~vtkPVSaveBatchScriptDialog()
{
  this->SetFilePath(NULL);
  this->SetFileRoot(NULL);

  this->OffScreenCheck->Delete();
  this->OffScreenCheck = NULL;

  this->SaveImagesCheck->Delete();
  this->SaveImagesCheck = NULL;
  this->ImageFileNameFrame->Delete();
  this->ImageFileNameFrame = NULL;
  this->ImageFileNameEntry->Delete();
  this->ImageFileNameEntry = NULL;
  this->ImageFileNameBrowseButton->Delete();
  this->ImageFileNameBrowseButton= NULL;

  this->SaveGeometryCheck->Delete();
  this->SaveGeometryCheck = NULL;
  this->GeometryFileNameFrame->Delete();
  this->GeometryFileNameFrame = NULL;
  this->GeometryFileNameEntry->Delete();
  this->GeometryFileNameEntry = NULL;
  this->GeometryFileNameBrowseButton->Delete();
  this->GeometryFileNameBrowseButton = NULL;

  this->AcceptButton->Delete();
  this->AcceptButton = NULL;
  this->CancelButton->Delete();
  this->CancelButton = NULL;
  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->OffScreenCheck->SetParent(this);
  this->OffScreenCheck->Create();
  this->OffScreenCheck->SetText("Offscreen");

  this->SaveImagesCheck->SetParent(this);
  this->SaveImagesCheck->Create();
  this->SaveImagesCheck->SetSelectedState(1);
  this->SaveImagesCheck->SetText("Save Images");
  this->SaveImagesCheck->SetCommand(this, "SaveImagesCheckCallback");

  this->ImageFileNameFrame->SetParent(this);
  this->ImageFileNameFrame->Create();

  this->SaveGeometryCheck->SetParent(this);
  this->SaveGeometryCheck->Create();
  this->SaveGeometryCheck->SetSelectedState(0);
  this->SaveGeometryCheck->SetText("Save Geometry");
  this->SaveGeometryCheck->SetCommand(this, "SaveGeometryCheckCallback");

  this->GeometryFileNameFrame->SetParent(this);
  this->GeometryFileNameFrame->Create();

  this->Script("pack %s %s -side top -padx 2 -anchor w",
               this->OffScreenCheck->GetWidgetName(),
               this->SaveImagesCheck->GetWidgetName());
  this->Script("pack %s -side top -expand 1 -fill x -padx 2",
               this->ImageFileNameFrame->GetWidgetName());

  //this->Script("pack %s -side top -expand 0 -padx 2 -anchor w",
  //this->SaveGeometryCheck->GetWidgetName());
  //this->Script("pack %s -side top -expand 1 -fill x -padx 2",
  //this->GeometryFileNameFrame->GetWidgetName());

  char* fileName = NULL;
  if (this->FilePath && this->FileRoot)
    {
    fileName = new char[strlen(this->FilePath)+strlen(this->FileRoot)+64];
    }
   
  this->ImageFileNameEntry->SetParent(this->ImageFileNameFrame);
  this->ImageFileNameEntry->Create();
  if (fileName)
    {
    sprintf(fileName, "%s/%s.jpg", this->FilePath, this->FileRoot);
    this->ImageFileNameEntry->SetValue(fileName);
    }

  this->ImageFileNameBrowseButton->SetParent(this->ImageFileNameFrame);
  this->ImageFileNameBrowseButton->Create();
  this->ImageFileNameBrowseButton->SetText("Browse");
  this->ImageFileNameBrowseButton->SetCommand(this, "ImageFileNameBrowseButtonCallback");
  this->Script("pack %s -side right -expand 0 -padx 2",
               this->ImageFileNameBrowseButton->GetWidgetName());
  this->Script("pack %s -side right -expand 1 -fill x -padx 2",
               this->ImageFileNameEntry->GetWidgetName());


  this->GeometryFileNameEntry->SetParent(this->GeometryFileNameFrame);
  this->GeometryFileNameEntry->Create();
  if (fileName)
    {
    sprintf(fileName, "%s/%s.vtp", this->FilePath, this->FileRoot);
    this->GeometryFileNameEntry->SetValue(fileName);
    }

  this->GeometryFileNameBrowseButton->SetParent(this->GeometryFileNameFrame);
  this->GeometryFileNameBrowseButton->Create();
  this->GeometryFileNameBrowseButton->SetText("Browse");
  this->GeometryFileNameBrowseButton->SetCommand(this, "GeometryFileNameBrowseButtonCallback");

  this->GeometryFileNameEntry->SetEnabled(0);
  this->GeometryFileNameBrowseButton->SetEnabled(0);

  this->Script("pack %s -side right -expand 0 -padx 2",
               this->GeometryFileNameBrowseButton->GetWidgetName());
  this->Script("pack %s -side right -expand 1 -fill x -padx 2",
               this->GeometryFileNameEntry->GetWidgetName());

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create();
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());

  this->AcceptButton->SetParent(this->ButtonFrame);
  this->AcceptButton->Create();
  this->AcceptButton->SetCommand(this, "OK");
  this->AcceptButton->SetText("Accept");

  this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->Create();
  this->CancelButton->SetCommand(this, "Cancel");
  this->CancelButton->SetText("Cancel");

  this->Script("pack %s %s -side left -expand 1 -fill x -padx 2",
               this->AcceptButton->GetWidgetName(),
               this->CancelButton->GetWidgetName());
}

//----------------------------------------------------------------------------
int vtkPVSaveBatchScriptDialog::GetOffScreen()
{
  return this->OffScreenCheck->GetSelectedState();
}

//----------------------------------------------------------------------------
const char* vtkPVSaveBatchScriptDialog::GetImagesFileName()
{
  if ( ! this->SaveImagesCheck->GetSelectedState())
    {
    return NULL;
    }

  return this->ImageFileNameEntry->GetValue();
}

//----------------------------------------------------------------------------
const char* vtkPVSaveBatchScriptDialog::GetGeometryFileName()
{
  if ( ! this->SaveGeometryCheck->GetSelectedState())
    {
    return NULL;
    }

  return this->GeometryFileNameEntry->GetValue();
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SaveImagesCheckCallback(int state)
{
  if (state)
    {
    this->ImageFileNameEntry->SetEnabled(1);
    this->ImageFileNameBrowseButton->SetEnabled(1);
    }
  else
    {
    this->ImageFileNameEntry->SetEnabled(0);
    this->ImageFileNameBrowseButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SaveGeometryCheckCallback(int state)
{
  if (state)
    {
    this->GeometryFileNameEntry->SetEnabled(1);
    this->GeometryFileNameBrowseButton->SetEnabled(1);
    }
  else
    {
    this->GeometryFileNameEntry->SetEnabled(0);
    this->GeometryFileNameBrowseButton->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::ImageFileNameBrowseButtonCallback()
{
  ostrstream str;
  vtkKWLoadSaveDialog* loadDialog = this->GetPVApplication()->NewLoadSaveDialog();
  loadDialog->Create();
  loadDialog->SetTitle("Select File Pattern");

  // Look for the current extension.
  const char *fileName = this->ImageFileNameEntry->GetValue();
  const char *ptr;
  const char *ext = NULL;

  ptr = fileName;
  while (*ptr != '\0')
    {
    if (*ptr == '.')
      {
      ext = ptr;
      }
    ++ptr;
    }

  if (ext == NULL || ext[1] == '\0')
    {
    loadDialog->SetDefaultExtension("jpg");
    }
  else
    {
    loadDialog->SetDefaultExtension(ext);
    }
  str << "{{} {.jpg}} {{} {.tif}} {{} {.png}} ";
  str << "{{All files} {*}}" << ends;  
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  if(loadDialog->Invoke())
    {
    this->ImageFileNameEntry->SetValue(loadDialog->GetFileName());
    }

  loadDialog->Delete();
}







//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::GeometryFileNameBrowseButtonCallback()
{
  ostrstream str;
  vtkKWLoadSaveDialog* loadDialog = this->GetPVApplication()->NewLoadSaveDialog();
  loadDialog->Create();
  loadDialog->SetTitle("Select Geometry File Pattern");

  // Look for the current extension.
  const char *fileName = this->GeometryFileNameEntry->GetValue();
  const char *ptr;
  const char *ext = NULL;

  ptr = fileName;
  while (*ptr != '\0')
    {
    if (*ptr == '.')
      {
      ext = ptr;
      }
    ++ptr;
    }

  if (ext == NULL || ext[1] == '\0')
    {
    loadDialog->SetDefaultExtension("vtk");
    }
  else
    {
    loadDialog->SetDefaultExtension(ext);
    }
  str << "{{} {.vtk}} ";
  str << "{{All files} {*}}" << ends;  
  loadDialog->SetFileTypes(str.str());
  str.rdbuf()->freeze(0);  
  if(loadDialog->Invoke())
    {
    this->GeometryFileNameEntry->SetValue(loadDialog->GetFileName());
    }

  loadDialog->Delete();
}

//----------------------------------------------------------------------------
vtkPVApplication *vtkPVSaveBatchScriptDialog::GetPVApplication()
{
  return vtkPVApplication::SafeDownCast(this->GetApplication());
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "FilePath: " << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "FileRoot: " << (this->FileRoot ? this->FileRoot : "(none)") << endl;
}
