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
vtkCxxRevisionMacro(vtkKWFrame, "1.19.2.1");

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
  const char *wname;
  
  // Set the application
  if (this->IsCreated())
    {
    vtkErrorMacro("ScrollableFrame already created");
    return;
    }
  this->SetApplication(app);
  
  if ( this->Scrollable )
    {
    // create the top level
    wname = this->GetWidgetName();
    this->Script("ScrolledWindow %s -relief flat -borderwidth 2", wname);

    this->ScrollFrame = vtkKWWidget::New();

    this->ScrollFrame->SetParent(this);
    this->ScrollFrame->Create(this->Application, 
                              "ScrollableFrame",
                              "-height 1024 -constrainedwidth 1");
    this->Script("%s setwidget %s", this->GetWidgetName(),
                 this->ScrollFrame->GetWidgetName());

    this->Frame = vtkKWWidget::New();
    this->Frame->SetParent(this->ScrollFrame);
    this->Script("%s getframe", this->ScrollFrame->GetWidgetName());
    this->Frame->SetWidgetName(this->Application->GetMainInterp()->result);
    this->Frame->SetApplication(this->Application);
    }
  else
    {
    // create the top level
    wname = this->GetWidgetName();
    if (args)
      {
      this->Script("frame %s %s", wname, args);
      }
    else // original code with hard defaults
      {
      this->Script("frame %s -borderwidth 0 -relief flat", wname);
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
}

