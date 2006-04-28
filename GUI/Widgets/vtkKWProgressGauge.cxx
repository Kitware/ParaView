/*=========================================================================

  Module:    vtkKWProgressGauge.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWProgressGauge.h"

#include "vtkKWCanvas.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWApplication.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWProgressGauge );
vtkCxxRevisionMacro(vtkKWProgressGauge, "1.38");

//----------------------------------------------------------------------------
vtkKWProgressGauge::vtkKWProgressGauge()
{ 
  this->Width = 100;
  this->Height = 14;
  this->MinimumHeight = this->Height;
  this->Value = 0.0;
  this->BarColor[0] = 0.0;
  this->BarColor[1] = 0.0;
  this->BarColor[2] = 1.0;
  this->Canvas = NULL;
  this->ExpandHeight = 0;
}

//----------------------------------------------------------------------------
vtkKWProgressGauge::~vtkKWProgressGauge()
{ 
  if (this->Canvas)
    {
    this->Canvas->Delete();
    this->Canvas = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  this->Canvas = vtkKWCanvas::New();
  this->Canvas->SetParent(this);
  this->Canvas->Create();
  this->Canvas->SetBorderWidth(0);
  this->Canvas->SetHighlightThickness(0);
  this->Canvas->SetWidth(0);
  this->Canvas->SetHeight(0);

  // Create the progress bar and text

  this->Script("%s create rectangle 0 0 0 0 -outline \"\" -tags bar", 
               this->Canvas->GetWidgetName());

  this->Script("%s create text 0 0 -anchor c -text \"\" -tags value",
               this->Canvas->GetWidgetName());

  this->Canvas->SetBinding("<Configure>", this, "ConfigureCallback");

  this->Script("pack %s -fill both -expand yes", 
               this->Canvas->GetWidgetName());

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetValue(double value)
{
  if(value < 0.0)
    {
    value = 0.0;
    }
  if(value > 100.0)
    {
    value = 100.0;
    }
  if (this->Value == value)
    {
    return;
    }

  this->Value = value;
  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetHeight(int height)
{
  if (this->Height == height)
    {
    return;
    }

  this->Height = height;
  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Width = width;
  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetMinimumHeight(int height)
{
  if (this->MinimumHeight == height)
    {
    return;
    }

  this->MinimumHeight = height;
  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetBarColor(double r, double g, double b)
{
  double *color = this->GetBarColor();
  if (!color || (color[0] == r && color[1] == g && color[2] == b))
    {
    return;
    }

  this->BarColor[0] = r;
  this->BarColor[1] = g;
  this->BarColor[2] = b;
  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetExpandHeight(int arg)
{
  if (this->ExpandHeight == arg)
    {
    return;
    }

  this->ExpandHeight = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::ConfigureCallback()
{
  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::Redraw()
{
  if (!this->Canvas || !this->Canvas->IsCreated())
    {
    return;
    }

  int enabled = this->GetEnabled();
  if (!enabled)
    {
    this->EnabledOn();
    }

  const char* wname = this->Canvas->GetWidgetName();

  ostrstream tk_cmd;

  // Resize the canvas

  this->Canvas->SetWidth(this->Width);

  int height = this->Height;
  if (this->ExpandHeight)
    {
    vtkKWTkUtilities::GetWidgetSize(this->Canvas, NULL, &height);
    if (height < this->MinimumHeight)
      {
      height = this->MinimumHeight;
      this->Canvas->SetHeight(height);
      }
    }
  else
    {
    this->Canvas->SetHeight(height);
    }

  // If the Value is 0, set the text to nothing and the color
  // of the bar to the background (0 0 0 0) rectangles show
  // up as a pixel...
  // Otherwise use the BarColor for the bar

  if (this->Value <= 0.0)
    {
    tk_cmd << wname << " itemconfigure value -text {}" << endl
           << wname << " coords bar 0 0 0 0" << endl
           << wname << " itemconfigure bar -fill {}" << endl;
    }
  else
    {
    tk_cmd << wname << " coords value " 
           << this->Width * 0.5 << " " << height * 0.5 << endl;

    char color[10];
    sprintf(color, "#%02x%02x%02x", 
            (int)(this->BarColor[0] * 255.0),
            (int)(this->BarColor[1] * 255.0),
            (int)(this->BarColor[2] * 255.0));

    tk_cmd << wname << " itemconfigure bar -fill " << color << endl;

    // Set the text to the percent done

    const char *textcolor = "-fill black";
    if(this->Value > 50.0)
      {
      textcolor = "-fill white";
      }
    
    char buffer[5];
    sprintf(buffer, "%3.0lf", this->Value);

    tk_cmd << wname << " itemconfigure value -text {" << buffer 
           << "%%} " << textcolor << endl;

    // Draw the correct rectangle

    tk_cmd << wname << " coords bar 0 0 [expr 0.01 * " << this->Value 
           << " * [winfo width " << wname << "]] [winfo height "
           << wname << "]" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  this->GetApplication()->ProcessIdleTasks();

  if (!enabled)
    {
    this->EnabledOff();
    }
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "BarColor: (" << this->BarColor[0] << ", " 
    << this->BarColor[1] << ", " << this->BarColor[2] << ")\n";
  os << indent << "Height: " << this->GetHeight() << endl;
  os << indent << "MinimumHeight: " << this->GetMinimumHeight() << endl;
  os << indent << "Width: " << this->GetWidth() << endl;
  os << indent << "ExpandHeight: "
     << (this->ExpandHeight ? "On" : "Off") << endl;
}
