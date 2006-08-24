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

#include "vtkKWOptions.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWScrollbar);
vtkCxxRevisionMacro(vtkKWScrollbar, "1.10");

//----------------------------------------------------------------------------
void vtkKWScrollbar::CreateWidget()
{
  // Call the superclass to set the appropriate flags then create manually

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "scrollbar", "-highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetOrientation(int orientation)
{
  this->SetConfigurationOption(
    "-orient", vtkKWOptions::GetOrientationAsTkOptionValue(orientation));
}

void vtkKWScrollbar::SetOrientationToHorizontal() 
{ 
  this->SetOrientation(vtkKWOptions::OrientationHorizontal); 
};
void vtkKWScrollbar::SetOrientationToVertical() 
{ 
  this->SetOrientation(vtkKWOptions::OrientationVertical); 
};

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetOrientation()
{
  return vtkKWOptions::GetOrientationFromTkOptionValue(
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
#ifdef _WIN32
  return NULL;
#else
  return this->GetConfigurationOptionAsColor("-foreground");
#endif
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetForegroundColor(double r, double g, double b)
{
#ifdef _WIN32
  (void)r;
  (void)g;
  (void)b;
#else
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
#endif
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
void vtkKWScrollbar::SetWidth(int width)
{
  this->SetConfigurationOptionAsInt("-width", width);
}

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetWidth()
{
  return this->GetConfigurationOptionAsInt("-width");
}

//----------------------------------------------------------------------------
void vtkKWScrollbar::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWScrollbar::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWScrollbar::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWScrollbar::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWScrollbar::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWScrollbar::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWScrollbar::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWScrollbar::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
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

