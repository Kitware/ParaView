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
vtkCxxRevisionMacro(vtkKWSpinBox, "1.6");

//----------------------------------------------------------------------------
vtkKWSpinBox::vtkKWSpinBox()
{
}

//----------------------------------------------------------------------------
vtkKWSpinBox::~vtkKWSpinBox()
{
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::Create(vtkKWApplication *app)
{
  if (!this->Superclass::CreateSpecificTkWidget(app, "spinbox"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

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
void vtkKWSpinBox::SetCommand(vtkObject *object, const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    this->SetConfigurationOption("-command", command);
    delete [] command;
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
