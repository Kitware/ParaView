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
#include "vtkPVProcessModule.h"
#include "vtkKWCheckButton.h"
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
vtkCxxRevisionMacro(vtkPVSaveBatchScriptDialog, "1.16");

int vtkPVSaveBatchScriptDialogCommand(ClientData cd, Tcl_Interp *interp,
                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSaveBatchScriptDialog::vtkPVSaveBatchScriptDialog()
{
  this->CommandFunction = vtkPVSaveBatchScriptDialogCommand;
  
  this->FilePath = NULL;
  this->FileRoot = NULL;

  this->ButtonFrame = vtkKWWidget::New();
  this->AcceptButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();
  
  this->OffScreenCheck = vtkKWCheckButton::New();

  this->SaveImagesCheck = vtkKWCheckButton::New();
  this->ImageFileNameFrame = vtkKWWidget::New();
  this->ImageFileNameEntry = vtkKWEntry::New();
  this->ImageFileNameBrowseButton = vtkKWPushButton::New();

  this->SaveGeometryCheck = vtkKWCheckButton::New();
  this->GeometryFileNameFrame = vtkKWWidget::New();
  this->GeometryFileNameEntry = vtkKWEntry::New();
  this->GeometryFileNameBrowseButton = vtkKWPushButton::New();

  this->Title = NULL;
  this->SetTitle("Batch File Elements");
  
  this->MasterWindow = 0;

  this->Exit = 1;
  this->AcceptedFlag = 1;
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
  
  this->SetTitle(NULL);
  this->SetMasterWindow(0);
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SetMasterWindow(vtkKWWindow* win)
{
  if (this->MasterWindow != win) 
    { 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->UnRegister(this); 
      }
    this->MasterWindow = win; 
    if (this->MasterWindow) 
      { 
      this->MasterWindow->Register(this); 
      if (this->IsCreated())
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    } 
  
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "toplevel", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  const char *wname = this->GetWidgetName();

  this->Script("wm title %s \"%s\"", wname, this->Title);
  this->Script("wm iconname %s \"vtk\"", wname);

  this->OffScreenCheck->SetParent(this);
  this->OffScreenCheck->Create(app, 0);
  this->OffScreenCheck->SetText("Offscreen");

  this->SaveImagesCheck->SetParent(this);
  this->SaveImagesCheck->Create(app, 0);
  this->SaveImagesCheck->SetState(1);
  this->SaveImagesCheck->SetText("Save Images");
  this->SaveImagesCheck->SetCommand(this, "SaveImagesCheckCallback");
  this->ImageFileNameFrame->SetParent(this);
  this->ImageFileNameFrame->Create(app, "frame", 0);

  this->SaveGeometryCheck->SetParent(this);
  this->SaveGeometryCheck->Create(app, 0);
  this->SaveGeometryCheck->SetState(0);
  this->SaveGeometryCheck->SetText("Save Geometry");
  this->SaveGeometryCheck->SetCommand(this, "SaveGeometryCheckCallback");
  this->GeometryFileNameFrame->SetParent(this);
  this->GeometryFileNameFrame->Create(app, "frame", 0);

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
  this->ImageFileNameEntry->Create(app, 0);
  if (fileName)
    {
    sprintf(fileName, "%s/%s.jpg", this->FilePath, this->FileRoot);
    this->ImageFileNameEntry->SetValue(fileName);
    }
  this->ImageFileNameBrowseButton->SetParent(this->ImageFileNameFrame);
  this->ImageFileNameBrowseButton->Create(app, 0);
  this->ImageFileNameBrowseButton->SetText("Browse");
  this->ImageFileNameBrowseButton->SetCommand(this, "ImageFileNameBrowseButtonCallback");
  this->Script("pack %s -side right -expand 0 -padx 2",
               this->ImageFileNameBrowseButton->GetWidgetName());
  this->Script("pack %s -side right -expand 1 -fill x -padx 2",
               this->ImageFileNameEntry->GetWidgetName());


  this->GeometryFileNameEntry->SetParent(this->GeometryFileNameFrame);
  this->GeometryFileNameEntry->Create(app, 0);
  if (fileName)
    {
    sprintf(fileName, "%s/%s.vtp", this->FilePath, this->FileRoot);
    this->GeometryFileNameEntry->SetValue(fileName);
    }
  this->GeometryFileNameBrowseButton->SetParent(this->GeometryFileNameFrame);
  this->GeometryFileNameBrowseButton->Create(app, 0);
  this->GeometryFileNameBrowseButton->SetText("Browse");
  this->GeometryFileNameBrowseButton->SetCommand(this, "GeometryFileNameBrowseButtonCallback");

  this->GeometryFileNameEntry->SetEnabled(0);
  this->GeometryFileNameBrowseButton->SetEnabled(0);

  this->Script("pack %s -side right -expand 0 -padx 2",
               this->GeometryFileNameBrowseButton->GetWidgetName());
  this->Script("pack %s -side right -expand 1 -fill x -padx 2",
               this->GeometryFileNameEntry->GetWidgetName());



  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill both -expand 0 -pady 2m",
               this->ButtonFrame->GetWidgetName());
  this->AcceptButton->SetParent(this->ButtonFrame);
  this->AcceptButton->Create(app, "");
  this->AcceptButton->SetCommand(this, "Accept");
  this->AcceptButton->SetText("Accept");
  this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->Create(app, "");
  this->CancelButton->SetCommand(this, "Cancel");
  this->CancelButton->SetText("Cancel");
  this->Script("pack %s %s -side left -expand 1 -fill x -padx 2",
               this->AcceptButton->GetWidgetName(),
               this->CancelButton->GetWidgetName());

  this->Script("wm protocol %s WM_DELETE_WINDOW {wm withdraw %s}",
               wname, wname);

  this->Script("wm withdraw %s", wname);

  this->Script("wm protocol %s WM_DELETE_WINDOW { %s Cancel}",
               this->GetWidgetName(), this->GetTclName());

  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
                 this->MasterWindow->GetWidgetName());
    }
  else
    {
    int sw, sh;
    this->Script("concat [winfo screenwidth %s] [winfo screenheight %s]",
                 this->GetWidgetName(), this->GetWidgetName());
    sscanf(app->GetMainInterp()->result, "%d %d", &sw, &sh);

    int ww, wh;
    this->Script("concat [winfo reqwidth %s] [winfo reqheight %s]",
                 this->GetWidgetName(), this->GetWidgetName());
    sscanf(app->GetMainInterp()->result, "%d %d", &ww, &wh);
    this->Script("wm geometry %s +%d+%d", this->GetWidgetName(), 
                 (sw-ww)/2, (sh-wh)/2);
    }

}

