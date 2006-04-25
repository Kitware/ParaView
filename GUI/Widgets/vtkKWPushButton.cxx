/*=========================================================================

  Module:    vtkKWPushButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWPushButton.h"

#include "vtkKWOptions.h"
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"
#include "vtkKWTkUtilities.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWPushButton );
vtkCxxRevisionMacro(vtkKWPushButton, "1.32");

//----------------------------------------------------------------------------
vtkKWPushButton::vtkKWPushButton()
{
  this->ButtonText = 0;
  this->Command      = NULL;
}

//----------------------------------------------------------------------------
vtkKWPushButton::~vtkKWPushButton()
{
  this->SetButtonText(0);

  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWPushButton::Create()
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::CreateSpecificTkWidget(
        "button", "-highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetTextOption("-text", this->ButtonText);

  char *command = NULL;
  this->SetObjectMethodCommand(&command, this, "CommandCallback");
  this->SetConfigurationOption("-command", command);
  delete [] command;
  
  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetText( const char *name )
{
  this->SetButtonText(name);
  this->SetTextOption("-text", name);
}

//----------------------------------------------------------------------------
char* vtkKWPushButton::GetText()
{
  return this->ButtonText;
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetHeight(int height)
{
  this->SetConfigurationOptionAsInt("-height", height);
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWPushButton::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWPushButton::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWPushButton::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::GetActiveForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWPushButton::GetActiveForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-activeforeground");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetActiveForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWPushButton::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWPushButton::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWPushButton::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWPushButton::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWPushButton::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWPushButton::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWPushButton::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWPushButton::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetPadY(int arg)
{
  this->SetConfigurationOptionAsInt("-pady", arg);
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetPadY()
{
  return this->GetConfigurationOptionAsInt("-pady");
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWOptions::GetAnchorAsTkOptionValue(anchor));
}

void vtkKWPushButton::SetAnchorToNorth() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorth); 
};
void vtkKWPushButton::SetAnchorToNorthEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorthEast); 
};
void vtkKWPushButton::SetAnchorToEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorEast); 
};
void vtkKWPushButton::SetAnchorToSouthEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouthEast); 
};
void vtkKWPushButton::SetAnchorToSouth() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouth); 
};
void vtkKWPushButton::SetAnchorToSouthWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouthWest); 
};
void vtkKWPushButton::SetAnchorToWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorWest); 
};
void vtkKWPushButton::SetAnchorToNorthWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorthWest); 
};
void vtkKWPushButton::SetAnchorToCenter() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorCenter); 
};

//----------------------------------------------------------------------------
void vtkKWPushButton::SetOverRelief(int relief)
{
  this->SetConfigurationOption(
    "-overrelief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWPushButton::SetOverReliefToRaised()     
{ 
  this->SetOverRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWPushButton::SetOverReliefToSunken() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWPushButton::SetOverReliefToFlat() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWPushButton::SetOverReliefToRidge() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWPushButton::SetOverReliefToSolid() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWPushButton::SetOverReliefToGroove() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefGroove); 
};
void vtkKWPushButton::SetOverReliefToNone() 
{ 
  this->SetOverRelief(vtkKWOptions::ReliefUnknown); 
};

//----------------------------------------------------------------------------
int vtkKWPushButton::GetOverRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-overrelief"));
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetAnchor()
{
  return vtkKWOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetImageToPredefinedIcon(int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetImageToIcon(icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetImageToIcon(vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetImageToPixels(
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetImageToPixels(const unsigned char* pixels, 
                                       int width, 
                                       int height,
                                       int pixel_size,
                                       unsigned long buffer_length)
{
  vtkKWTkUtilities::SetImageOptionToPixels(
    this, pixels, width, height, pixel_size, buffer_length);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::CommandCallback()
{
  this->InvokeCommand();
  this->InvokeEvent(vtkKWPushButton::InvokedEvent);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::SetCommand(vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::InvokeCommand()
{
  this->InvokeObjectMethodCommand(this->Command);
}

//----------------------------------------------------------------------------
void vtkKWPushButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWPushButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
