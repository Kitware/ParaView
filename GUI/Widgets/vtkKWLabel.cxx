/*=========================================================================

  Module:    vtkKWLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWLabel.h"

#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWIcon.h"
#include "vtkKWOptions.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWLabel );
vtkCxxRevisionMacro(vtkKWLabel, "1.52");

//----------------------------------------------------------------------------
vtkKWLabel::vtkKWLabel()
{
  this->Text                    = NULL;
  this->AdjustWrapLengthToWidth = 0;
}

//----------------------------------------------------------------------------
vtkKWLabel::~vtkKWLabel()
{
  if (this->Text) 
    { 
    delete [] this->Text; 
    this->Text = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetText(const char* _arg)
{
  if (this->Text == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->Text && _arg && (!strcmp(this->Text, _arg))) 
    {
    return;
    }

  if (this->Text) 
    { 
    delete [] this->Text; 
    }

  if (_arg)
    {
    this->Text = new char[strlen(_arg) + 1];
    strcpy(this->Text, _arg);
    }
  else
    {
    this->Text = NULL;
    }

  this->Modified();

  this->UpdateText();
} 

//----------------------------------------------------------------------------
void vtkKWLabel::UpdateText()
{
  if (this->IsCreated())
    {
    // NULL is handled correctly as ""
    this->SetTextOption("-text", this->Text); 

    // Whatever the label, -image always takes precedence, unless it's empty
    // so change it accordingly
    
    if (this->Text && *this->Text)
      {
      this->SetConfigurationOption("-image", NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::CreateWidget()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "label", "-justify left -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->UpdateText();

  // Set bindings (if any)
  
  this->UpdateBindings();
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWLabel::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetHeight(int height)
{
  this->SetConfigurationOptionAsInt("-height", height);
}

//----------------------------------------------------------------------------
int vtkKWLabel::GetHeight()
{
  return this->GetConfigurationOptionAsInt("-height");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetAdjustWrapLengthToWidth(int v)
{
  if (this->AdjustWrapLengthToWidth == v)
    {
    return;
    }

  this->AdjustWrapLengthToWidth = v;
  this->Modified();

  this->UpdateBindings();
}

//----------------------------------------------------------------------------
void vtkKWLabel::AdjustWrapLengthToWidthCallback()
{
  if (!this->IsCreated() || 
      !this->AdjustWrapLengthToWidth)
    {
    return;
    }

  // Get the widget width and the current wraplength

  int wraplength = atoi(this->GetWrapLength());

  int width;
  vtkKWTkUtilities::GetWidgetSize(this, &width, NULL);

  // Adjust the wraplength to width (within a tolerance so that it does
  // not put too much stress on the GUI).

  if (width < (wraplength - 5) || width > (wraplength + 5))
    {
    this->SetConfigurationOptionAsInt("-wraplength", width - 5);
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetFont(const char *font)
{
  this->SetConfigurationOption("-font", font);
}

//----------------------------------------------------------------------------
const char* vtkKWLabel::GetFont()
{
  return this->GetConfigurationOption("-font");
}

//----------------------------------------------------------------------------
void vtkKWLabel::UpdateBindings()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->AdjustWrapLengthToWidth)
    {
    this->SetBinding("<Configure>", this, "AdjustWrapLengthToWidthCallback");
    }
  else
    {
    this->RemoveBinding("<Configure>");
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetJustification(int justification)
{
  this->SetConfigurationOption(
    "-justify", 
    vtkKWOptions::GetJustificationAsTkOptionValue(justification));
}

void vtkKWLabel::SetJustificationToLeft() 
{ 
  this->SetJustification(vtkKWOptions::JustificationLeft); 
};
void vtkKWLabel::SetJustificationToCenter() 
{ 
  this->SetJustification(vtkKWOptions::JustificationCenter); 
};
void vtkKWLabel::SetJustificationToRight() 
{ 
  this->SetJustification(vtkKWOptions::JustificationRight); 
};

//----------------------------------------------------------------------------
int vtkKWLabel::GetJustification()
{
  return vtkKWOptions::GetJustificationFromTkOptionValue(
    this->GetConfigurationOption("-justify"));
}

//----------------------------------------------------------------------------
void vtkKWLabel::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWLabel::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWLabel::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWLabel::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWLabel::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWLabel::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWLabel::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWLabel::GetActiveForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWLabel::GetActiveForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-activeforeground");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetActiveForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWLabel::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWLabel::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWLabel::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWLabel::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWLabel::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWLabel::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWLabel::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWLabel::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWLabel::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWLabel::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWLabel::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetPadY(int arg)
{
  this->SetConfigurationOptionAsInt("-pady", arg);
}

//----------------------------------------------------------------------------
int vtkKWLabel::GetPadY()
{
  return this->GetConfigurationOptionAsInt("-pady");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetWrapLength(const char *wraplength)
{
  this->SetConfigurationOption("-wraplength", wraplength);
}

//----------------------------------------------------------------------------
const char* vtkKWLabel::GetWrapLength()
{
  return this->GetConfigurationOption("-wraplength");
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWOptions::GetAnchorAsTkOptionValue(anchor));
}

void vtkKWLabel::SetAnchorToNorth() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorth); 
};
void vtkKWLabel::SetAnchorToNorthEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorthEast); 
};
void vtkKWLabel::SetAnchorToEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorEast); 
};
void vtkKWLabel::SetAnchorToSouthEast() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouthEast); 
};
void vtkKWLabel::SetAnchorToSouth() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouth); 
};
void vtkKWLabel::SetAnchorToSouthWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorSouthWest); 
};
void vtkKWLabel::SetAnchorToWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorWest); 
};
void vtkKWLabel::SetAnchorToNorthWest() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorNorthWest); 
};
void vtkKWLabel::SetAnchorToCenter() 
{ 
  this->SetAnchor(vtkKWOptions::AnchorCenter); 
};

//----------------------------------------------------------------------------
int vtkKWLabel::GetAnchor()
{
  return vtkKWOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetImageToPredefinedIcon(int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetImageToIcon(icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetImageToIcon(vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetImageToPixels(
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWLabel::SetImageToPixels(const unsigned char* pixels, 
                                   int width, 
                                   int height,
                                   int pixel_size,
                                   unsigned long buffer_length)
{
  vtkKWTkUtilities::SetImageOptionToPixels(
    this, pixels, width, height, pixel_size, buffer_length);
}

//---------------------------------------------------------------------------
void vtkKWLabel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AdjustWrapLengthToWidth: " 
     << (this->AdjustWrapLengthToWidth ? "On" : "Off") << endl;
  os << indent << "Text: ";
  if (this->Text)
    {
    os << this->Text << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
}

