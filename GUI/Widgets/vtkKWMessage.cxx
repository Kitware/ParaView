/*=========================================================================

  Module:    vtkKWMessage.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMessage.h"
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMessage );
vtkCxxRevisionMacro(vtkKWMessage, "1.4");

//----------------------------------------------------------------------------
vtkKWMessage::vtkKWMessage()
{
  this->Text = NULL;
}

//----------------------------------------------------------------------------
vtkKWMessage::~vtkKWMessage()
{
  if (this->Text) 
    { 
    delete [] this->Text; 
    this->Text = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetText(const char* _arg)
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
void vtkKWMessage::UpdateText()
{
  if (this->IsCreated())
    {
    // NULL is handled correctly as ""
    this->SetTextOption("-text", this->Text); 
    }
}

//----------------------------------------------------------------------------
void vtkKWMessage::Create()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(
        "message", "-justify left -highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->UpdateText();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMessage::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMessage::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMessage::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMessage::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWMessage::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWMessage::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWMessage::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWMessage::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWMessage::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWMessage::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWMessage::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetPadX(int arg)
{
  this->SetConfigurationOptionAsInt("-padx", arg);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetPadX()
{
  return this->GetConfigurationOptionAsInt("-padx");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetPadY(int arg)
{
  this->SetConfigurationOptionAsInt("-pady", arg);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetPadY()
{
  return this->GetConfigurationOptionAsInt("-pady");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetAspectRatio(int aspect)
{
  this->SetConfigurationOptionAsInt("-aspect", aspect);
}

//----------------------------------------------------------------------------
int vtkKWMessage::GetAspectRatio()
{
  return this->GetConfigurationOptionAsInt("-aspect");
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetJustification(int justification)
{
  this->SetConfigurationOption(
    "-justify", 
    vtkKWTkOptions::GetJustificationAsTkOptionValue(justification));
}

void vtkKWMessage::SetJustificationToLeft() 
{ 
  this->SetJustification(vtkKWTkOptions::JustificationLeft); 
};
void vtkKWMessage::SetJustificationToCenter() 
{ 
  this->SetJustification(vtkKWTkOptions::JustificationCenter); 
};
void vtkKWMessage::SetJustificationToRight() 
{ 
  this->SetJustification(vtkKWTkOptions::JustificationRight); 
};

//----------------------------------------------------------------------------
int vtkKWMessage::GetJustification()
{
  return vtkKWTkOptions::GetJustificationFromTkOptionValue(
    this->GetConfigurationOption("-justify"));
}

//----------------------------------------------------------------------------
void vtkKWMessage::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWTkOptions::GetAnchorAsTkOptionValue(anchor));
}

void vtkKWMessage::SetAnchorToNorth() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorNorth); 
};
void vtkKWMessage::SetAnchorToNorthEast() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorNorthEast); 
};
void vtkKWMessage::SetAnchorToEast() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorEast); 
};
void vtkKWMessage::SetAnchorToSouthEast() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorSouthEast); 
};
void vtkKWMessage::SetAnchorToSouth() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorSouth); 
};
void vtkKWMessage::SetAnchorToSouthWest() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorSouthWest); 
};
void vtkKWMessage::SetAnchorToWest() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorWest); 
};
void vtkKWMessage::SetAnchorToNorthWest() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorNorthWest); 
};
void vtkKWMessage::SetAnchorToCenter() 
{ 
  this->SetAnchor(vtkKWTkOptions::AnchorCenter); 
};

//----------------------------------------------------------------------------
int vtkKWMessage::GetAnchor()
{
  return vtkKWTkOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWMessage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
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

