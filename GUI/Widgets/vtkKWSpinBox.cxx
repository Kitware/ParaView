/*=========================================================================

  Module:    vtkKWSpinBox.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSpinBox.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWSpinBox);
vtkCxxRevisionMacro(vtkKWSpinBox, "1.1");

//----------------------------------------------------------------------------
int vtkKWSpinBoxCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWSpinBox::vtkKWSpinBox()
{
  this->CommandFunction = vtkKWSpinBoxCommand;
}

//----------------------------------------------------------------------------
vtkKWSpinBox::~vtkKWSpinBox()
{
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::Create(vtkKWApplication *app)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(app, "spinbox"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetRange(int from, int to, int increment, int autoenable)
{
  this->Script("%s configure -from %d -to %d -increment %d",
    this->GetWidgetName(), from, to, increment);

  // When using the integer overload of SetRange, automatically:
  this->RestrictValuesToIntegers();
  this->SetValue(from);

  // Update Enabled based on range:
  if (autoenable)
    {
    this->SetEnabled(to>from ? 1 : 0);
    this->UpdateEnableState();
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetValue(int value)
{
  this->Script("%s set %d", this->GetWidgetName(), value);
}

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetValue()
{
  char buffer[1024];
  sprintf(buffer, "%s", this->Script("%s get", this->GetWidgetName()));
  return atoi(buffer);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::RestrictValuesToIntegers()
{
  this->Script("%s configure -validate key -validatecommand {string is integer %%P}",
    this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetWidth(int w)
{
  this->Script("%s configure -width %d", this->GetWidgetName(), w);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetExportSelection(int arg)
{
  this->SetConfigurationOptionAsInt("-exportselection", arg);
}

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetExportSelection()
{
  return this->GetConfigurationOptionAsInt("-exportselection");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
