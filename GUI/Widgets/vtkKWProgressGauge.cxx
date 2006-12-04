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
#include <vtksys/stl/map>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWProgressGauge );
vtkCxxRevisionMacro(vtkKWProgressGauge, "1.41");

//----------------------------------------------------------------------------
class vtkKWProgressGaugeInternals
{
public:

  typedef vtksys_stl::map<int, double> ValuePoolType;
  typedef vtksys_stl::map<int, double>::iterator ValuePoolIterator;
  ValuePoolType ValuePool;
};

//----------------------------------------------------------------------------
vtkKWProgressGauge::vtkKWProgressGauge()
{ 
  this->Width         = 100;
  this->Height        = 15;
  this->MinimumHeight = this->Height;
  this->Canvas        = NULL;
  this->ExpandHeight  = 0;
  this->PrimaryGaugePosition = vtkKWProgressGauge::GaugePositionTop;

  this->BarColor[0] = 0.0;
  this->BarColor[1] = 0.0;
  this->BarColor[2] = 1.0;

  this->Internals     = new vtkKWProgressGaugeInternals;
}

//----------------------------------------------------------------------------
vtkKWProgressGauge::~vtkKWProgressGauge()
{ 
  if (this->Canvas)
    {
    this->Canvas->Delete();
    this->Canvas = NULL;
    }

  delete this->Internals;
  this->Internals = NULL;
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

  // Create the text

  this->Script("%s create text 0 0 -anchor c -text \"\" -tags value",
               this->Canvas->GetWidgetName());

  this->Canvas->SetBinding("<Configure>", this, "ConfigureCallback");

  this->Script("pack %s -fill both -expand yes", 
               this->Canvas->GetWidgetName());

  this->Redraw();
}

//----------------------------------------------------------------------------
double vtkKWProgressGauge::GetNthValue(int rank)
{
  if (rank >= 0)
    {
    vtkKWProgressGaugeInternals::ValuePoolIterator it = 
      this->Internals->ValuePool.find(rank);
    if (it != this->Internals->ValuePool.end())
      {
      return it->second;
      }
    }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetNthValue(int rank, double value)
{
  if (rank < 0)
    {
    return;
    }

  if (value < 0.0)
    {
    value = 0.0;
    }
  if (value > 100.0)
    {
    value = 100.0;
    }
  if (this->GetNthValue(rank) == value)
    {
    return;
    }

  this->Internals->ValuePool[rank] = value;
  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
double vtkKWProgressGauge::GetValue()
{
  return this->GetNthValue(0);
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetValue(double value)
{
  this->SetNthValue(0, value);
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
  if (!color || 
      (color[0] == r && color[1] == g && color[2] == b) ||
      r < 0.0 || r > 1.0 || 
      g < 0.0 || g > 1.0 || 
      b < 0.0 || b > 1.0)
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

  vtkKWProgressGaugeInternals::ValuePoolIterator it;
  vtkKWProgressGaugeInternals::ValuePoolIterator end
    = this->Internals->ValuePool.end();

  int rank;
  double value;

  // Compute the number of gauges to display
  // Create a rectangle for each gauge if it does not exist

  int nb_gauges_visible = 1;

  it = this->Internals->ValuePool.begin();
  for (; it != end; ++it)
    {
    rank = it->first;
    value = it->second;
    if (value > 0.0)
      {
      nb_gauges_visible = rank + 1;
      if (!atoi(this->Canvas->Script("llength [%s find withtag bar%d]",
                                     wname, rank)))
        {
        tk_cmd << wname << " create rectangle 0 0 0 0 -outline \"\" -tags bar"
               << rank << endl
               << wname << " lower bar" << rank << " all" << endl;
        }
      }
    }

  // If the Value is 0, set the text of the primary gauge to an empty string

  value = this->GetValue();
  if (value <= 0.0 || nb_gauges_visible > 1)
    {
    tk_cmd << wname << " itemconfigure value -text {}" << endl;
    }
  else
    {
    tk_cmd << wname << " coords value " 
           << this->Width * 0.5 << " " << height * 0.5 << endl;

    // Set the text to the percent done for the primary gauge (rank 0)

    const char *textcolor = "-fill black";
    if(value > 50.0)
      {
      textcolor = "-fill white";
      }
    
    char buffer[5];
    sprintf(buffer, "%3.0lf", value);

    tk_cmd << wname << " itemconfigure value -text {" << buffer 
           << "%%} " << textcolor << endl;
    }

  // If the value is 0, set the color of the bar to the background, otherwise 
  // the rectangle will still show up as a pixel...

  char color[10];
  sprintf(color, "#%02x%02x%02x", 
          (int)(this->BarColor[0] * 255.0),
          (int)(this->BarColor[1] * 255.0),
          (int)(this->BarColor[2] * 255.0));
  
  int bar_height = 
    (int)floor((height - nb_gauges_visible + 1) / (double)nb_gauges_visible);

  it = this->Internals->ValuePool.begin();
  for (; it != end; ++it)
    {
    rank = it->first;
    value = it->second;
    if (value <= 0.0)
      {
      if (atoi(this->Canvas->Script("llength [%s find withtag bar%d]",
                                    wname, rank)))
        {
        tk_cmd << wname << " coords bar" << rank << " 0 0 0 0" << endl
               << wname << " itemconfigure bar" << rank << " -fill {}" << endl;
        }
      }
    else
      {
      double y;
      if (this->PrimaryGaugePosition == vtkKWProgressGauge::GaugePositionTop)
        {
        y = rank * (bar_height + 1);
        }
      else
        {
        y = height - (rank + 1) * (bar_height + 1) + 1;
        }
      double w = 0.01 * value * this->Width;
      tk_cmd 
        << wname << " itemconfigure bar" << rank << " -fill " << color << endl
        << wname << " coords bar" << rank << " 0 " << y << " " << w << " " 
        << y + bar_height << endl;
      }
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
void vtkKWProgressGauge::SetPrimaryGaugePosition(int arg)
{
  if (arg < vtkKWProgressGauge::GaugePositionTop)
    {
    arg = vtkKWProgressGauge::GaugePositionTop;
    }
  else if (arg > vtkKWProgressGauge::GaugePositionBottom)
    {
    arg = vtkKWProgressGauge::GaugePositionBottom;
    }

  if (this->PrimaryGaugePosition == arg)
    {
    return;
    }

  this->PrimaryGaugePosition = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetPrimaryGaugePositionToTop()
{ 
  this->SetPrimaryGaugePosition(
    vtkKWProgressGauge::GaugePositionTop); 
}

//----------------------------------------------------------------------------
void vtkKWProgressGauge::SetPrimaryGaugePositionToBottom()
{ 
  this->SetPrimaryGaugePosition(
    vtkKWProgressGauge::GaugePositionBottom); 
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
  os << indent << "PrimaryGaugePosition: " << this->PrimaryGaugePosition << endl;
}
