/*=========================================================================

  Module:    vtkKWFrameWithScrollbar.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkObjectFactory.h"

#include "Utilities/BWidgets/vtkKWBWidgetsInit.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrameWithScrollbar );
vtkCxxRevisionMacro(vtkKWFrameWithScrollbar, "1.9");

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
void vtkKWFrameWithScrollbar::Create(vtkKWApplication *app)
{
  // Use BWidget's ScrolledWindow class:
  // http://aspn.activestate.com/ASPN/docs/ActiveTcl/bwidget/contents.html

  vtkKWBWidgetsInit::Initialize(app ? app->GetMainInterp() : NULL);

  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(app, "ScrolledWindow"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // The widget itself is a BWidget's ScrolledWindow

  this->SetReliefToFlat();
  this->SetBorderWidth(2);
  this->SetConfigurationOption("-auto", "both");

  // ScrollableFrame is a BWidget's ScrollableFrame
  // attached to the ScrolledWindow

  this->ScrollableFrame = vtkKWCoreWidget::New();
  this->ScrollableFrame->SetParent(this);
  this->ScrollableFrame->CreateSpecificTkWidget(app, "ScrollableFrame");
  this->ScrollableFrame->SetConfigurationOptionAsInt("-height", 1024);
  this->ScrollableFrame->SetConfigurationOptionAsInt("-constrainedwidth", 1);

  this->Script("%s setwidget %s", 
               this->GetWidgetName(), this->ScrollableFrame->GetWidgetName());

  // The internal frame is a frame we set the widget name explicitly

  this->Frame = vtkKWCoreWidget::New();
  this->Frame->SetParent(this->ScrollableFrame);
  this->Frame->SetWidgetName(
    this->Script("%s getframe", this->ScrollableFrame->GetWidgetName()));
  this->Frame->Create(app);

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
