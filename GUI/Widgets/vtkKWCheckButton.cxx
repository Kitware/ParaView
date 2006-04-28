/*=========================================================================

  Module:    vtkKWCheckButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWIcon.h"
#include "vtkKWOptions.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCheckButton );
vtkCxxRevisionMacro(vtkKWCheckButton, "1.55");

//----------------------------------------------------------------------------
vtkKWCheckButton::vtkKWCheckButton() 
{
  this->IndicatorVisibility = 1;

  this->InternalText = NULL;
  this->VariableName = NULL;
  this->Command      = NULL;
}

//----------------------------------------------------------------------------
vtkKWCheckButton::~vtkKWCheckButton() 
{
  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }

  this->SetInternalText(NULL);
  this->SetVariableName(NULL);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetVariableName(const char* _arg)
{
  if (this->VariableName == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->VariableName && _arg && (!strcmp(this->VariableName, _arg))) 
    { 
    return;
    }

  if (this->VariableName) 
    { 
    delete [] this->VariableName; 
    }

  if (_arg)
    {
    this->VariableName = new char[strlen(_arg)+1];
    strcpy(this->VariableName,_arg);
    }
   else
    {
    this->VariableName = NULL;
    }

  this->Modified();
  
  if (this->VariableName)
    {
    this->SetConfigurationOption("-variable", this->VariableName);
    }
} 

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetIndicatorVisibility(int ind)
{
  if (ind != this->IndicatorVisibility)
    {
    this->IndicatorVisibility = ind;
    this->Modified();
    this->SetConfigurationOptionAsInt(
      "-indicatoron", (this->IndicatorVisibility ? 1 : 0));
    }
  this->SetInternalText(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetText(const char* txt)
{
  this->SetInternalText(txt);
  this->SetTextOption("-text", txt);
}

//----------------------------------------------------------------------------
const char* vtkKWCheckButton::GetText()
{
  return this->InternalText;
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetSelectedState()
{
  if (this->IsCreated() && this->VariableName)
    {
#if 0
    return atoi(
      this->Script("expr {${%s}} == {[%s cget -onvalue]}",
                   this->VariableName, this->GetWidgetName()));
#else
    const char *varvalue = 
      Tcl_GetVar(
        this->GetApplication()->GetMainInterp(), this->VariableName, TCL_GLOBAL_ONLY);
    const char *onvalue = this->GetConfigurationOption("-onvalue");
    return varvalue && onvalue && !strcmp(varvalue, onvalue);
#endif
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetSelectedState(int s)
{
  if (this->IsCreated())
    {
    int was_disabled = !this->GetEnabled();
    if (was_disabled)
      {
      this->SetEnabled(1);
      }

    if (s)
      {
      this->Script("%s select",this->GetWidgetName());
      }
    else
      {
      this->Script("%s deselect",this->GetWidgetName());
      }

    if (was_disabled)
      {
      this->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::ToggleSelectedState()
{
  this->SetSelectedState(this->GetSelectedState() ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::CreateWidget()
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "checkbutton", "-highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Configure();
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::Configure()
{
  char *command = NULL;
  this->SetObjectMethodCommand(&command, this, "CommandCallback");
  this->SetConfigurationOption("-command", command);
  delete [] command;
  
  this->SetConfigurationOptionAsInt(
    "-indicatoron", (this->IndicatorVisibility ? 1 : 0));

  this->SetTextOption("-text", this->InternalText);

  // Set the variable name if not set already

  if (!this->VariableName)
    {
    vtksys_stl::string vname(this->GetWidgetName());
    vname += "Value";
    this->SetVariableName(vname.c_str());
    }
  else
    {
    this->SetConfigurationOption("-variable", this->VariableName);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::CommandCallback()
{
  int state = this->GetSelectedState();
  this->InvokeCommand(state);
  this->InvokeEvent(vtkKWCheckButton::SelectedStateChangedEvent, &state);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetCommand(vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::InvokeCommand(int state)
{
  if (this->GetApplication() &&
      this->Command && *this->Command)
    {
    this->Script("%s %d", this->Command, state);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCheckButton::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCheckButton::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCheckButton::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::GetActiveForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCheckButton::GetActiveForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-activeforeground");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetActiveForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCheckButton::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWCheckButton::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWCheckButton::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWCheckButton::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWCheckButton::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWCheckButton::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWCheckButton::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetPadY(int arg)
{
  this->SetConfigurationOptionAsInt("-pady", arg);
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetPadY()
{
  return this->GetConfigurationOptionAsInt("-pady");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWOptions::GetAnchorAsTkOptionValue(anchor));
}

void vtkKWCheckButton::SetAnchorToNorth() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorth); 
};
void vtkKWCheckButton::SetAnchorToNorthEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorthEast); 
};
void vtkKWCheckButton::SetAnchorToEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorEast); 
};
void vtkKWCheckButton::SetAnchorToSouthEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouthEast); 
};
void vtkKWCheckButton::SetAnchorToSouth() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouth); 
};
void vtkKWCheckButton::SetAnchorToSouthWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouthWest); 
};
void vtkKWCheckButton::SetAnchorToWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorWest); 
};
void vtkKWCheckButton::SetAnchorToNorthWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorthWest); 
};
void vtkKWCheckButton::SetAnchorToCenter() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorCenter); 
};

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetAnchor()
{
  return vtkKWOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetOffRelief(int relief)
{
  this->SetConfigurationOption(
    "-offrelief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWCheckButton::SetOffReliefToRaised()     
{ 
  this->SetOffRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWCheckButton::SetOffReliefToSunken() 
{ 
  this->SetOffRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWCheckButton::SetOffReliefToFlat() 
{ 
  this->SetOffRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWCheckButton::SetOffReliefToRidge() 
{ 
  this->SetOffRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWCheckButton::SetOffReliefToSolid() 
{ 
  this->SetOffRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWCheckButton::SetOffReliefToGroove() 
{ 
  this->SetOffRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetOffRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-offrelief"));
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetOverRelief(int relief)
{
  this->SetConfigurationOption(
    "-overrelief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWCheckButton::SetOverReliefToRaised()     
{ 
  this->SetOverRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWCheckButton::SetOverReliefToSunken() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWCheckButton::SetOverReliefToFlat() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWCheckButton::SetOverReliefToRidge() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWCheckButton::SetOverReliefToSolid() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWCheckButton::SetOverReliefToGroove() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefGroove); 
};
void vtkKWCheckButton::SetOverReliefToNone() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefUnknown); 
};

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetOverRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-overrelief"));
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetFont(const char *font)
{
  this->SetConfigurationOption("-font", font);
}

//----------------------------------------------------------------------------
const char* vtkKWCheckButton::GetFont()
{
  return this->GetConfigurationOption("-font");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageToPredefinedIcon(int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetImageToIcon(icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageToIcon(vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetImageToPixels(
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageToPixels(
  const unsigned char* pixels, 
  int width, 
  int height,
  int pixel_size,
  unsigned long buffer_length)
{
  vtkKWTkUtilities::SetImageOptionToPixels(
    this, pixels, width, height, pixel_size, buffer_length);

  vtkKWTkUtilities::SetImageOptionToPixels(
    this, pixels, width, height, pixel_size, buffer_length, "-selectimage");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::GetSelectColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-selectcolor", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWCheckButton::GetSelectColor()
{
  return this->GetConfigurationOptionAsColor("-selectcolor");
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetSelectColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-selectcolor", r, g, b);
}

// ---------------------------------------------------------------------------
void vtkKWCheckButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VariableName: " 
     << (this->VariableName ? this->VariableName : "None" );
}

