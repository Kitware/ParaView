/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWizard.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkPVWizard.h"
#include "vtkObjectFactory.h"
#include "vtkPVWindow.h"
#include "vtkPVFileEntry.h"
#include "vtkPVSelectionList.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkCollection.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVWizard );

int vtkPVWizardCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVWizard::vtkPVWizard()
{
  this->CommandFunction = vtkPVWizardCommand;
  this->Done = 1;
  this->TitleString = 0;
  this->SetTitleString("CTH Wizard");
  this->MasterWindow = 0;

  this->Label = vtkKWLabel::New();
  this->FileEntry = vtkPVFileEntry::New();

  this->WizardFrame = vtkKWWidget::New();
  this->ButtonFrame = vtkKWWidget::New();
  this->NextButton = vtkKWPushButton::New();
  this->CancelButton = vtkKWPushButton::New();

  this->ReaderTclName = NULL;

  this->FirstFileName = NULL;
  this->LastFileName = NULL;
  this->Stride = 0;
  this->MaterialChecks = vtkCollection::New();
  this->ColorArrayName = NULL;

  this->Data = NULL;
  this->String = NULL;
}

//----------------------------------------------------------------------------
vtkPVWizard::~vtkPVWizard()
{
  this->SetTitleString(0);
  this->SetMasterWindow(0);

  this->Label->Delete();
  this->Label = NULL;

  this->FileEntry->Delete();
  this->FileEntry = NULL;

  this->WizardFrame->Delete();
  this->WizardFrame = NULL;

  this->ButtonFrame->Delete();
  this->ButtonFrame = NULL;
  this->NextButton->Delete();
  this->NextButton = NULL;
  this->CancelButton->Delete();
  this->CancelButton = NULL;

  if (this->ReaderTclName)
    {
    this->Script("%s Delete", this->ReaderTclName);
    this->SetReaderTclName(NULL);
    }

  this->SetFirstFileName(NULL);
  this->SetLastFileName(NULL);
  this->MaterialChecks->Delete();
  this->MaterialChecks = NULL;
  this->SetColorArrayName(NULL);

  this->SetData(NULL);
  this->SetString(NULL);
}

//----------------------------------------------------------------------------
void vtkPVWizard::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("Dialog already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("toplevel %s %s",wname,args);
  this->Script("wm title %s \"%s\"",wname,this->TitleString);
  this->Script("wm iconname %s \"Dialog\"",wname);
  this->Script("wm protocol %s WM_DELETE_WINDOW {%s CancelCallback}",
               wname, this->GetTclName());
  this->Script("wm geometry %s 500x200", wname);
  this->Script("wm withdraw %s",wname);
  if (this->MasterWindow)
    {
    this->Script("wm transient %s %s", wname, 
		this->MasterWindow->GetWidgetName());
    }

  this->ButtonFrame->SetParent(this);
  this->ButtonFrame->Create(app, "frame", "");
  this->NextButton->SetParent(this->ButtonFrame);
  this->NextButton->Create(app, "");
  this->NextButton->SetLabel("Next");
  this->NextButton->SetCommand(this, "NextCallback");
  this->Script("pack %s -side left -fill x -expand t",
               this->NextButton->GetWidgetName());
  this->CancelButton->SetParent(this->ButtonFrame);
  this->CancelButton->Create(app, "");
  this->CancelButton->SetLabel("Cancel");
  this->CancelButton->SetCommand(this, "CancelCallback");
  this->Script("pack %s -side left -fill x -expand t",
               this->CancelButton->GetWidgetName());
  this->Script("pack %s -side bottom -fill x -expand t",
               this->ButtonFrame->GetWidgetName());

  this->WizardFrame->SetParent(this);
  this->WizardFrame->Create(app, "frame", "");
  this->Script("pack %s -side bottom -fill both -expand t",
               this->WizardFrame->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "");
  this->Script("pack %s -side bottom -fill x -expand f",
               this->Label->GetWidgetName());

  this->FileEntry->SetParent(this->WizardFrame);
  this->FileEntry->Create(this->Application, "xml", NULL);

  // create the reader.
  // I know the name conflict issue, but I expect to start using C++ ...
  this->Script("vtkARLXDMFReader ARLXDMFReader");
  this->SetReaderTclName("ARLXDMFReader");
}

