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

#include "vtkKWCheckButtonWithChangeColor.h"

#include "vtkKWChangeColorButton.h"
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWCheckButtonWithChangeColor);
vtkCxxRevisionMacro(vtkKWCheckButtonWithChangeColor, "1.4");

int vtkKWCheckButtonWithChangeColorCommand(ClientData cd, Tcl_Interp *interp,
                                           int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWCheckButtonWithChangeColor::vtkKWCheckButtonWithChangeColor()
{
  this->CommandFunction = vtkKWCheckButtonWithChangeColorCommand;

  this->CheckButton       = vtkKWCheckButton::New();
  this->ChangeColorButton = vtkKWChangeColorButton::New();

  this->DisableChangeColorButtonWhenNotChecked = 0;
}

//----------------------------------------------------------------------------
vtkKWCheckButtonWithChangeColor::~vtkKWCheckButtonWithChangeColor()
{
  if (this->CheckButton)
    {
    this->CheckButton->Delete();
    this->CheckButton = NULL;
    }

  if (this->ChangeColorButton)
    {
    this->ChangeColorButton->Delete();
    this->ChangeColorButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("vtkKWCheckButtonWithChangeColor widget already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s -relief flat -bd 0 -highlightthickness 0 %s", 
               this->GetWidgetName(), args ? args : "");
  
  // Create the checkbutton. 
  
  this->CheckButton->SetParent(this);
  this->CheckButton->Create(app, "-anchor w");

  // Create the change color button

  this->ChangeColorButton->SetParent(this);
  this->ChangeColorButton->Create(app, "");

  // Pack the checkbutton and the change color button

  this->Pack();

  // Update

  this->Update();
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->CheckButton->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  tk_cmd << "pack " << this->CheckButton->GetWidgetName() 
         << " -side left -anchor w" << endl
         << "pack " << this->ChangeColorButton->GetWidgetName() 
         << " -side left -anchor w -fill x -expand t -padx 2" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // Disable the color change button if not checked

  if (this->DisableChangeColorButtonWhenNotChecked &&
      this->ChangeColorButton && 
      this->CheckButton && this->CheckButton->IsCreated())
    {
    this->ChangeColorButton->SetEnabled(
      this->CheckButton->GetState() ? this->Enabled : 0);
    }
}

// ----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::SetDisableChangeColorButtonWhenNotChecked(
  int _arg)
{
  if (this->DisableChangeColorButtonWhenNotChecked == _arg)
    {
    return;
    }
  this->DisableChangeColorButtonWhenNotChecked = _arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CheckButton)
    {
    this->CheckButton->SetEnabled(this->Enabled);
    }

  if (this->ChangeColorButton)
    {
    this->ChangeColorButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButtonWithChangeColor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CheckButton: " << this->CheckButton << endl;
  os << indent << "ChangeColorButton: " << this->ChangeColorButton << endl;

  os << indent << "DisableChangeColorButtonWhenNotChecked: " 
     << (this->DisableChangeColorButtonWhenNotChecked ? "On" : "Off") << endl;
}

