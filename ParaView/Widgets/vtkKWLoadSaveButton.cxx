/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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
#include "vtkKWLoadSaveButton.h"

#include "vtkKWApplication.h"
#include "vtkKWDirectoryUtilities.h"
#include "vtkKWIcon.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkObjectFactory.h"
#include "vtkString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLoadSaveButton);
vtkCxxRevisionMacro(vtkKWLoadSaveButton, "1.1.4.1");

int vtkKWLoadSaveButtonCommand(ClientData cd, Tcl_Interp *interp,
                               int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLoadSaveButton::vtkKWLoadSaveButton()
{
  this->CommandFunction = vtkKWLoadSaveButtonCommand;

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
void vtkKWLoadSaveButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PopupButton already created");
    return;
    }

  // Call the superclass, this will set the application and 
  // create the pushbutton.

  this->Superclass::Create(app, args);

  // Do not use SetCommand (we override it to get max compatibility)
  // Save the old command, if any

  this->SetUserCommand(this->Script("%s cget -command", 
                                    this->GetWidgetName()));

  this->Script("%s configure -command {%s InvokeLoadSaveDialogCallback}",
               this->GetWidgetName(), this->GetTclName());

  // Cosmetic add-on

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 4)
  this->SetImageOption(vtkKWIcon::ICON_FOLDER);
  this->Script("%s configure -compound left -padx 3 -pady 2", 
               this->GetWidgetName());
#endif

  // No filename yet, set it to empty

  if (!this->GetLabel())
    {
    this->SetLabel("");
    }

  // Create the load/save dialog

  this->LoadSaveDialog->Create(app, "");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::SetCommand(vtkKWObject *object, 
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
  return this->LoadSaveDialog->GetFileName();
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
    this->SetLabel(NULL);
    return;
    }

  if (this->MaximumFileNameLength <= 0 && !this->TrimPathFromFileName)
    {
    this->SetLabel(fname);
    }
  else
    {
    size_t fname_len = strlen(fname);
    char *new_fname = new char [fname_len + 1];
    if (this->TrimPathFromFileName)
      {
      vtkKWDirectoryUtilities::GetFilenameName(fname, new_fname);
      }
    else
      {
      strcpy(new_fname, fname);
      }
    vtkString::CropString(new_fname, this->MaximumFileNameLength);
    this->SetLabel(new_fname);
    delete [] new_fname;
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

  if (this->UserCommand)
    {
    this->Script("eval %s", this->UserCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWLoadSaveButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->LoadSaveDialog)
    {
    this->LoadSaveDialog->SetEnabled(this->Enabled);
    }
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
