/*=========================================================================

  Module:    vtkKWScrollbar.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWScrollbar.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWScrollbar);
vtkCxxRevisionMacro(vtkKWScrollbar, "1.4");

//----------------------------------------------------------------------------
void vtkKWScrollbar::Create()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget("scrollbar"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetOrientation(int orientation)
{
  this->SetConfigurationOption(
    "-orient", vtkKWTkOptions::GetOrientationAsTkOptionValue(orientation));
}

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetOrientation()
{
  return vtkKWTkOptions::GetOrientationFromTkOptionValue(
    this->GetConfigurationOption("-orient"));
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetCommand(vtkObject *object, const char *method)
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
void vtkKWScrollbar::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScrollbar::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScrollbar::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetHighlightThickness(int width)
{
  this->SetConfigurationOptionAsInt("-highlightthickness", width);
}

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetHighlightThickness()
{
  return this->GetConfigurationOptionAsInt("-highlightthickness");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScrollbar::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWTkOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWScrollbar::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRaised); 
};
void vtkKWScrollbar::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSunken); 
};
void vtkKWScrollbar::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefFlat); 
};
void vtkKWScrollbar::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefRidge); 
};
void vtkKWScrollbar::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefSolid); 
};
void vtkKWScrollbar::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWTkOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetRelief()
{
  return vtkKWTkOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::GetTroughColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-troughcolor", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWScrollbar::GetTroughColor()
{
  return this->GetConfigurationOptionAsColor("-troughcolor");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetTroughColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-troughcolor", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

