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
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"
#include "vtkKWScrollbar.h"

#include "Utilities/BWidgets/vtkKWBWidgetsInit.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrameWithScrollbar );
vtkCxxRevisionMacro(vtkKWFrameWithScrollbar, "1.16");

//----------------------------------------------------------------------------
class vtkKWFrameWithScrollbarInternals
{
public:

  vtkKWScrollbar *HorizontalScrollbar;
  vtkKWScrollbar *VerticalScrollbar;
};

//----------------------------------------------------------------------------
vtkKWFrameWithScrollbar::vtkKWFrameWithScrollbar()
{
  this->Internals = new vtkKWFrameWithScrollbarInternals;

  this->Frame   = NULL;
  this->ScrollableFrame = NULL;
}

//----------------------------------------------------------------------------
vtkKWFrameWithScrollbar::~vtkKWFrameWithScrollbar()
{
  if (this->Internals)
    {
    if (this->Internals->HorizontalScrollbar)
      {
      this->Internals->HorizontalScrollbar->Delete();
      this->Internals->HorizontalScrollbar = NULL;
      }
    if (this->Internals->VerticalScrollbar)
      {
      this->Internals->VerticalScrollbar->Delete();
      this->Internals->VerticalScrollbar = NULL;
      }
    delete this->Internals;
    }

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
void vtkKWFrameWithScrollbar::CreateWidget()
{
  // Use BWidget's ScrolledWindow class:
  // http://aspn.activestate.com/ASPN/docs/ActiveTcl/bwidget/contents.html

  vtkKWApplication *app = this->GetApplication();
  vtkKWBWidgetsInit::Initialize(app ? app->GetMainInterp() : NULL);

  // Call the superclass to set the appropriate flags then create manually

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "ScrolledWindow", "-relief flat -bd 2 -auto both"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // ScrollableFrame is a BWidget's ScrollableFrame
  // attached to the ScrolledWindow

  this->ScrollableFrame = vtkKWCoreWidget::New();
  this->ScrollableFrame->SetParent(this);
  vtkKWWidget::CreateSpecificTkWidget(
    this->ScrollableFrame,
    "ScrollableFrame", "-height 1024 -constrainedwidth 1");

  this->Script("%s setwidget %s", 
               this->GetWidgetName(), this->ScrollableFrame->GetWidgetName());

  // The internal frame is a frame we set the widget name explicitly

  this->Frame = vtkKWFrame::New();
  this->Frame->SetParent(this->ScrollableFrame);
  this->Frame->SetWidgetName(
    this->Script("%s getframe", this->ScrollableFrame->GetWidgetName()));
  this->Frame->Create();

  // Create scrollbar wrappers so that we are sure it goes through the
  // theming/option database framework

  vtksys_stl::string name;
  name = this->GetWidgetName();
  name += ".hscroll";
  this->Internals->HorizontalScrollbar = vtkKWScrollbar::New();
  this->Internals->HorizontalScrollbar->SetApplication(this->GetApplication());
  this->Internals->HorizontalScrollbar->SetWidgetName(name.c_str());
  this->Internals->HorizontalScrollbar->Create();

  name = this->GetWidgetName();
  name += ".vscroll";
  this->Internals->VerticalScrollbar = vtkKWScrollbar::New();
  this->Internals->VerticalScrollbar->SetApplication(this->GetApplication());
  this->Internals->VerticalScrollbar->SetWidgetName(name.c_str());
  this->Internals->VerticalScrollbar->Create();
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
  if (this->ScrollableFrame)
    {
    this->ScrollableFrame->SetConfigurationOptionAsColor(
      "-background", r, g, b);
    }
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
