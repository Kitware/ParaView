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
vtkCxxRevisionMacro(vtkKWRadioButton, "1.16");

//----------------------------------------------------------------------------
void vtkKWRadioButton::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "radiobutton", "-value 1"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Configure();
  this->ConfigureOptions(args);

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetValue(int v)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -value %d", this->GetWidgetName(), v);
    }
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetValue(const char *v)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -value %s", this->GetWidgetName(), v);
    }
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

