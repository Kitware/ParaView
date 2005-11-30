/*=========================================================================

  Module:    vtkKWPushButtonWithMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWPushButtonWithMenu.h"

#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWPushButtonWithMenu );
vtkCxxRevisionMacro(vtkKWPushButtonWithMenu, "1.6");

//----------------------------------------------------------------------------
vtkKWPushButtonWithMenu::vtkKWPushButtonWithMenu()
{
  this->Menu = vtkKWMenu::New();
}

//----------------------------------------------------------------------------
vtkKWPushButtonWithMenu::~vtkKWPushButtonWithMenu()
{
  this->Menu->Delete();
  this->Menu = NULL;
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::Create(vtkKWApplication *app)
{ 
  // Call the superclass to create the widget and set the appropriate flags

  this->Superclass::Create(app);
  this->Menu->SetParent(this);
  this->Menu->Create(app);  

  this->SetBinding("<ButtonPress-3>", this, "PopupCallback %X %Y");
}
  
  
//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::PopupCallback(int x, int y)
{ 
  this->Menu->PopUp(x, y);
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::AddCheckButton(const char* label, 
                                             const char* varName,
                                             int defaultState,
                                             const char* help)
{
  char* buttonVar = this->Menu->CreateCheckButtonVariable(this, varName);
  this->Menu->AddCheckButton(label, buttonVar, 0, "", help);
  this->Script("set %s %d",buttonVar,defaultState);  
  delete [] buttonVar;
}

//----------------------------------------------------------------------------
int vtkKWPushButtonWithMenu::GetCheckButtonState(const char* varName)
{
  return this->Menu->GetCheckButtonValue(this, varName);
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::SetCheckButtonState(const char* varName, 
                                                  int state)
{
  this->Menu->CheckCheckButton(this, varName, state);
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWPushButtonWithMenu::GetMenu()
{
  return this->Menu;
}


//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

