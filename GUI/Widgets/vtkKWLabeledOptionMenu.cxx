/*=========================================================================

  Module:    vtkKWLabeledOptionMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWLabeledOptionMenu.h"

#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWLabeledOptionMenu);
vtkCxxRevisionMacro(vtkKWLabeledOptionMenu, "1.11");

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

  this->OptionMenu->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  if (this->PackHorizontally)
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
             << " -side left -anchor nw" << endl;
      }
    tk_cmd << "pack " << this->OptionMenu->GetWidgetName() 
           << " -side left -anchor nw -fill both -expand " 
           << (this->ExpandOptionMenu ? "y" : "n") << endl;
    }
  else
    {
    if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
      {
      tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
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

