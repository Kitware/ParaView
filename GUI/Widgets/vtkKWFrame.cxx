/*=========================================================================

  Module:    vtkKWFrame.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWFrame );
vtkCxxRevisionMacro(vtkKWFrame, "1.25");

//----------------------------------------------------------------------------
vtkKWFrame::vtkKWFrame()
{
  this->ScrollFrame = 0;
  this->Frame = 0;
  this->Scrollable = 0;
}

//----------------------------------------------------------------------------
vtkKWFrame::~vtkKWFrame()
{
  if ( this->ScrollFrame )
    {
    this->ScrollFrame->Delete();
    this->ScrollFrame = 0;
    }
  if (this->Frame && this->Frame != this)
    {
    this->Frame->Delete();
    this->Frame = 0;
    }
}

//----------------------------------------------------------------------------
void vtkKWFrame::Create(vtkKWApplication *app, const char* args)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::Create(app, NULL, NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  const char *wname = this->GetWidgetName();
  
  if (this->Scrollable)
    {
    this->Script("ScrolledWindow %s -relief flat -bd 2", wname);

    this->ScrollFrame = vtkKWWidget::New();
    this->ScrollFrame->SetParent(this);

    ostrstream options;
    options << "-height 1024 -constrainedwidth 1 ";
    if (args)
      {
      options << args;
      }
    options << ends;
    this->ScrollFrame->Create(app, "ScrollableFrame",  options.str());
    options.rdbuf()->freeze(0);

    this->Script("%s setwidget %s", 
                 this->GetWidgetName(),
                 this->ScrollFrame->GetWidgetName());

    this->Frame = vtkKWWidget::New();
    this->Frame->SetParent(this->ScrollFrame);
    this->Frame->SetWidgetName(
      this->Script("%s getframe", this->ScrollFrame->GetWidgetName()));
    this->Frame->Create(app, NULL, NULL);
    }
  else
    {
    if (args)
      {
      this->Script("frame %s %s", wname, args);
      }
    else // original code with hard defaults
      {
      this->Script("frame %s -bd 0 -relief flat", wname);
      }
    this->Frame = this;
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWFrame::SetWidth(int width)
{
  if (this->IsCreated() && this->HasConfigurationOption("-width"))
    {
    this->Script("%s config -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWFrame::SetHeight(int height)
{
  if (this->IsCreated() && this->HasConfigurationOption("-height"))
    {
    this->Script("%s config -height %d", this->GetWidgetName(), height);
    }
}

//----------------------------------------------------------------------------
void vtkKWFrame::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->ScrollFrame);
}

//----------------------------------------------------------------------------
void vtkKWFrame::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Scrollable " << this->Scrollable << "\n";

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

  os << indent << "ScrollFrame: ";
  if (this->ScrollFrame)
    {
    os << endl;
    this->ScrollFrame->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}

