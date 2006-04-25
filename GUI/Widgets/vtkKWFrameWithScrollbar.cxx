/*=========================================================================

  Module:    vtkKWFrameWithScrollbar.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWFrameWithScrollbar.h"

#include "vtkKWApplication.h"
#include "vtkKWOptions.h"
#include "vtkObjectFactory.h"

#include "Utilities/BWidgets/vtkKWBWidgetsInit.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrameWithScrollbar );
vtkCxxRevisionMacro(vtkKWFrameWithScrollbar, "1.12");

//----------------------------------------------------------------------------
vtkKWFrameWithScrollbar::vtkKWFrameWithScrollbar()
{
  this->Frame   = NULL;
  this->ScrollableFrame = NULL;
}

//----------------------------------------------------------------------------
vtkKWFrameWithScrollbar::~vtkKWFrameWithScrollbar()
{
  if (this->ScrollableFrame)
    {
    this->ScrollableFrame->Delete();
    this->ScrollableFrame = NULL;
    }
  if (this->Frame)
    {
    this->Frame->Delete();
    this->Frame = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::Create()
{
  // Use BWidget's ScrolledWindow class:
  // http://aspn.activestate.com/ASPN/docs/ActiveTcl/bwidget/contents.html

  vtkKWApplication *app = this->GetApplication();
  vtkKWBWidgetsInit::Initialize(app ? app->GetMainInterp() : NULL);

  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(
        "ScrolledWindow", "-relief flat -bd 2 -auto both"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // ScrollableFrame is a BWidget's ScrollableFrame
  // attached to the ScrolledWindow

  this->ScrollableFrame = vtkKWCoreWidget::New();
  this->ScrollableFrame->SetParent(this);
  this->ScrollableFrame->CreateSpecificTkWidget(
    "ScrollableFrame", "-height 1024 -constrainedwidth 1");

  this->Script("%s setwidget %s", 
               this->GetWidgetName(), this->ScrollableFrame->GetWidgetName());

  // The internal frame is a frame we set the widget name explicitly

  this->Frame = vtkKWCoreWidget::New();
  this->Frame->SetParent(this->ScrollableFrame);
  this->Frame->SetWidgetName(
    this->Script("%s getframe", this->ScrollableFrame->GetWidgetName()));
  this->Frame->Create();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetWidth(int width)
{
  if (this->ScrollableFrame)
    {
    this->ScrollableFrame->SetConfigurationOptionAsInt("-width", width);
    }
}

//----------------------------------------------------------------------------
int vtkKWFrameWithScrollbar::GetWidth()
{
  if (this->ScrollableFrame)
    {
    return this->ScrollableFrame->GetConfigurationOptionAsInt("-width");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetHeight(int height)
{
  if (this->ScrollableFrame)
    {
    this->ScrollableFrame->SetConfigurationOptionAsInt("-height", height);
    }
}

//----------------------------------------------------------------------------
int vtkKWFrameWithScrollbar::GetHeight()
{
  if (this->ScrollableFrame)
    {
    return this->ScrollableFrame->GetConfigurationOptionAsInt("-height");
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWFrameWithScrollbar::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWFrameWithScrollbar::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWFrameWithScrollbar::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWFrameWithScrollbar::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWFrameWithScrollbar::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWFrameWithScrollbar::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWFrameWithScrollbar::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWFrameWithScrollbar::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWFrameWithScrollbar::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->ScrollableFrame);
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Frame: ";
  if (this->Frame)
    {
    os << endl;
    this->Frame->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ScrollableFrame: ";
  if (this->ScrollableFrame)
    {
    os << endl;
    this->ScrollableFrame->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
