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
#include "vtkKWMenuButton.h"
#include "vtkKWPushButton.h"
#include "vtkKWIcon.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWPushButtonWithMenu );
vtkCxxRevisionMacro(vtkKWPushButtonWithMenu, "1.10");

//----------------------------------------------------------------------------
vtkKWPushButtonWithMenu::vtkKWPushButtonWithMenu()
{
  this->PushButton = vtkKWPushButton::New();
  this->MenuButton = vtkKWMenuButton::New();
}

//----------------------------------------------------------------------------
vtkKWPushButtonWithMenu::~vtkKWPushButtonWithMenu()
{
  if (this->PushButton)
    {
    this->PushButton->Delete();
    this->PushButton = NULL;
    }

  if (this->MenuButton)
    {
    this->MenuButton->Delete();
    this->MenuButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::CreateWidget()
{ 
  // Call the superclass to create the widget and set the appropriate flags

  this->Superclass::CreateWidget();

  this->PushButton->SetParent(this);
  this->PushButton->Create();  

  this->Script("pack %s -side left -expand y -fill both",
               this->PushButton->GetWidgetName());

  this->MenuButton->SetParent(this);
  this->MenuButton->Create();
  this->MenuButton->IndicatorVisibilityOff();
  this->MenuButton->SetImageToPredefinedIcon(vtkKWIcon::IconExpandMini);

  this->Script("pack %s -side left -expand y -fill y",
               this->MenuButton->GetWidgetName());
}
  
//----------------------------------------------------------------------------
vtkKWMenu* vtkKWPushButtonWithMenu::GetMenu()
{
  if (this->MenuButton)
    {
    return this->MenuButton->GetMenu();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->PushButton);
  this->PropagateEnableState(this->MenuButton);
}

//----------------------------------------------------------------------------
void vtkKWPushButtonWithMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

