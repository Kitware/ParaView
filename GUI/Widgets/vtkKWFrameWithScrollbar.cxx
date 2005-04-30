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

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrameWithScrollbar );
vtkCxxRevisionMacro(vtkKWFrameWithScrollbar, "1.1");

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
void vtkKWFrameWithScrollbar::Create(vtkKWApplication *app, const char* args)
{
  // Call the superclass to set the appropriate flags then create manually

#if 0
  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // The widget itself is a BWidget's ScrolledWindow

  const char *wname = this->GetWidgetName();
  this->Script("ScrolledWindow %s -relief flat -bd 2", wname);

  // ScrollableFrame is a BWidget's ScrollableFrame
  // attached to the ScrolledWindow

  this->ScrollableFrame = vtkKWWidget::New();
  this->ScrollableFrame->SetParent(this);
  ostrstream options;
  options << "-height 1024 -constrainedwidth 1 ";
  if (args)
    {
    options << args;
    }
  options << ends;
  this->ScrollableFrame->Create(app, "ScrollableFrame",  options.str());
  options.rdbuf()->freeze(0);

  this->Script("%s setwidget %s", 
               this->GetWidgetName(), this->ScrollableFrame->GetWidgetName());

  // The internal frame is a frame we set the widget name explicitly

  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this->ScrollableFrame);
  this->Frame->SetWidgetName(
    this->Script("%s getframe", this->ScrollableFrame->GetWidgetName()));
  this->Frame->Create(app, NULL, NULL);
#else

  if (!this->Superclass::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Frame = vtkKWWidget::New();
  this->Frame->SetParent(this);
  this->Frame->Create(app, "frame", NULL);
  this->Script("pack %s -expand yes -fill both", this->Frame->GetWidgetName());
#endif

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetWidth(int width)
{
  if (this->IsCreated() && this->HasConfigurationOption("-width"))
    {
    this->Script("%s config -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWFrameWithScrollbar::SetHeight(int height)
{
  if (this->IsCreated() && this->HasConfigurationOption("-height"))
    {
    this->Script("%s config -height %d", this->GetWidgetName(), height);
    }
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
