/*=========================================================================

  Module:    vtkKWWidgetWithScrollbars.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWWidgetWithScrollbars.h"
#include "vtkObjectFactory.h"
#include "vtkKWScrollbar.h"
#include "vtkKWCoreWidget.h"

#include <vtksys/stl/string>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWWidgetWithScrollbars, "1.6");

//----------------------------------------------------------------------------
vtkKWWidgetWithScrollbars::vtkKWWidgetWithScrollbars()
{
  this->VerticalScrollBar   = NULL;
  this->HorizontalScrollBar = NULL;
  this->VerticalScrollbarVisibility = 1;
  this->HorizontalScrollbarVisibility = 1;
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
void vtkKWWidgetWithScrollbars::Create()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create();

#if 0
  this->SetPadX(2); // or 1
  this->SetPadY(this->GetPadX());
  this->SetReliefToGroove(); // or Sunken with padx 1
  this->SetBorderWidth(2);
#endif
  
  // Create the scrollbars

  if (this->VerticalScrollbarVisibility)
    {
    this->CreateVerticalScrollbar();
    }

  if (this->HorizontalScrollbarVisibility)
    {
    this->CreateHorizontalScrollbar();
    }

  // Pack
  
  this->Pack();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateVerticalScrollbar()
{
  if (!this->VerticalScrollBar)
    {
    this->VerticalScrollBar = vtkKWScrollbar::New();
    }

  if (!this->VerticalScrollBar->IsCreated())
    {
    this->VerticalScrollBar->SetParent(this);
    this->VerticalScrollBar->Create();
    this->VerticalScrollBar->SetOrientationToVertical();
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::AssociateVerticalScrollbarToWidget(
  vtkKWCoreWidget *widget)
{
  if (this->VerticalScrollBar && this->VerticalScrollBar->IsCreated() &&
      widget && widget->IsCreated())
    {
    vtksys_stl::string command(widget->GetWidgetName());
    command += " yview";
    this->VerticalScrollBar->SetCommand(NULL, command.c_str());
    command = this->VerticalScrollBar->GetWidgetName();
    command += " set";
    widget->SetConfigurationOption(
      "-yscrollcommand", command.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateHorizontalScrollbar()
{
  if (!this->HorizontalScrollBar)
    {
    this->HorizontalScrollBar = vtkKWScrollbar::New();
    }

  if (!this->HorizontalScrollBar->IsCreated())
    {
    this->HorizontalScrollBar->SetParent(this);
    this->HorizontalScrollBar->Create();
    this->HorizontalScrollBar->SetOrientationToHorizontal();
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::AssociateHorizontalScrollbarToWidget(
  vtkKWCoreWidget *widget)
{
  if (this->HorizontalScrollBar && this->HorizontalScrollBar->IsCreated() &&
      widget && widget->IsCreated())
    {
    vtksys_stl::string command(widget->GetWidgetName());
    command += " xview";
    this->HorizontalScrollBar->SetCommand(NULL, command.c_str());
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

  if (this->VerticalScrollbarVisibility && 
      this->VerticalScrollBar && this->VerticalScrollBar->IsCreated())
    {
    tk_cmd << "grid " << this->VerticalScrollBar->GetWidgetName() 
           << " -row 0 -column 1 -sticky ns" << endl;
    }

  if (this->HorizontalScrollbarVisibility && 
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
void vtkKWWidgetWithScrollbars::SetVerticalScrollbarVisibility(int arg)
{
  if (this->VerticalScrollbarVisibility == arg)
    {
    return;
    }

  this->VerticalScrollbarVisibility = arg;
  if (this->VerticalScrollbarVisibility)
    {
    this->CreateVerticalScrollbar();
    }
  this->Pack();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::SetHorizontalScrollbarVisibility(int arg)
{
  if (this->HorizontalScrollbarVisibility == arg)
    {
    return;
    }

  this->HorizontalScrollbarVisibility = arg;
  if (this->HorizontalScrollbarVisibility)
    {
    this->CreateHorizontalScrollbar();
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

  os << indent << "VerticalScrollbarVisibility: " 
     << (this->VerticalScrollbarVisibility ? "On" : "Off") << endl;
  os << indent << "HorizontalScrollbarVisibility: " 
     << (this->HorizontalScrollbarVisibility ? "On" : "Off") << endl;
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
