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
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"
#include "vtkKWTkUtilities.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWPushButton );
vtkCxxRevisionMacro(vtkKWPushButton, "1.30");

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

  if (!this->Superclass::CreateSpecificTkWidget("button"))
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
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWPushButton::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWPushButton::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWPushButton::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWPushButton::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWPushButton::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWPushButton::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWPushButton::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
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
    "-anchor", vtkKWTkOptions::GetAnchorAsTkOptionValue(anchor));
}

void vtkKWPushButton::SetAnchorToNorth() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorNorth); 
};
void vtkKWPushButton::SetAnchorToNorthEast() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorNorthEast); 
};
void vtkKWPushButton::SetAnchorToEast() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorEast); 
};
void vtkKWPushButton::SetAnchorToSouthEast() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorSouthEast); 
};
void vtkKWPushButton::SetAnchorToSouth() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorSouth); 
};
void vtkKWPushButton::SetAnchorToSouthWest() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorSouthWest); 
};
void vtkKWPushButton::SetAnchorToWest() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorWest); 
};
void vtkKWPushButton::SetAnchorToNorthWest() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorNorthWest); 
};
void vtkKWPushButton::SetAnchorToCenter() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorCenter); 
};

//----------------------------------------------------------------------------
void vtkKWPushButton::SetOverRelief(int relief)
{
  this->SetConfigurationOption(
    "-overrelief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWPushButton::SetOverReliefToRaised()     
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWPushButton::SetOverReliefToSunken() 
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWPushButton::SetOverReliefToFlat() 
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWPushButton::SetOverReliefToRidge() 
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWPushButton::SetOverReliefToSolid() 
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWPushButton::SetOverReliefToGroove() 
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefGroove); 
};
void vtkKWPushButton::SetOverReliefToNone() 
{ 
  this->SetOverRelief(vtkKWTkOptions::ReliefUnknown); 
};

//----------------------------------------------------------------------------
int vtkKWPushButton::GetOverRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-overrelief"));
}

//----------------------------------------------------------------------------
int vtkKWPushButton::GetAnchor()
{
  return vtkKWTkOptions::GetAnchorFromTkOptionValue(
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