//----------------------------------------------------------------------------
void vtkPVWizard::NextCallback()
{
  this->Done = 1;
}

//----------------------------------------------------------------------------
void vtkPVWizard::CancelCallback()
{
  this->Done = -1;
}

//----------------------------------------------------------------------------
void vtkPVWizard::CheckForValidFile(int gate)
{
  const char* fileName;

  fileName = this->FileEntry->GetValue();
  if (fileName == NULL || fileName[0] == '\0')
    {
    if (gate)
      {
      this->NextButton->Disable();
      }
    else
      {
      this->NextButton->Enable();
      }
    return;
    }

  // See if the file even exists.
  this->Script("file readable {%s}", fileName);
  if (this->GetIntegerResult(this->Application) == 0)
    {
    this->NextButton->Disable();
    return;
    }

  // See if this file is of the correct type.
  this->Script("%s CanReadFile {%s}", this->ReaderTclName, 
               this->FileEntry->GetValue());
  if (this->GetIntegerResult(this->Application))
    { 
    this->NextButton->Enable();
    }
  else
    {
    this->NextButton->Disable();
    }
}

//----------------------------------------------------------------------------
int vtkPVWizard::Invoke(vtkPVWindow *pvWin)
{
  this->Application->SetDialogUp(1);
  // map the window
  this->Script("wm deiconify %s",this->GetWidgetName());
  this->Script("focus %s",this->GetWidgetName());
  this->Script("update idletasks");
  this->Script("grab %s",this->GetWidgetName());

  // Get the starting and ending filenames.
  this->QueryFirstFileName();
  if (this->Done != -1)
    {
    this->QueryLastFileName();
    }
  if (this->Done != -1)
    {
    this->QueryStride();
    }
  if (this->Done != -1)
    {
    this->QueryMaterials();
    }
  if (this->Done != -1)
    {
    this->QueryColorVariable();
    }
  if (this->Done != -1)
    {
    this->QueryColoredMaterials();
    }
  if (this->Done != -1)
    {
    this->SetupPipeline(pvWin);
    }

  this->Script("grab release %s",this->GetWidgetName());
  this->Script("wm withdraw %s",this->GetWidgetName());

  this->Application->SetDialogUp(0);
  return (this->Done-1);
}

