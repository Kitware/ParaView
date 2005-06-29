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
vtkCxxRevisionMacro(vtkKWSpinBox, "1.2");

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
  char format[1024];
  char script[1024];
  sprintf(format, "%s", this->GetConfigurationOption("-format"));
  sprintf(script, "%%s configure -from %s -to %s", format, format);
  this->Script(script, this->GetWidgetName(), from, to);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetIncrement(double increment)
{
  char format[1024];
  char script[1024];
  sprintf(format, "%s", this->GetConfigurationOption("-format"));
  sprintf(script, "%%s configure -increment %s", format);
  this->Script(script, this->GetWidgetName(), increment);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetValue(double value)
{
  char format[1024];
  char script[1024];
  sprintf(format, "%s", this->GetConfigurationOption("-format"));
  sprintf(script, "%%s set %s", format);
  this->Script(script, this->GetWidgetName(), value);
}

//----------------------------------------------------------------------------
double vtkKWSpinBox::GetValue()
{
  char buffer[1024];
  sprintf(buffer, "%s", this->Script("%s get", this->GetWidgetName()));
  return atof(buffer);
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
    this->Script("%s configure -validate key -validatecommand {string is integer %%P}",
      this->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -validate none",
      this->GetWidgetName());
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
