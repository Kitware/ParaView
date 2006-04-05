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
vtkCxxRevisionMacro(vtkKWRadioButton, "1.28");

//----------------------------------------------------------------------------
void vtkKWRadioButton::Create()
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::CreateSpecificTkWidget(
        "radiobutton", "-value 1 -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Configure();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetValue(const char *v)
{
  this->SetConfigurationOption("-value", v);
}

//----------------------------------------------------------------------------
const char* vtkKWRadioButton::GetValue()
{
  return this->GetConfigurationOption("-value");
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetValueAsInt(int v)
{
  this->SetConfigurationOptionAsInt("-value", v);
}

//----------------------------------------------------------------------------
int vtkKWRadioButton::GetValueAsInt()
{
  return this->GetConfigurationOptionAsInt("-value");
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetVariableValue(const char *v)
{
  if (this->IsCreated())
    {
    this->Script("set %s {%s}", this->GetVariableName(), v);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWRadioButton::GetVariableValue()
{
  if (this->IsCreated())
    {
    return this->Script("set %s", this->GetVariableName());
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetVariableValueAsInt(int v)
{
  char buffer[256];
  sprintf(buffer, "%d", v);
  this->SetVariableValue(buffer);
}

//----------------------------------------------------------------------------
int vtkKWRadioButton::GetVariableValueAsInt()
{
  return atoi(this->GetVariableValue());
}

//----------------------------------------------------------------------------
int vtkKWRadioButton::GetSelectedState()
{
  if (this->IsCreated())
    {
#if 0
    return atoi(
       this->Script("expr {${%s}} == {[%s cget -value]}",
                    this->VariableName, this->GetWidgetName()));
#else
    const char* varvalue =
      Tcl_GetVar(
        this->GetApplication()->GetMainInterp(), this->VariableName, TCL_GLOBAL_ONLY);
    const char *value = this->GetConfigurationOption("-value");
    return varvalue && value && !strcmp(varvalue, value);
#endif
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::CommandCallback()
{
  int state = this->GetSelectedState();
  this->InvokeCommand(state);
  this->InvokeEvent(vtkKWRadioButton::SelectedStateChangedEvent, &state);
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::SetCommand(vtkObject *object, const char *method)
{
  this->Superclass::SetCommand(object, method);
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::InvokeCommand(int)
{
  this->InvokeObjectMethodCommand(this->Command);
}

//----------------------------------------------------------------------------
void vtkKWRadioButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

