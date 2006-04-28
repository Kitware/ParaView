/*=========================================================================

  Module:    vtkKWEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWEntry.h"

#include "vtkKWOptions.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWEntry);
vtkCxxRevisionMacro(vtkKWEntry, "1.80");

//----------------------------------------------------------------------------
vtkKWEntry::vtkKWEntry()
{
  this->Width               = -1;
  this->ReadOnly            = 0;
  this->InternalValueString = NULL;
  this->Command             = NULL;
}

//----------------------------------------------------------------------------
vtkKWEntry::~vtkKWEntry()
{
  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }
  this->SetInternalValueString(NULL);
}

//----------------------------------------------------------------------------
void vtkKWEntry::CreateWidget()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "entry", "-highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Configure();
}

//----------------------------------------------------------------------------
void vtkKWEntry::Configure()
{
  this->SetBinding("<Return>", this, "ValueCallback");
  this->SetBinding("<FocusOut>", this, "ValueCallback");

  if (this->Width >= 0)
    {
    this->SetConfigurationOptionAsInt("-width", this->Width);
    }
}

//----------------------------------------------------------------------------
const char* vtkKWEntry::GetValue()
{
  if (!this->IsCreated())
    {
    return NULL;
    }

  const char *val = this->Script("%s get", this->GetWidgetName());
  this->SetInternalValueString(this->ConvertTclStringToInternalString(val));
  return this->GetInternalValueString();
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetValueAsInt()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // Do not call this->GetValue() here to speed up things (GetValue() copies
  // the buffer to a string each time, for safety reasons)

  const char *val = this->Script("%s get", this->GetWidgetName());
  if (!val || !*val)
    {
    return 0;
    }
  return atoi(val);
}

//----------------------------------------------------------------------------
double vtkKWEntry::GetValueAsDouble()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  // Do not call this->GetValue() here to speed up things (GetValue() copies
  // the buffer to a string each time, for safety reasons)

  const char *val = this->Script("%s get", this->GetWidgetName());
  if (!val || !*val)
    {
    return 0;
    }
  return atof(val);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(const char *s)
{
  if (!this->IsAlive())
    {
    return;
    }

  int old_state = this->GetState();
  this->SetStateToNormal();

  this->Script("%s delete 0 end", this->GetWidgetName());
  if (s)
    {
    const char *val = this->ConvertInternalStringToTclString(
      s, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    this->Script("%s insert 0 \"%s\"", 
                 this->GetWidgetName(), val ? val : "");
    }

  this->SetState(old_state);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValueAsInt(int i)
{
  if (!this->IsCreated())
    {
    return;
    }

  // Do not call this->GetValue() here to speed up things (GetValue() copies
  // the buffer to a string each time, for safety reasons)

  const char *val = this->Script("%s get", this->GetWidgetName());
  if (val && *val && i == atoi(val))
    {
    return;
    }

  char tmp[1024];
  sprintf(tmp, "%d", i);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValueAsDouble(double f)
{
  if (!this->IsCreated())
    {
    return;
    }

  // Do not call this->GetValue() here to speed up things (GetValue() copies
  // the buffer to a string each time, for safety reasons)

  const char *val = this->Script("%s get", this->GetWidgetName());
  if (val && *val && f == atof(val))
    {
    return;
    }

  char tmp[1024];
  sprintf(tmp, "%.5g", f);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValueAsFormattedDouble(double f, int size)
{
  if (!this->IsCreated())
    {
    return;
    }

  // Do not call this->GetValue() here to speed up things (GetValue() copies
  // the buffer to a string each time, for safety reasons)

  const char *val = this->Script("%s get", this->GetWidgetName());
  if (val && *val && f == atof(val))
    {
    return;
    }

  char format[1024];
  sprintf(format, "%%.%dg", size);

  char tmp[1024];
  sprintf(tmp, format, f);

  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetReadOnly(int arg)
{
  if (this->ReadOnly == arg)
    {
    return;
    }

  this->ReadOnly = arg;
  this->Modified();
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Width = width;
  this->Modified();

  if (this->IsCreated() && this->Width >= 0)
    {
    this->SetConfigurationOptionAsInt("-width", this->Width);
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWEntry::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWEntry::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWEntry::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWEntry::GetDisabledBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledbackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWEntry::GetDisabledBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledbackground");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetDisabledBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledbackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWEntry::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWEntry::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWEntry::GetReadOnlyBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-readonlybackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWEntry::GetReadOnlyBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-readonlybackground");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetReadOnlyBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-readonlybackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWEntry::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWEntry::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWEntry::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWEntry::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWEntry::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWEntry::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWEntry::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetFont(const char *font)
{
  this->SetConfigurationOption("-font", font);
}

//----------------------------------------------------------------------------
const char* vtkKWEntry::GetFont()
{
  return this->GetConfigurationOption("-font");
}

//----------------------------------------------------------------------------
void vtkKWEntry::ValueCallback()
{
  this->InvokeCommand(this->GetValue());
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetCommand(vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWEntry::InvokeCommand(const char *value)
{
  if (this->Command && *this->Command && this->GetApplication())
    {
    const char *val = this->ConvertInternalStringToTclString(
      value, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
    this->Script("%s \"%s\"", this->Command, val ? val : "");
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->GetEnabled() && this->ReadOnly)
    {
    this->SetStateToReadOnly();
    }
  else
    {
    this->SetState(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Width: " << this->GetWidth() << endl;
  os << indent << "Readonly: " << (this->ReadOnly ? "On" : "Off") << endl;
}

