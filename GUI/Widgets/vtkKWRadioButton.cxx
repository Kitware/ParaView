/*=========================================================================

  Module:    vtkKWRadioButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWRadioButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWRadioButton );
vtkCxxRevisionMacro(vtkKWRadioButton, "1.14");

//----------------------------------------------------------------------------
void vtkKWRadioButton::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // must set the application
  if (this->IsCreated())
    {
    vtkErrorMacro("RadioButton already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("radiobutton %s -value 1", wname);
  this->Configure();
  this->Script("%s configure %s", wname, (args?args:""));

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetValue(int v)
{
  this->Script("%s configure -value %d", this->GetWidgetName(), v);
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetValue(const char *v)
{
  this->Script("%s configure -value %s", this->GetWidgetName(), v);
}

//----------------------------------------------------------------------------
int vtkKWRadioButton::GetState()
{
  if (this->IsCreated())
    {
    this->Script("expr {${%s}} == {[%s cget -value]}",
                 this->VariableName, this->GetWidgetName());
    return vtkKWObject::GetIntegerResult(this->GetApplication());
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

