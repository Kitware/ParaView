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

#include "vtkKWLabeledOptionMenu.h"

#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledOptionMenu);
vtkCxxRevisionMacro(vtkKWLabeledOptionMenu, "1.9");

int vtkKWLabeledOptionMenuCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWLabeledOptionMenu::vtkKWLabeledOptionMenu()
{
  this->CommandFunction = vtkKWLabeledOptionMenuCommand;

  this->PackHorizontally = 1;
  this->ExpandOptionMenu = 0;

  this->OptionMenu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkKWLabeledOptionMenu::~vtkKWLabeledOptionMenu()
{
  if (this->OptionMenu)
    {
    this->OptionMenu->Delete();
    this->OptionMenu = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledOptionMenu already created");
    return;
    }

  // Call the superclass, this will set the application and create the Label

  this->Superclass::Create(app, args);

  // Create the option menu

  this->OptionMenu->SetParent(this);
  this->OptionMenu->Create(app, "");

  // Pack the label and the option menu

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Label->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side left -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->OptionMenu->GetWidgetName() 
           << " -side left -anchor nw -fill both -expand " 
           << (this->ExpandOptionMenu ? "y" : "n") << endl;
    }
  else
    {
    if (this->ShowLabel)
      {
      tk_cmd << "pack " << this->Label->GetWidgetName() 
             << " -side top -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->OptionMenu->GetWidgetName() 
           << " -side top -anchor nw -padx 10 -fill both -expand "
           << (this->ExpandOptionMenu ? "y" : "n") << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ----------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::SetPackHorizontally(int _arg)
{
  if (this->PackHorizontally == _arg)
    {
    return;
    }
  this->PackHorizontally = _arg;
  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::SetExpandOptionMenu(int _arg)
{
  if (this->ExpandOptionMenu == _arg)
    {
    return;
    }
  this->ExpandOptionMenu = _arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->OptionMenu)
    {
    this->OptionMenu->SetEnabled(this->Enabled);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->OptionMenu)
    {
    this->OptionMenu->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->OptionMenu)
    {
    this->OptionMenu->SetBalloonHelpJustification(j);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabeledOptionMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "OptionMenu: " << this->OptionMenu << endl;

  os << indent << "PackHorizontally: " 
     << (this->PackHorizontally ? "On" : "Off") << endl;

  os << indent << "ExpandOptionMenu: " 
     << (this->ExpandOptionMenu ? "On" : "Off") << endl;
}