//----------------------------------------------------------------------------
int vtkPVSaveBatchScriptDialog::GetOffScreen()
{
  return this->OffScreenCheck->GetState();
}

//----------------------------------------------------------------------------
const char* vtkPVSaveBatchScriptDialog::GetImagesFileName()
{
  if ( ! this->SaveImagesCheck->GetState())
    {
    return NULL;
    }

  return this->ImageFileNameEntry->GetValue();
}

//----------------------------------------------------------------------------
const char* vtkPVSaveBatchScriptDialog::GetGeometryFileName()
{
  if ( ! this->SaveGeometryCheck->GetState())
    {
    return NULL;
    }

  return this->GeometryFileNameEntry->GetValue();
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::SaveImagesCheckCallback()
{
  if (this->SaveImagesCheck->GetState())
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
void vtkPVSaveBatchScriptDialog::SaveGeometryCheckCallback()
{
  if (this->SaveGeometryCheck->GetState())
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
  loadDialog->Create(this->GetPVApplication(), 0);
  loadDialog->SetTitle("Select File Pattern");

  // Look for the current extension.
  char *fileName = this->ImageFileNameEntry->GetValue();
  char *ptr;
  char *ext = NULL;

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
  loadDialog->Create(this->GetPVApplication(), 0);
  loadDialog->SetTitle("Select Geometry File Pattern");

  // Look for the current extension.
  char *fileName = this->GeometryFileNameEntry->GetValue();
  char *ptr;
  char *ext = NULL;

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
int vtkPVSaveBatchScriptDialog::Invoke()
{   
  int sw, sh;
  sscanf(this->Script("concat [winfo screenwidth .] [winfo screenheight .]"),
         "%d %d", &sw, &sh);
  
  int width, height;

  int x, y;

  if (this->MasterWindow)
    {
    this->Script("wm geometry %s", this->MasterWindow->GetWidgetName());
    sscanf(this->GetApplication()->GetMainInterp()->result, "%dx%d+%d+%d",
           &width, &height, &x, &y);
    
    x += width / 2;
    y += height / 2;
    
    if (x > sw - 200)
      {
      x = sw / 2;
      }
    if (y > sh - 200)
      {
      y = sh / 2;
      }
    }
  else
    {
    x = sw / 2;
    y = sh / 2;
    }

  width = atoi(this->Script("winfo reqwidth %s", this->GetWidgetName()));
  height = atoi(this->Script("winfo reqheight %s", this->GetWidgetName()));
  
  if (x > width / 2)
    {
    x -= width / 2;
    }
  if (y > height / 2)
    {
    y -= height / 2;
    }

  this->Script("wm geometry %s +%d+%d", this->GetWidgetName(),
               x, y);

  this->Script("wm deiconify %s", this->GetWidgetName());
  this->Script("grab %s", this->GetWidgetName());

  this->Exit = 0;
  this->AcceptedFlag = 0;
  while (this->Exit == 0)
    {
    // I assume the update will process multiple events.
    this->Script("update");
    if (this->Exit == 0)
      {
      this->Script("after 100");
      }
    }

  this->Script("grab release %s", this->GetWidgetName());
  this->Script("wm withdraw %s", this->GetWidgetName());

  return this->AcceptedFlag;
}



//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::Accept()
{
  this->Exit = 1;
  this->AcceptedFlag = 1;
}

//----------------------------------------------------------------------------
void vtkPVSaveBatchScriptDialog::Cancel()
{
  this->Exit = 1;
  this->AcceptedFlag = 0;
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
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << endl;
  os << indent << "FilePath: " << (this->FilePath ? this->FilePath : "(none)") << endl;
  os << indent << "FileRoot: " << (this->FileRoot ? this->FileRoot : "(none)") << endl;
}
