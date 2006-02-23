/*=========================================================================

  Module:    vtkKWLoadSaveButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWLoadSaveButton.h"

#include "vtkKWIcon.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkObjectFactory.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLoadSaveButton);
vtkCxxRevisionMacro(vtkKWLoadSaveButton, "1.22");

//----------------------------------------------------------------------------
vtkKWLoadSaveButton::vtkKWLoadSaveButton()
{
  this->LoadSaveDialog = vtkKWLoadSaveDialog::New();

  this->MaximumFileNameLength = 30;
  this->TrimPathFromFileName  = 1;

  this->UserCommand = NULL;
}

//----------------------------------------------------------------------------
vtkKWLoadSaveButton::~vtkKWLoadSaveButton()
{
  if (this->LoadSaveDialog)
    {
    this->LoadSaveDialog->Delete();
    this->LoadSaveDialog = NULL;
    }

  this->SetUserCommand(NULL);
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupButton already created");
    return;
    }

  // Call the superclass, this will set the application and 
  // create the pushbutton.

  this->Superclass::Create();

  // Do not use SetCommand (we override it to get max compatibility)
  // Save the old command, if any

  this->SetUserCommand(this->GetConfigurationOption("-command"));
  this->Superclass::SetCommand(this, "InvokeLoadSaveDialogCallback");

  // Cosmetic add-on

  this->SetImageToPredefinedIcon(vtkKWIcon::IconFolder);
  this->SetConfigurationOption("-compound", "left");
  this->SetPadX(3);
  this->SetPadY(2);

  // No filename yet, set it to empty

  if (!this->GetText())
    {
    this->SetText("");
    }

  // Create the load/save dialog

  this->LoadSaveDialog->SetParent(this);
  this->LoadSaveDialog->Create();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::SetCommand(vtkObject *object, 
                                     const char *method)
{
  if (!this->IsCreated())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  this->SetUserCommand(command);
  delete [] command;
}

//----------------------------------------------------------------------------
char* vtkKWLoadSaveButton::GetFileName()
{
  if (this->LoadSaveDialog)
    {
    return this->LoadSaveDialog->GetFileName();
    }
  return NULL;
} 

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::SetMaximumFileNameLength(int arg)
{
  if (this->MaximumFileNameLength == arg)
    {
    return;
    }

  this->MaximumFileNameLength = arg;
  this->Modified();

  this->UpdateFileName();
} 

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::SetTrimPathFromFileName(int arg)
{
  if (this->TrimPathFromFileName == arg)
    {
    return;
    }

  this->TrimPathFromFileName = arg;
  this->Modified();

  this->UpdateFileName();
} 

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::UpdateFileName()
{
  const char *fname = this->GetFileName();
  if (!fname || !*fname)
    {
    this->SetText(NULL);
    return;
    }

  if (this->MaximumFileNameLength <= 0 && !this->TrimPathFromFileName)
    {
    this->SetText(fname);
    }
  else
    {
    vtksys_stl::string new_fname; 
    if (this->TrimPathFromFileName)
      {
      new_fname = vtksys::SystemTools::GetFilenameName(fname);
      }
    else
      {
      new_fname = fname;
      }
    new_fname = 
      vtksys::SystemTools::CropString(new_fname, this->MaximumFileNameLength);
    this->SetText(new_fname.c_str());
    }
} 

// ---------------------------------------------------------------------------
void vtkKWLoadSaveButton::InvokeLoadSaveDialogCallback()
{
  if (!this->LoadSaveDialog->IsCreated() ||
      !this->LoadSaveDialog->Invoke())
    {
    return;
    }

  this->UpdateFileName();

  if (this->UserCommand && *this->UserCommand)
    {
    this->Script(this->UserCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LoadSaveDialog);
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LoadSaveDialog: " << this->LoadSaveDialog << endl;
  os << indent << "MaximumFileNameLength: " 
     << this->MaximumFileNameLength << endl;
  os << indent << "TrimPathFromFileName: " 
     << (this->TrimPathFromFileName ? "On" : "Off") << endl;
}
