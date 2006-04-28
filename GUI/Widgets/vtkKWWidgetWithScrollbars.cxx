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
vtkCxxRevisionMacro(vtkKWWidgetWithScrollbars, "1.8");

//----------------------------------------------------------------------------
vtkKWWidgetWithScrollbars::vtkKWWidgetWithScrollbars()
{
  this->VerticalScrollbar   = NULL;
  this->HorizontalScrollbar = NULL;
  this->VerticalScrollbarVisibility = 1;
  this->HorizontalScrollbarVisibility = 1;
}

//----------------------------------------------------------------------------
vtkKWWidgetWithScrollbars::~vtkKWWidgetWithScrollbars()
{
  if (this->VerticalScrollbar)
    {
    this->VerticalScrollbar->Delete();
    this->VerticalScrollbar = NULL;
    }

  if (this->HorizontalScrollbar)
    {
    this->HorizontalScrollbar->Delete();
    this->HorizontalScrollbar = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

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
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateVerticalScrollbar()
{
  if (!this->VerticalScrollbar)
    {
    this->VerticalScrollbar = vtkKWScrollbar::New();
    }

  if (!this->VerticalScrollbar->IsCreated())
    {
    this->VerticalScrollbar->SetParent(this);
    this->VerticalScrollbar->Create();
    this->VerticalScrollbar->SetOrientationToVertical();
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::AssociateVerticalScrollbarToWidget(
  vtkKWCoreWidget *widget)
{
  if (this->VerticalScrollbar && this->VerticalScrollbar->IsCreated() &&
      widget && widget->IsCreated())
    {
    vtksys_stl::string command(widget->GetWidgetName());
    command += " yview";
    this->VerticalScrollbar->SetCommand(NULL, command.c_str());
    command = this->VerticalScrollbar->GetWidgetName();
    command += " set";
    widget->SetConfigurationOption(
      "-yscrollcommand", command.c_str());
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::CreateHorizontalScrollbar()
{
  if (!this->HorizontalScrollbar)
    {
    this->HorizontalScrollbar = vtkKWScrollbar::New();
    }

  if (!this->HorizontalScrollbar->IsCreated())
    {
    this->HorizontalScrollbar->SetParent(this);
    this->HorizontalScrollbar->Create();
    this->HorizontalScrollbar->SetOrientationToHorizontal();
    }
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::AssociateHorizontalScrollbarToWidget(
  vtkKWCoreWidget *widget)
{
  if (this->HorizontalScrollbar && this->HorizontalScrollbar->IsCreated() &&
      widget && widget->IsCreated())
    {
    vtksys_stl::string command(widget->GetWidgetName());
    command += " xview";
    this->HorizontalScrollbar->SetCommand(NULL, command.c_str());
    command = this->HorizontalScrollbar->GetWidgetName();
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
      this->VerticalScrollbar && this->VerticalScrollbar->IsCreated())
    {
    tk_cmd << "grid " << this->VerticalScrollbar->GetWidgetName() 
           << " -row 0 -column 1 -sticky ns" << endl;
    }

  if (this->HorizontalScrollbarVisibility && 
      this->HorizontalScrollbar && this->HorizontalScrollbar->IsCreated())
    {
    tk_cmd << "grid " << this->HorizontalScrollbar->GetWidgetName() 
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

  this->PropagateEnableState(this->VerticalScrollbar);
  this->PropagateEnableState(this->HorizontalScrollbar);
}

//----------------------------------------------------------------------------
void vtkKWWidgetWithScrollbars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "VerticalScrollbarVisibility: " 
     << (this->VerticalScrollbarVisibility ? "On" : "Off") << endl;
  os << indent << "HorizontalScrollbarVisibility: " 
     << (this->HorizontalScrollbarVisibility ? "On" : "Off") << endl;
  os << indent << "VerticalScrollbar: ";
  if (this->VerticalScrollbar)
    {
    os << this->VerticalScrollbar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
  os << indent << "HorizontalScrollbar: ";
  if (this->HorizontalScrollbar)
    {
    os << this->HorizontalScrollbar << endl;
    }
  else
    {
    os << "(None)" << endl;
    }
}
