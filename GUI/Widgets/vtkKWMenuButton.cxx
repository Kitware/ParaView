/*=========================================================================

  Module:    vtkKWMenuButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWMenuButton.h"

#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWMenuButton );
vtkCxxRevisionMacro(vtkKWMenuButton, "1.21");

int vtkKWMenuButtonCommand(ClientData cd, Tcl_Interp *interp,
                      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWMenuButton::vtkKWMenuButton()
{
  this->CommandFunction = vtkKWMenuButtonCommand;
  
  this->Menu = vtkKWMenu::New();
}

//----------------------------------------------------------------------------
vtkKWMenuButton::~vtkKWMenuButton()
{
  this->Menu->Delete();
  this->Menu = NULL;
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::Create(vtkKWApplication *app, const char *args)
{ 
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "menubutton", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Menu->SetParent(this);
  this->Menu->Create(app, NULL);  

  this->Script("%s config -menu %s -indicatoron 1 -relief raised -bd 2 "
               "-direction flush %s", 
               this->GetWidgetName(), 
               this->Menu->GetWidgetName(), 
               (args ? args : ""));

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetButtonText(const char *text)
{
  this->SetTextOption(text);
}

//----------------------------------------------------------------------------
const char* vtkKWMenuButton::GetButtonText()
{
  return this->GetTextOption();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::AddCommand(const char* label, vtkKWObject* Object,
                                 const char* MethodAndArgString,
                                 const char* help)
{
  this->Menu->AddCommand(label, Object, MethodAndArgString, help);
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWMenuButton::GetMenu()
{
  return this->Menu;
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::IndicatorOn()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 1", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::IndicatorOff()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 0", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->GetEnabled());

  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

