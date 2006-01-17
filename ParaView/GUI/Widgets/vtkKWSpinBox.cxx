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

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWSpinBox);
vtkCxxRevisionMacro(vtkKWSpinBox, "1.11");

//----------------------------------------------------------------------------
vtkKWSpinBox::vtkKWSpinBox() 
{
  this->Command = NULL;
}

//----------------------------------------------------------------------------
vtkKWSpinBox::~vtkKWSpinBox() 
{
  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::Create()
{
  if (!this->Superclass::CreateSpecificTkWidget("spinbox"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, this, "CommandCallback");
  this->SetConfigurationOption("-command", command);
  delete [] command;
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetRange(double from, double to)
{
  if (this->IsCreated())
    {
    // both options have to be set at the same time to avoid error if
    // -from/-to is greater/lower the -to/-from
    this->Script("%s configure -from %lf -to %lf", 
                 this->GetWidgetName(), from, to);
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetIncrement(double increment)
{
  this->SetConfigurationOptionAsDouble("-increment", increment);
}

//----------------------------------------------------------------------------
double vtkKWSpinBox::GetIncrement()
{
  return this->GetConfigurationOptionAsDouble("-increment");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetValue(double value)
{
  if (this->IsCreated())
    {
    this->Script("%s set %lf", this->GetWidgetName(), value);
    }
}

//----------------------------------------------------------------------------
double vtkKWSpinBox::GetValue()
{
  return atof(this->Script("%s get", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetValueFormat(const char *arg)
{
  this->SetConfigurationOption("-format", arg);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetWrap(int arg)
{
  this->SetConfigurationOptionAsInt("-wrap", arg);
}

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetWrap()
{
  return this->GetConfigurationOptionAsInt("-wrap");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetRestrictValuesToIntegers(int restrict)
{
  if (restrict)
    {
    this->SetConfigurationOption("-validate", "key");
    this->SetConfigurationOption("-validatecommand", "string is integer %%P");
    }
  else
    {
    this->SetConfigurationOption("-validate", "none");
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetWidth(int arg)
{
  this->SetConfigurationOptionAsInt("-width", arg);
}

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
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
void vtkKWSpinBox::CommandCallback()
{
  this->InvokeCommand(this->GetValue());
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetCommand(vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::InvokeCommand(double value)
{
  if (this->Command && *this->Command && this->GetApplication())
    {
    // As a convenience, try to detect if we are manipulating integers, and
    // invoke the callback with the approriate type.
    double increment = this->GetIncrement();
    if ((double)((long int)increment) == increment)
      {
      this->Script("%s %ld", this->Command, (long int)value);
      }
    else
      {
      this->Script("%s %lf", this->Command, value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
