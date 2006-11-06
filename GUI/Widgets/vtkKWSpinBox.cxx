/*=========================================================================

  Module:    vtkKWSpinBox.cxx,v

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWSpinBox.h"

#include "vtkKWOptions.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWSpinBox);
vtkCxxRevisionMacro(vtkKWSpinBox, "1.21");

//----------------------------------------------------------------------------
vtkKWSpinBox::vtkKWSpinBox() 
{
  this->Command = NULL;
  this->RestrictValue = vtkKWSpinBox::RestrictNone;
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
void vtkKWSpinBox::CreateWidget()
{
  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "spinbox", "-highlightthickness 0 -from 0 -to 10 -increment 1"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, this, "CommandCallback");
  this->SetConfigurationOption("-command", command);
  delete [] command;

  this->SetBinding("<Return>", this, "CommandCallback");
  this->SetBinding("<FocusOut>", this, "CommandCallback");

  this->UpdateValueRestriction();
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
    // Save the old -validate option, which seems to be reset to none
    // whenever the entry was set to something invalid
    vtksys_stl::string old_validate;
    if (this->RestrictValue != vtkKWSpinBox::RestrictNone)
      {
      old_validate = this->GetConfigurationOption("-validate");
      }

    const char *ptr = this->GetValueFormat(), *format;
    char user_format[256];
    if (ptr && *ptr)
      {
      sprintf(user_format, "%%s set %s", ptr);
      format = user_format;
      }
    else
      {
      format = "%s set %g";
      if (this->RestrictValue == vtkKWSpinBox::RestrictInteger)
        {
        value = floor(value);
        }
      }
    this->Script(format, this->GetWidgetName(), value);
    if (this->RestrictValue != vtkKWSpinBox::RestrictNone)
      {
      this->SetConfigurationOption("-validate", old_validate.c_str());
      }
    this->InvokeCommand(this->GetValue());
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
const char* vtkKWSpinBox::GetValueFormat()
{
  return this->GetConfigurationOption("-format");
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
void vtkKWSpinBox::SetRestrictValue(int arg)
{
  if (this->RestrictValue == arg)
    {
    return;
    }

  this->RestrictValue = arg;
  this->Modified();

  this->UpdateValueRestriction();
}

void vtkKWSpinBox::SetRestrictValueToInteger()
{ 
  this->SetRestrictValue(vtkKWSpinBox::RestrictInteger); 
}

void vtkKWSpinBox::SetRestrictValueToDouble()
{ 
  this->SetRestrictValue(vtkKWSpinBox::RestrictDouble); 
}

void vtkKWSpinBox::SetRestrictValueToNone()
{ 
  this->SetRestrictValue(vtkKWSpinBox::RestrictNone); 
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::UpdateValueRestriction()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->RestrictValue == vtkKWSpinBox::RestrictInteger)
    {
    this->SetConfigurationOption("-validate", "all");
    this->SetConfigurationOption("-validatecommand", "string is integer %P");
    }
  else if (this->RestrictValue == vtkKWSpinBox::RestrictDouble)
    {
    this->SetConfigurationOption("-validate", "all");
    this->SetConfigurationOption("-validatecommand", "string is double %P");
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
void vtkKWSpinBox::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::GetDisabledBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetDisabledBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledbackground");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetDisabledBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledbackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::GetReadOnlyBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-readonlybackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetReadOnlyBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-readonlybackground");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetReadOnlyBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-readonlybackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::GetButtonBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-buttonbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetButtonBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-buttonbackground");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetButtonBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-buttonbackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWSpinBox::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWSpinBox::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWSpinBox::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWSpinBox::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWSpinBox::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWSpinBox::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWSpinBox::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWSpinBox::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWSpinBox::SetFont(const char *font)
{
  this->SetConfigurationOption("-font", font);
}

//----------------------------------------------------------------------------
const char* vtkKWSpinBox::GetFont()
{
  return this->GetConfigurationOption("-font");
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
  this->InvokeEvent(vtkKWSpinBox::SpinBoxValueChangedEvent, &value);
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
  os << indent << "RestrictValue: " << this->RestrictValue << endl;
}
