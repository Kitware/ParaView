/*=========================================================================

  Module:    vtkKWPushButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWPushButton );
vtkCxxRevisionMacro(vtkKWPushButton, "1.16");

//----------------------------------------------------------------------------
vtkKWPushButton::vtkKWPushButton()
{
  this->ButtonLabel = 0;
}

//----------------------------------------------------------------------------
vtkKWPushButton::~vtkKWPushButton()
{
  this->SetButtonLabel(0);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("PushButton already created");
    return;
    }

  this->SetApplication(app);

  // Create the button

  wname = this->GetWidgetName();

  this->Script("button %s", wname);

  this->SetTextOption(this->ButtonLabel);

  if (args && *args)
    {
    this->Script("%s config %s", wname, args);
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetLabel( const char *name )
{
  this->SetButtonLabel(name);
  this->SetTextOption(name);
}

//----------------------------------------------------------------------------
char* vtkKWPushButton::GetLabel()
{
  return this->ButtonLabel;
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetLabelWidth(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetLabelWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