//----------------------------------------------------------------------------
void vtkPVWizard::SetupPipeline(vtkPVWindow *pvWin)
{
  vtkKWCheckButton *check;

  // Setup the reader.
  this->Script("set cthReader [%s Open {%s}]", pvWin->GetTclName(),
               this->FirstFileName);
  this->Script("[$cthReader GetPVWidget CellArraySelection] AllOffCallback");
  if (this->ColorArrayName)
    {
    this->Script("[$cthReader GetPVWidget CellArraySelection] SetArrayStatus {%s} 1",
                 this->ColorArrayName);
    }
  this->MaterialChecks->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->MaterialChecks->GetNextItemAsObject())) )
    {
    this->Script("[$cthReader GetPVWidget CellArraySelection] SetArrayStatus {Volume Fraction for %s} 1",
                 check->GetText());
    }
  this->Script("$cthReader AcceptCallback");

  // Setup the cell to point data filter.
  this->Script("set cthFilter [%s CreatePVSource vtkPCellDataToPointData]",
               pvWin->GetTclName());
  this->Script("$cthFilter AcceptCallback");

  // Create the Iso surfaces.
  this->MaterialChecks->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->MaterialChecks->GetNextItemAsObject())) )
    {
    this->Script("%s SetCurrentPVSource $cthFilter", pvWin->GetTclName());
    this->Script("set cthContour [%s ContourCallback]",
                 pvWin->GetTclName());
    this->Script("[$cthContour GetPVWidget Scalars] SetValue {Volume Fraction for %s}",
                 check->GetText());
    this->Script("[$cthContour GetPVWidget {Contour Values}] AddValue 0.5");
    this->Script("$cthContour AcceptCallback");
    if (this->ColorArrayName != NULL && this->ColorArrayName[0] != '\0' && check->GetState())
      {
      this->Script("[$cthContour GetPVOutput] ColorByPointFieldComponent {%s} 0",
                   this->ColorArrayName);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVWizard::QueryFirstFileName()
{
  // Disable the next button until a valid file is choose.
  this->NextButton->Disable();

  this->Label->SetLabel("Select your first CTH file.");
  this->FileEntry->SetLabel("First CTH File");
  this->FileEntry->SetModifiedCommand(this->GetTclName(), "CheckForValidFile 1");
  this->Script("pack %s -side top -fill x -expand t",
               this->FileEntry->GetWidgetName());

  // wait for the end
  this->Done = 0;
  while (this->Done == 0)
    {
    Tcl_DoOneEvent(0);    
    }

  this->SetFirstFileName(this->FileEntry->GetValue());
  this->Script("pack forget %s", this->FileEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVWizard::QueryLastFileName()
{
  this->Label->SetLabel("If you want to create an animation,\nChoose the last file in the time series.\nIf you want to just create an image, leave the second file blank.");
  this->FileEntry->SetLabel("Last CTH File");
  this->FileEntry->SetModifiedCommand(this->GetTclName(), "CheckForValidFile 0");
  this->Script("pack %s -side top -fill x -expand t",
               this->FileEntry->GetWidgetName());

  // Disable the next button until a valid file is choose.
  this->FileEntry->SetValue("");
  this->NextButton->Enable();

  // wait for the end
  this->Done = 0;
  while (this->Done == 0)
    {
    Tcl_DoOneEvent(0);    
    }

  this->SetLastFileName(this->FileEntry->GetValue());
  this->Script("pack forget %s", this->FileEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVWizard::QueryStride()
{
  vtkKWLabeledEntry *entry;
  char str[1024];
  int *ext;
  
  // Enable the next button because we have a good default value.
  this->NextButton->Enable();

  this->Script("%s SetFileName {%s}", this->ReaderTclName, this->FirstFileName);
  this->Script("%s UpdateInformation", this->ReaderTclName);
  this->Script("%s SetData [%s GetOutput]", this->GetTclName(), this->ReaderTclName);

  ext = this->Data->GetWholeExtent();
  sprintf(str, "The dimensions of the whole data set are:\nx: %d, y: %d. z: %d\nYou can subsampling the volume for previewing\n by entering a stride greater than 1.\nNote: The batch visualization will still be\ngenerated at full resolution.",
          (ext[1]-ext[0]+1), (ext[3]-ext[2]+1), (ext[5]-ext[4]+1));
  this->Label->SetLabel(str);

  entry = vtkKWLabeledEntry::New();
  entry->SetParent(this->WizardFrame);
  entry->Create(this->Application);
  entry->SetLabel("Stride");
  entry->SetValue("1");
  this->Script("pack %s -side top -fill x -expand 1",
               entry->GetWidgetName());

  // wait for the end
  this->Done = 0;
  while (this->Done == 0)
    {
    Tcl_DoOneEvent(0);
    if (this->Done == 1)
      { // check for a valid number.
      int val = atoi(entry->GetValue());
      if (val <= 0)
        {
        vtkErrorMacro("Bad stride.  Please choose an integer greater than 1.");
        this->Done = 0;
        }
      }
    }  

  this->Stride = atoi(entry->GetValue());
  this->Script("pack forget %s", entry->GetWidgetName());
  entry->Delete();
}


//----------------------------------------------------------------------------
void vtkPVWizard::QueryMaterials()
{
  vtkCollection *selectedChecks;
  vtkKWCheckButton *check;
  int num, idx;
  char *name;

  // Enable the next button because we default to visualize all materials.
  this->NextButton->Enable();

  this->Label->SetLabel("Choose which materials you want to include in your visualization.");

  this->Script("%s GetNumberOfCellArrays", this->ReaderTclName);
  this->MaterialChecks->RemoveAllItems();
  num = this->GetIntegerResult(this->Application);
  for (idx = 0; idx < num; ++idx)
    {
    this->Script("%s SetString [%s GetCellArrayName %d]", this->GetTclName(),
                 this->ReaderTclName, idx);
    if (strncmp("Volume Fraction for ", this->String, 20) == 0)
      {
      name = this->String+20;
      check = vtkKWCheckButton::New();
      check->SetParent(this->WizardFrame);
      check->Create(this->Application, "");
      check->SetText(name);
      check->SetState(1);
      this->MaterialChecks->AddItem(check);
      this->Script("pack %s -side top -fill x -expand t", check->GetWidgetName());
      check->Delete();
      }
    }

  // wait for the end
  this->Done = 0;
  while (this->Done == 0)
    {
    Tcl_DoOneEvent(0);
    }
  
  // Unpack all of the checks. Save the ones selected.
  selectedChecks = vtkCollection::New();
  this->MaterialChecks->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->MaterialChecks->GetNextItemAsObject())) )
    {
    this->Script("pack forget %s", check->GetWidgetName());
    if (check->GetState())
      {
      selectedChecks->AddItem(check);
      }
    }
  this->MaterialChecks->Delete();
  this->MaterialChecks = selectedChecks;
}

//----------------------------------------------------------------------------
void vtkPVWizard::QueryColorVariable()
{
  int num, idx;
  vtkPVSelectionList *menu;

  // Enable the next button because we default to have no arrays to visualize.
  this->NextButton->Enable();

  this->Label->SetLabel("Choose a variable to visualize.");

  menu = vtkPVSelectionList::New();
  menu->SetParent(this->WizardFrame);
  menu->Create(this->Application);
  menu->SetLabel("Color by variable");
  this->Script("pack %s -side top -fill x -expand t", menu->GetWidgetName());

  menu->AddItem("", 0);

  this->Script("%s GetNumberOfCellArrays", this->ReaderTclName);
  num = this->GetIntegerResult(this->Application);
  for (idx = 0; idx < num; ++idx)
    {
    this->Script("%s SetString [%s GetCellArrayName %d]", this->GetTclName(),
                 this->ReaderTclName, idx);
    menu->AddItem(this->String, idx+1);
    }

  // wait for the end
  this->Done = 0;
  while (this->Done == 0)
    {
    Tcl_DoOneEvent(0);
    }

  this->SetColorArrayName(menu->GetCurrentName());
    
  this->Script("pack forget %s", menu->GetWidgetName());
  menu->Delete();
}

//----------------------------------------------------------------------------
void vtkPVWizard::QueryColoredMaterials()
{
  vtkKWCheckButton *check;

  // Skip this if there is no color array choosen.
  if (this->ColorArrayName == NULL || this->ColorArrayName[0] == '\0')
    {
    return;
    }

  // Enable the next button. We default to color no materials.
  this->NextButton->Enable();

  this->Label->SetLabel("Which materials do you want to color?");

  this->MaterialChecks->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->MaterialChecks->GetNextItemAsObject())) )
    {
    check->SetState(0);
    this->Script("pack %s -side top -fill x -expand t", check->GetWidgetName());
    }

  // wait for the end
  this->Done = 0;
  while (this->Done == 0)
    {
    Tcl_DoOneEvent(0);
    }

  this->MaterialChecks->InitTraversal();
  while ( (check = (vtkKWCheckButton*)(this->MaterialChecks->GetNextItemAsObject())) )
    {
    this->Script("pack forget %s", check->GetWidgetName());
    }
}


//----------------------------------------------------------------------------
vtkKWWindow *vtkPVWizard::GetMasterWindow()
{
  return this->MasterWindow;
}

//----------------------------------------------------------------------------
void vtkPVWizard::SetMasterWindow(vtkKWWindow* win)
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
      if (this->Application)
        {
        this->Script("wm transient %s %s", this->GetWidgetName(), 
                     this->MasterWindow->GetWidgetName());
        }
      } 
    this->Modified(); 
    } 
}

//----------------------------------------------------------------------------
void vtkPVWizard::SetTitle( const char* title )
{
  if (this->Application)
    {
    this->Script("wm title %s \"%s\"", this->GetWidgetName(), 
		 title);
    }
  else
    {
    this->SetTitleString(title);
    }
}

