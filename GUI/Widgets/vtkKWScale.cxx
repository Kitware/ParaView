/*=========================================================================

  Module:    vtkKWScale.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWScale.h"

#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWScale );
vtkCxxRevisionMacro(vtkKWScale, "1.116");

//----------------------------------------------------------------------------
vtkKWScale::vtkKWScale()
{
  this->Value      = 0;
  this->Range[0]   = 0;
  this->Range[1]   = 100;  
  this->Resolution = 1;

  this->Orientation = vtkKWTkOptions::OrientationHorizontal;
  this->Command      = NULL;
  this->StartCommand = NULL;
  this->EndCommand   = NULL;

  this->ClampValue      = 1;
  this->DisableCommands = 0;

  this->DisableScaleValueCallback = 1;
}

//----------------------------------------------------------------------------
vtkKWScale::~vtkKWScale()
{
  if (this->IsAlive())
    {
    this->UnBind();
    }

  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }

  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    this->StartCommand = NULL;
    }

  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    this->EndCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWScale::Create()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(
        "scale", "-highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->UpdateOrientation();
  this->UpdateResolution();
  this->UpdateRange();
  this->UpdateValue();
  
  this->Bind();
}

//----------------------------------------------------------------------------
void vtkKWScale::UpdateOrientation()
{
  if (this->IsCreated())
    {
    this->SetConfigurationOption(
      "-orient", vtkKWTkOptions::GetOrientationAsTkOptionValue(
        this->Orientation));
    }
}

//----------------------------------------------------------------------------
void vtkKWScale::SetOrientation(int orientation)
{
  if (this->Orientation == orientation ||
      (orientation != vtkKWTkOptions::OrientationHorizontal &&
       orientation != vtkKWTkOptions::OrientationVertical))
    {
    return;
    }
      
  this->Orientation = orientation;
  this->Modified();

  this->UpdateOrientation();
}

void vtkKWScale::SetOrientationToHorizontal() 
{ 
  this->SetOrientation(vtkKWTkOptions::OrientationHorizontal); 
};
void vtkKWScale::SetOrientationToVertical() 
{ 
  this->SetOrientation(vtkKWTkOptions::OrientationVertical); 
};

//----------------------------------------------------------------------------
void vtkKWScale::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScale::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScale::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScale::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScale::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWScale::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWScale::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScale::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScale::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWScale::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWScale::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWScale::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWScale::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWScale::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWScale::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWScale::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWScale::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWScale::GetTroughColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-troughcolor", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScale::GetTroughColor()
{
  return this->GetConfigurationOptionAsColor("-troughcolor");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetTroughColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-troughcolor", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScale::Bind()
{
  this->SetBinding("<ButtonPress>", this, "ButtonPressCallback");
  this->SetBinding("<ButtonRelease>", this, "ButtonReleaseCallback");

  this->AddBinding("<ButtonPress>", this, "DisableScaleValueCallbackOff");
  this->AddBinding("<ButtonRelease>", this, "DisableScaleValueCallbackOn");

  char *command = NULL;
  this->SetObjectMethodCommand(&command, this, "ScaleValueCallback");
  this->SetConfigurationOption("-command", command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWScale::UnBind()
{
  this->RemoveBinding("<ButtonPress>");
  this->RemoveBinding("<ButtonRelease>");

  this->SetConfigurationOption("-command", NULL);
}

//----------------------------------------------------------------------------
void vtkKWScale::SetResolution(double r)
{
  if (this->Resolution == r)
    {
    return;
    }

  this->Resolution = r;
  this->Modified();

  this->UpdateResolution();
}

//----------------------------------------------------------------------------
void vtkKWScale::UpdateResolution()
{
  if (this->IsCreated())
    {
    this->SetConfigurationOptionAsDouble("-resolution", this->Resolution);
    }
}

//----------------------------------------------------------------------------
void vtkKWScale::SetValue(double num)
{
  if (this->ClampValue)
    {
    if (this->Range[1] > this->Range[0])
      {
      if (num > this->Range[1]) 
        { 
        num = this->Range[1]; 
        }
      else if (num < this->Range[0])
        {
        num = this->Range[0];
        }
      }
    else
      {
      if (num < this->Range[1]) 
        { 
        num = this->Range[1]; 
        }
      else if (num > this->Range[0])
        {
        num = this->Range[0];
        }
      }
    }

  if (this->Value == num)
    {
    return;
    }

  this->Value = num;
  this->Modified();

  this->UpdateValue();

  this->InvokeCommand(this->GetValue());
}

//----------------------------------------------------------------------------
void vtkKWScale::UpdateValue()
{
  if (!this->IsCreated())
    {
    return;
    }

  int was_disabled = !this->GetEnabled();
  if (was_disabled)
    {
    this->SetState(vtkKWTkOptions::StateNormal);
    this->SetEnabled(1);
    }

  this->Script("%s set %g", this->GetWidgetName(), this->Value);

  if (was_disabled)
    {
    this->SetState(vtkKWTkOptions::StateDisabled);
    this->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWScale::SetRange(double min, double max)
{
  if (this->Range[0] == min && this->Range[1] == max)
    {
    return;
    }

  this->Range[0] = min;
  this->Range[1] = max;

  this->Modified();

  this->UpdateRange();
}

//----------------------------------------------------------------------------
void vtkKWScale::UpdateRange()
{
  if (this->IsCreated())
    {
    this->SetConfigurationOptionAsDouble("-from", this->Range[0]);
    this->SetConfigurationOptionAsDouble("-to", this->Range[1]);
    }
}

//----------------------------------------------------------------------------
void vtkKWScale::ScaleValueCallback(double num)
{
  if (this->DisableScaleValueCallback)
    {
    return;
    }

  this->SetValue(num);
}

//----------------------------------------------------------------------------
void vtkKWScale::ButtonPressCallback()
{
  this->InvokeStartCommand(this->GetValue());
}

//----------------------------------------------------------------------------
void vtkKWScale::ButtonReleaseCallback()
{
  this->InvokeEndCommand(this->GetValue());
}

//----------------------------------------------------------------------------
void vtkKWScale::InvokeScaleCommand(const char *command, double value)
{
  if (!this->DisableCommands && command && *command && this->GetApplication())
    {
    // As a convenience, try to detect if we are manipulating integers, and
    // invoke the callback with the approriate type.
    if ((double)((long int)this->Resolution) == this->Resolution)
      {
      this->Script("%s %ld", command, (long int)value);
      }
    else
      {
      this->Script("%s %lf", command, value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWScale::SetCommand(vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWScale::InvokeCommand(double value)
{
  this->InvokeScaleCommand(this->Command, value);
  this->InvokeEvent(vtkKWScale::ScaleValueChangingEvent, &value);
}

//----------------------------------------------------------------------------
void vtkKWScale::SetStartCommand(vtkObject *object, const char * method)
{
  this->SetObjectMethodCommand(&this->StartCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWScale::InvokeStartCommand(double value)
{
  this->InvokeScaleCommand(this->StartCommand, value);
  this->InvokeEvent(vtkKWScale::ScaleValueStartChangingEvent, &value);
}

//----------------------------------------------------------------------------
void vtkKWScale::SetEndCommand(vtkObject *object, const char * method)
{
  this->SetObjectMethodCommand(&this->EndCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWScale::InvokeEndCommand(double value)
{
  this->InvokeScaleCommand(this->EndCommand, value);
  this->InvokeEvent(vtkKWScale::ScaleValueChangedEvent, &value);
}

//----------------------------------------------------------------------------
void vtkKWScale::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWScale::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWScale::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetLength(int length)
{
  this->SetConfigurationOptionAsInt("-length", length);
}

//----------------------------------------------------------------------------
int vtkKWScale::GetLength()
{
  return this->GetConfigurationOptionAsInt("-length");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetSliderLength(int length)
{
  this->SetConfigurationOptionAsInt("-sliderlength", length);
}

//----------------------------------------------------------------------------
int vtkKWScale::GetSliderLength()
{
  return this->GetConfigurationOptionAsInt("-sliderlength");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetValueVisibility(int val)
{
  this->SetConfigurationOptionAsInt("-showvalue", val);
}

//----------------------------------------------------------------------------
int vtkKWScale::GetValueVisibility()
{
  return this->GetConfigurationOptionAsInt("-showvalue");
}

//----------------------------------------------------------------------------
void vtkKWScale::SetTickInterval(double val)
{
  this->SetConfigurationOptionAsDouble("-tickinterval", val);
}

//----------------------------------------------------------------------------
double vtkKWScale::GetTickInterval()
{
  return this->GetConfigurationOptionAsDouble("-tickinterval");
}

//---------------------------------------------------------------------------
void vtkKWScale::SetLabelText(const char *label)
{
  this->SetTextOption("-label", label); 
}

//---------------------------------------------------------------------------
const char* vtkKWScale::GetLabelText()
{
  return this->GetTextOption("-label"); 
}

//----------------------------------------------------------------------------
void vtkKWScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Value: " << this->Value << endl;
  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "Range: " << this->Range[0] << "..." <<  this->Range[1] << endl;
  os << indent << "DisableCommands: "
     << (this->DisableCommands ? "On" : "Off") << endl;
  os << indent << "ClampValue: " << (this->ClampValue ? "On" : "Off") << endl;
  os << indent << "DisableScaleValueCallback: " << (this->DisableScaleValueCallback ? "On" : "Off") << endl;
}
