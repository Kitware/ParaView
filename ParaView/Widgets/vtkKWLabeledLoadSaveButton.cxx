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

#include "vtkKWLabeledLoadSaveButton.h"

#include "vtkKWLabel.h"
#include "vtkKWLoadSaveButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledLoadSaveButton);
vtkCxxRevisionMacro(vtkKWLabeledLoadSaveButton, "1.1");

int vtkKWLabeledLoadSaveButtonCommand(ClientData cd, Tcl_Interp *interp,
                                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledLoadSaveButton::vtkKWLabeledLoadSaveButton()
{
  this->CommandFunction = vtkKWLabeledLoadSaveButtonCommand;

  this->PackLabelLast = 0;

  this->LoadSaveButton = vtkKWLoadSaveButton::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledLoadSaveButton::~vtkKWLabeledLoadSaveButton()
{
  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->Delete();
    this->LoadSaveButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledLoadSaveButton already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the push button

  this->LoadSaveButton->SetParent(this);
  this->LoadSaveButton->Create(app, "");

  // Pack the label and the push button

  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackLabelLast)
    {
    tk_cmd << "pack " << this->LoadSaveButton->GetWidgetName() 
           << " -side left -fill x" << endl;
    }

  if (this->ShowLabel)
    {
    tk_cmd << "pack " << this->Label->GetWidgetName() << " -side left" << endl;
    }

  if (!this->PackLabelLast)
    {
    tk_cmd << "pack " << this->LoadSaveButton->GetWidgetName() 
           << " -side left -fill x" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetLoadSaveButtonLabel(const char *text)
{
  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetLabel(text);
    }
}

//----------------------------------------------------------------------------
char* vtkKWLabeledLoadSaveButton::GetLoadSaveButtonFileName()
{
  if (this->LoadSaveButton)
    {
    return this->LoadSaveButton->GetFileName();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetPackLabelLast(int arg)
{
  if (this->PackLabelLast == arg)
    {
    return;
    }

  this->PackLabelLast = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->LoadSaveButton)
    {
    this->LoadSaveButton->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledLoadSaveButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LoadSaveButton: " << this->LoadSaveButton << endl;
  os << indent << "PackLabelLast: " 
     << (this->PackLabelLast ? "On" : "Off") << endl;
}

