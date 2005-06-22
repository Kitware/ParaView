/*=========================================================================

  Module:    vtkKWWidgetWithScrollbars.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWWidgetWithScrollbars.h"
#include "vtkObjectFactory.h"
#include "vtkKWScrollbar.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWWidgetWithScrollbars, "1.3");

//----------------------------------------------------------------------------
vtkKWWidgetWithScrollbars::vtkKWWidgetWithScrollbars()
{
  this->VerticalScrollBar   = NULL;
  this->HorizontalScrollBar = NULL;
  this->ShowVerticalScrollbar = 1;
  this->ShowHorizontalScrollbar = 1;
}

//----------------------------------------------------------------------------
vtkKWWidgetWithScrollbars::~vtkKWWidgetWithScrollbars()
{
  if (this->VerticalScrollBar)
    {
    this->VerticalScrollBar->Delete();
    this->VerticalScrollBar = NULL;
    }

  if (this->HorizontalScrollBar)
    {
    this->HorizontalScrollBar->Delete();
    this->HorizontalScrollBar = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

#if 0
  this->SetPadX(2); // or 1
  this->SetPadY(this->GetPadX());
  this->SetReliefToGroove(); // or Sunken with padx 1
  this->SetBorderWidth(2);
#endif
  
  // Create the scrollbars

  if (this->ShowVerticalScrollbar)
    {
    this->CreateVerticalScrollbar(app);
    }

  if (this->ShowHorizontalScrollbar)
    {
    this->CreateHorizontalScrollbar(app);
    }

  // Pack
  
  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateVerticalScrollbar(vtkKWApplication *app)
{
  if (!this->VerticalScrollBar)
    {
    this->VerticalScrollBar = vtkKWScrollbar::New();
    }

  if (!this->VerticalScrollBar->IsCreated())
    {
    this->VerticalScrollBar->SetParent(this);
    this->VerticalScrollBar->Create(app);
    this->VerticalScrollBar->SetOrientationToVertical();
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::AssociateVerticalScrollbarToWidget(
  vtkKWWidget *widget)
{
  if (this->VerticalScrollBar && this->VerticalScrollBar->IsCreated() &&
      widget && widget->IsCreated())
    {
    vtksys_stl::string command(widget->GetWidgetName());
    command += " yview";
    this->VerticalScrollBar->SetConfigurationOption(
      "-command", command.c_str());
    command = this->VerticalScrollBar->GetWidgetName();
    command += " set";
    widget->SetConfigurationOption(
      "-yscrollcommand", command.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateHorizontalScrollbar(vtkKWApplication *app)
{
  if (!this->HorizontalScrollBar)
    {
    this->HorizontalScrollBar = vtkKWScrollbar::New();
    }

  if (!this->HorizontalScrollBar->IsCreated())
    {
    this->HorizontalScrollBar->SetParent(this);
    this->HorizontalScrollBar->Create(app);
    this->HorizontalScrollBar->SetOrientationToHorizontal();
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::AssociateHorizontalScrollbarToWidget(
  vtkKWWidget *widget)
{
  if (this->HorizontalScrollBar && this->HorizontalScrollBar->IsCreated() &&
      widget && widget->IsCreated())
    {
    vtksys_stl::string command(widget->GetWidgetName());
    command += " xview";
    this->HorizontalScrollBar->SetConfigurationOption(
      "-command", command.c_str());
    command = this->HorizontalScrollBar->GetWidgetName();
    command += " set";
    widget->SetConfigurationOption(
      "-xscrollcommand", command.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::PackScrollbarsWithWidget(vtkKWWidget *widget)
{
  if (!this->IsCreated())
    {
    return;
    }

  this->UnpackChildren();

  ostrstream tk_cmd;

  if (widget && widget->IsCreated())
    {
    tk_cmd << "grid " << widget->GetWidgetName() 
           << " -row 0 -column 0 -sticky news" << endl;
    }

  if (this->ShowVerticalScrollbar && 
      this->VerticalScrollBar && this->VerticalScrollBar->IsCreated())
    {
    tk_cmd << "grid " << this->VerticalScrollBar->GetWidgetName() 
           << " -row 0 -column 1 -sticky ns" << endl;
    }

  if (this->ShowHorizontalScrollbar && 
      this->HorizontalScrollBar && this->HorizontalScrollBar->IsCreated())
    {
    tk_cmd << "grid " << this->HorizontalScrollBar->GetWidgetName() 
           << " -row 1 -column 0 -sticky ew" << endl;
    }

  tk_cmd << "grid rowconfigure " << this->GetWidgetName() << " 0 -weight 1" 
         << endl;
  tk_cmd << "grid columnconfigure " << this->GetWidgetName() << " 0 -weight 1" 
         << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::SetShowVerticalScrollbar(int arg)
{
  if (this->ShowVerticalScrollbar == arg)
    {
    return;
    }

  this->ShowVerticalScrollbar = arg;
  if (this->ShowVerticalScrollbar)
    {
    this->CreateVerticalScrollbar(this->GetApplication());
    }
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::SetShowHorizontalScrollbar(int arg)
{
  if (this->ShowHorizontalScrollbar == arg)
    {
    return;
    }

  this->ShowHorizontalScrollbar = arg;
  if (this->ShowHorizontalScrollbar)
    {
    this->CreateHorizontalScrollbar(this->GetApplication());
    }
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->VerticalScrollBar);
  this->PropagateEnableState(this->HorizontalScrollBar);
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowVerticalScrollbar: " 
     << (this->ShowVerticalScrollbar ? "On" : "Off") << endl;
  os << indent << "ShowHorizontalScrollbar: " 
     << (this->ShowHorizontalScrollbar ? "On" : "Off") << endl;
  os << indent << "VerticalScrollBar: ";
  if (this->VerticalScrollBar)
    {
    os << this->VerticalScrollBar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  os << indent << "HorizontalScrollBar: ";
  if (this->HorizontalScrollBar)
    {
    os << this->HorizontalScrollBar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
}
