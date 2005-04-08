/*=========================================================================

  Module:    vtkKWRange.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWRange.h"

#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWRange );
vtkCxxRevisionMacro(vtkKWRange, "1.41");

#define VTK_KW_RANGE_MIN_SLIDER_SIZE        2
#define VTK_KW_RANGE_MIN_THICKNESS          (2*VTK_KW_RANGE_MIN_SLIDER_SIZE+1)
#define VTK_KW_RANGE_MIN_INTERNAL_THICKNESS 5
#define VTK_KW_RANGE_MIN_LENGTH             (2*VTK_KW_RANGE_MIN_THICKNESS)

#define VTK_KW_RANGE_NB_ENTRIES             2

#define VTK_KW_RANGE_WHOLE_RANGE_TAG        "whole_range"
#define VTK_KW_RANGE_RANGE_TAG              "range"
#define VTK_KW_RANGE_SLIDER1_TAG            "slider1"
#define VTK_KW_RANGE_SLIDER2_TAG            "slider2"
#define VTK_KW_RANGE_SLIDERS_TAG            "sliders"

// For some reasons, the end-point of a line/box is not drawn on Windows. 
// Comply with that.

#ifndef _WIN32
#define LSTRANGE 0
#else
#define LSTRANGE 1
#endif
#define RSTRANGE 1

//----------------------------------------------------------------------------
vtkKWRange::vtkKWRange()
{
  int i;

  this->WholeRange[0]         = 0;
  this->WholeRange[1]         = 1;  
  this->Range[0]              = this->WholeRange[0];
  this->Range[1]              = this->WholeRange[1];  
  this->WholeRangeAdjusted[0] = this->WholeRange[0];
  this->WholeRangeAdjusted[1] = this->WholeRange[1];
  this->RangeAdjusted[0]      = this->Range[0];
  this->RangeAdjusted[1]      = this->Range[1];
  this->Resolution            = (this->WholeRange[1]-this->WholeRange[0])*0.01;
  this->AdjustResolution      = 0;
  this->Thickness             = 19;
  this->InternalThickness     = 0.5;
  this->Orientation           = vtkKWRange::ORIENTATION_HORIZONTAL;
  this->Inverted              = 0;
  this->SliderSize            = 3;
  this->ShowEntries           = 0;
  this->Entry1Position        = vtkKWRange::EntryPositionDefault;
  this->Entry2Position        = vtkKWRange::EntryPositionDefault;
  this->EntriesWidth          = 10;
  this->SliderCanPush         = 0;
  this->DisableCommands       = 0;

  this->InInteraction            = 0;
  this->StartInteractionPos      = 0;
  this->StartInteractionRange[0] = this->Range[0];
  this->StartInteractionRange[1] = this->Range[1];

  this->RangeColor[0]      = -1; // will used a shade of -bg at runtime
  this->RangeColor[1]      = -1;
  this->RangeColor[2]      = -1;

  this->RangeInteractionColor[0] = 0.59;
  this->RangeInteractionColor[1] = 0.63;
  this->RangeInteractionColor[2] = 0.82;

  this->Command             = NULL;
  this->StartCommand        = NULL;
  this->EndCommand          = NULL;
  this->EntriesCommand      = NULL;

  this->CanvasFrame         = NULL;
  this->Canvas              = NULL;

  for (i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    this->Entries[i]        = NULL;
    }

  this->ClampRange = 1;

  this->ConstrainRanges();

  this->ConstrainResolution();

}

//----------------------------------------------------------------------------
vtkKWRange::~vtkKWRange()
{
  if (this->Command)
    {
    delete [] this->Command;
    this->Command = NULL;
    }

  if (this->StartCommand)
    {
    delete [] this->StartCommand;
    this->StartCommand = NULL;
    }

  if (this->EndCommand)
    {
    delete [] this->EndCommand;
    this->EndCommand = NULL;
    }

  if (this->EntriesCommand)
    {
    delete [] this->EntriesCommand;
    this->EntriesCommand = NULL;
    }

  if (this->CanvasFrame)
    {
    this->CanvasFrame->Delete();
    this->CanvasFrame = NULL;
    }

  if (this->Canvas)
    {
    this->Canvas->Delete();
    this->Canvas = NULL;
    }

  for (int i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    if (this->Entries[i])
      {
      this->Entries[i]->Delete();
      this->Entries[i] = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::Create(vtkKWApplication *app, const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("Range already created");
    return;
    }

  // Call the superclass, this will set the application,
  // create the frame and the Label

  this->Superclass::Create(app, args);

  // Now we need the canvas

  if (!this->CanvasFrame)
    {
    this->CanvasFrame = vtkKWFrame::New();
    }
  this->CanvasFrame->SetParent(this);
  this->CanvasFrame->Create(app, "");

  if (!this->Canvas)
    {
    this->Canvas = vtkKWCanvas::New();
    }
  this->Canvas->SetParent(this->CanvasFrame);
  this->Canvas->Create(app, "-bd 0 -highlightthickness 0 -width 0 -height 0");

  this->Script("bind %s <Configure> {%s ConfigureCallback}",
               this->CanvasFrame->GetWidgetName(), this->GetTclName());

  // Create more stuff

  if (this->ShowEntries)
    {
    this->CreateEntries();
    }

  // Pack the widget

  this->Pack();

  // Set the bindings

  this->Bind();

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWRange::CreateEntries()
{
  for (int i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    if (!this->Entries[i])
      {
      this->Entries[i] = vtkKWEntry::New();
      }

    if (!this->Entries[i]->IsCreated() && this->IsCreated())
      {
      this->Entries[i]->SetParent(this);
      this->Entries[i]->Create(this->GetApplication(), "");
      this->Entries[i]->SetWidth(this->EntriesWidth);
      this->Entries[i]->SetEnabled(this->Enabled);
      this->Script("bind %s <Return> {%s EntriesUpdateCallback %d}",
                   this->Entries[i]->GetWidgetName(), this->GetTclName(), i);
      this->Script("bind %s <FocusOut> {%s EntriesUpdateCallback %d}",
                   this->Entries[i]->GetWidgetName(), this->GetTclName(), i);
      }
    }

  this->UpdateEntriesValue(this->Range);
}

//----------------------------------------------------------------------------
void vtkKWRange::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  if (this->CanvasFrame)
    {
    this->CanvasFrame->UnpackSiblings();
    }

  // Repack everything

  ostrstream tk_cmd;
  int is_horiz = (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL);

  int row, col, row_span, col_span, c_padx = 0, c_pady = 0;
  const char *anchor, *sticky;

  /*
           0 1  2  3    4    5 6 7  8        0   1  2
         +---------------------------      +---------
        0|         E1   L   E2            0|     L 
        1| L E1 E2 [---------] L E1 E2    1|     E1
        2|         E1   L   E2            2|     E2
                                          3| E1  ^  E1
                                           |     |
                                          4| L   |  L
                                           |     |
                                          5| E2  v  E2
                                          6|     L
                                          7|     E1
                                          8|     E2
  */

  // Canvas

  if (this->Canvas && this->Canvas->IsCreated())
    {
    tk_cmd << "pack " << this->Canvas->GetWidgetName() 
           << " -fill both -expand y -pady 0 -padx 0 -ipady 0 -ipadx 0" 
           << endl;
    }

  // Label

  if (this->ShowLabel && this->HasLabel() && this->GetLabel()->IsCreated())
    {
    if (is_horiz)
      {
      switch (this->LabelPosition)
        {
        case vtkKWWidgetLabeled::LabelPositionLeft:
          col = 0; row = 1; sticky = "nsw"; anchor = "w";
          break;
        case vtkKWWidgetLabeled::LabelPositionRight:
          col = 6; row = 1; sticky = "nsw"; anchor = "w";
          break;
        case vtkKWWidgetLabeled::LabelPositionBottom:
          col = 4; row = 2; sticky = "ew"; anchor = "c";
          break;
        case vtkKWWidgetLabeled::LabelPositionDefault:
        case vtkKWWidgetLabeled::LabelPositionTop:
        default:
          col = 4; row = 0; sticky = "ew"; anchor = "c";
          break;
        }
      } else {
      switch (this->LabelPosition)
        {
        case vtkKWWidgetLabeled::LabelPositionDefault:
        case vtkKWWidgetLabeled::LabelPositionLeft:
        default:
          col = 0; row = 4; sticky = "nsw"; anchor = "w";
          break;
        case vtkKWWidgetLabeled::LabelPositionRight:
          col = 2; row = 4; sticky = "nsw"; anchor = "w";
          break;
        case vtkKWWidgetLabeled::LabelPositionBottom:
          col = 1; row = 6; sticky = "ew"; anchor = "w";
          break;
        case vtkKWWidgetLabeled::LabelPositionTop:
          col = 1; row = 0; sticky = "ew"; anchor = "w";
          break;
        }
      }
    tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
           << " -row " << row << " -column " << col 
           << " -sticky " << sticky << endl;
    tk_cmd << this->GetLabel()->GetWidgetName() 
           << " config -anchor " << anchor << endl;
    }

  // Entries

  if (this->ShowEntries)
    {
    vtkKWEntry *entry = this->Entries[this->Inverted ? 1 : 0];
    if (entry && entry->IsCreated())
      {
      if (is_horiz)
        {
        switch (this->Entry1Position)
          {
          case vtkKWRange::EntryPositionLeft:
            col = 1; row = 1; sticky = "nsw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionRight:
            col = 7; row = 1; sticky = "nsw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionBottom:
            col = 3; row = 2; sticky = "w"; c_pady = 1;
            break;
          case vtkKWRange::EntryPositionDefault:
          case vtkKWRange::EntryPositionTop:
          default:
            col = 3; row = 0; sticky = "w"; c_pady = 1;
            break;
          }
        } else {
        switch (this->Entry1Position)
          {
          case vtkKWRange::EntryPositionDefault:
          case vtkKWRange::EntryPositionLeft:
          default:
            col = 0; row = 3; sticky = "nw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionRight:
            col = 2; row = 3; sticky = "nw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionBottom:
            col = 1; row = 7; sticky = "w"; c_pady = 1;
            break;
          case vtkKWRange::EntryPositionTop:
            col = 1; row = 1; sticky = "w"; c_pady = 1;
            break;
          }
        }
      tk_cmd << "grid " << entry->GetWidgetName()
             << " -row " << row << " -column " << col 
             << " -sticky " << sticky << endl;
      }

    entry = this->Entries[this->Inverted ? 0 : 1];
    if (entry && entry->IsCreated())
      {

      if (is_horiz)
        {
        switch (this->Entry2Position)
          {
          case vtkKWRange::EntryPositionLeft:
            col = 2; row = 1; sticky = "nsw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionRight:
            col = 8; row = 1; sticky = "nsw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionBottom:
            col = 5; row = 2; sticky = "e"; c_pady = 1;
            break;
          case vtkKWRange::EntryPositionDefault:
          case vtkKWRange::EntryPositionTop:
          default:
            col = 5; row = 0; sticky = "e"; c_pady = 1;
            break;
          }
        } else {
        switch (this->Entry2Position)
          {
          case vtkKWRange::EntryPositionDefault:
          case vtkKWRange::EntryPositionLeft:
          default:
            col = 0; row = 5; sticky = "nw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionRight:
            col = 2; row = 5; sticky = "nw"; c_padx = 1;
            break;
          case vtkKWRange::EntryPositionBottom:
            col = 1; row = 8; sticky = "w"; c_pady = 1;
            break;
          case vtkKWRange::EntryPositionTop:
            col = 1; row = 2; sticky = "w"; c_pady = 1;
            break;
          }
        }
      tk_cmd << "grid " << entry->GetWidgetName()
             << " -row " << row << " -column " << col 
             << " -sticky " << sticky << endl;
      }
    }

  // Canvas

  if (this->CanvasFrame && this->CanvasFrame->IsCreated())
    {
    if (is_horiz) 
      { 
      col = 3; row = 1; col_span = 3; row_span = 1; sticky = "ew";
      }
    else 
      { 
      col = 1; row = 3; col_span = 1; row_span = 3; sticky = "ns";
      }
    tk_cmd << "grid " << this->CanvasFrame->GetWidgetName()
           << " -row " << row << " -column " << col 
           << " -rowspan " << row_span << " -columnspan " << col_span
           << " -sticky " << sticky 
           << " -padx " << c_padx * 2 << " -pady " << c_pady * 2
           << endl;

    // Make sure it will resize properly

    for (int i = 3; i <= 5; i++)
      {
      tk_cmd << "grid " << (is_horiz ? "columnconfigure" : "rowconfigure")
             << " " << this->CanvasFrame->GetParent()->GetWidgetName() 
             << " " << i << " -weight 1" << endl;
      }
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::Bind()
{
  if (!this->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsCreated())
    {
    const char *canv = this->Canvas->GetWidgetName();

    // Range

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_RANGE_TAG 
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " StartInteractionCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_RANGE_TAG 
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " EndInteractionCallback}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_RANGE_TAG
           << " <B1-Motion> {" << this->GetTclName() 
           << " RangeMotionCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_RANGE_TAG 
           << " <Double-1> {" << this->GetTclName() 
           << " MaximizeRangeCallback}" << endl;

    // Sliders

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDERS_TAG 
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " StartInteractionCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDERS_TAG 
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " EndInteractionCallback}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDER1_TAG 
           << " <B1-Motion> {" << this->GetTclName() 
           << " SliderMotionCallback " 
           << vtkKWRange::SLIDER_INDEX_1 << " %%x %%y}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDER2_TAG 
           << " <B1-Motion> {" << this->GetTclName() 
           << " SliderMotionCallback " 
           << vtkKWRange::SLIDER_INDEX_2 << " %%x %%y}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::UnBind()
{
  if (!this->IsAlive())
    {
    return;
    }

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsCreated())
    {
    const char *canv = this->Canvas->GetWidgetName();

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDERS_TAG 
           << " <ButtonPress-1> {}" << endl;
    
    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDERS_TAG 
           << " <ButtonRelease-1> {}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDER1_TAG 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " <<  VTK_KW_RANGE_SLIDER2_TAG 
           << " <B1-Motion> {}" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetWholeRange(double r0, double r1)
{
  if (this->WholeRange[0] == r0 && this->WholeRange[1] == r1)
    {
    return;
    }

  this->WholeRange[0] = r0;
  this->WholeRange[1] = r1;

  this->Modified();

  this->ConstrainRanges();

  this->RedrawCanvas();
  this->UpdateEntriesValue(this->Range);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetRange(double r0, double r1)
{
  if (this->Range[0] == r0 && this->Range[1] == r1)
    {
    return;
    }

  double old_range[2];
  old_range[0] = this->Range[0];
  old_range[1] = this->Range[1];

  this->Range[0] = r0;
  this->Range[1] = r1;

  this->Modified();

  int old_sliders_pos[2], sliders_pos[2];
  if (this->IsCreated())
    {
    this->GetSlidersPositions(old_sliders_pos);
    }

  this->ConstrainRange(old_range);

  // Update the widget aspect

  if (this->IsCreated())
    {
    this->RedrawRange();

    this->GetSlidersPositions(sliders_pos);

    if (old_sliders_pos[0] != sliders_pos[0])
      {
      this->RedrawSlider(sliders_pos[0], vtkKWRange::SLIDER_INDEX_1);
      }
    if (old_sliders_pos[1] != sliders_pos[1])
      {
      this->RedrawSlider(sliders_pos[1], vtkKWRange::SLIDER_INDEX_2);
      }

    this->UpdateEntriesValue(this->Range);
    }

  // Invoke callback if needed

  if (old_range[0] != this->Range[0] || old_range[1] != this->Range[1])
    {
    this->InvokeCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::GetRelativeRange(double &r0, double &r1)
{
  if (this->WholeRange[1] == this->WholeRange[0])
    {
    r0 = r1 = 0.0;
    }
  else
    {
    double whole_range = this->WholeRange[1] - this->WholeRange[0];
    r0 = (this->Range[0] - this->WholeRange[0]) / whole_range;
    r1 = (this->Range[1] - this->WholeRange[0]) / whole_range;
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::SetRelativeRange(double r0, double r1)
{
  double whole_range = this->WholeRange[1] - this->WholeRange[0];
  this->SetRange(r0 * whole_range + this->WholeRange[0],
                 r1 * whole_range + this->WholeRange[0]);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetResolution(double arg)
{
  if (this->Resolution == arg || arg <= 0.0)
    {
    return;
    }

  double old_res = this->Resolution;
  this->Resolution = arg;
  this->ConstrainResolution();

  if (this->Resolution == old_res)
    {
    return;
    }

  this->Modified();

  this->ConstrainRanges();
  
  this->RedrawCanvas();

  this->UpdateEntriesValue(this->Range);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetAdjustResolution(int arg)
{
  if (this->AdjustResolution == arg)
    {
    return;
    }

  this->AdjustResolution = arg;

  this->Modified();

  this->ConstrainResolution();
}

//----------------------------------------------------------------------------
void vtkKWRange::ConstrainResolution()
{
  if (this->AdjustResolution)
    {
    double reslog = log10((double)this->Resolution);
    double intp, fracp;
    fracp = modf(reslog, &intp);
    if (fabs(fracp) > 0.00001)
      {
      double newres = (double)pow(10.0, floor(reslog));
      if (newres != this->Resolution)
        {
        this->SetResolution(newres);
        }
      }
    }
}

// ---------------------------------------------------------------------------
void vtkKWRange::UpdateEntriesValue(double range[2])
{
  if (!range)
    {
    return;
    }

  for (int i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    if (this->Entries[i] && this->Entries[i]->IsCreated())
      {
      this->Entries[i]->SetValue(range[i]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::ConstrainRangeToResolution(double range[2], int adjust)
{
  int inv = (range[0] > range[1]) ? 1 : 0;

  double res = this->Resolution;
  double epsilon = res / 1000.0;

  for (int i = 0; i <= 1; i++)
    {
    double value = range[i];
    double q = value / res;
    double new_value = res * (q >= 0 ? floor(q + 0.5) : ceil(q - 0.5));
    // This adjustment is made to make sure that the new values
    // fall in the range. For example, if range is [1,62],
    // the line above assigns [0,63] to the new_values. The
    // code below adjusts this range to be [3,60]
    if (adjust)
      {
      if (new_value != value)
        {
        if (i - inv)
          {
          // If inv is false (0), this executes for range[1].
          // If the new_value is greater than range[1], adjust
          // it to the smaller multiple of res.
          if (new_value > value + epsilon)
            {
            int times = static_cast<int>(
              (new_value - (value + epsilon))/res);
            new_value = new_value - (times + 1)*res;
            }

          // This should newer be true. Just in case.
          if (new_value <= (value - res) + epsilon)
            {
            int times = static_cast<int>(
              ((value - epsilon) - new_value)/res);
            new_value = new_value + (times + 1)*res;
            }
          }
        else
          {
          // If inv is false (0), this executes for range[0].
          // If the new_value is smaller than range[0], adjust
          // it to the larger multiple of res.
          if (new_value < value - epsilon)
            {
            int times = static_cast<int>(
              ((value - epsilon) - new_value)/res);
            new_value = new_value + (times + 1)*res;
            }

          // This should newer be true. Just in case.
          if (new_value >= (value + res) - epsilon)
            {
            int times = static_cast<int>(
              (new_value - (value + epsilon))/res);
            new_value = new_value - (times + 1)*res;
            }
          }
        }
      }
    range[i] = new_value;
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::ConstrainRangeToWholeRange(
  double range[2], double whole_range[2], double *old_range_hint)
{
  int i;

  int inv = (whole_range[0] > whole_range[1]);
  int wmin_idx = (inv ? 1 : 0);
  int wmax_idx = (wmin_idx == 0 ? 1 : 0);

  // Resolution OK for this ? Is it out of WholeRange ?

  if (this->ClampRange)
    {
    for (i = 0; i <= 1; i++)
      {
      if (range[i] < whole_range[wmin_idx])
        {
        range[i] = whole_range[wmin_idx];
        }
      else if (range[i] > whole_range[wmax_idx])
        {
        range[i] = whole_range[wmax_idx];
        }
      }
    }

  // Range not in right order ?

  if (range[wmin_idx] > range[wmax_idx])
    {
    if (old_range_hint) // old_range_hint is used as an old range value
      {
      if (range[1] == old_range_hint[1]) // range[0] is moving
        {
        if (this->SliderCanPush)  
          {
          range[1] = range[0];
          }
        else
          {
          range[0] = range[1];
          }
        }
      else                                 // range[1] is moving
        {
        if (this->SliderCanPush)
          {
          range[0] = range[1];
          }
        else
          {
          range[1] = range[0];
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::ConstrainWholeRange()
{
  this->WholeRangeAdjusted[0] = this->WholeRange[0];
  this->WholeRangeAdjusted[1] = this->WholeRange[1];

  this->ConstrainRangeToResolution(this->WholeRangeAdjusted);
}

//----------------------------------------------------------------------------
void vtkKWRange::ConstrainRange(double *old_range_hint)
{
  this->ConstrainRangeToWholeRange(
    this->Range, this->WholeRange, old_range_hint);

  this->RangeAdjusted[0] = this->Range[0];
  this->RangeAdjusted[1] = this->Range[1];
  
  this->ConstrainRangeToResolution(this->RangeAdjusted);
}

//----------------------------------------------------------------------------
void vtkKWRange::ConstrainRanges()
{
  this->ConstrainWholeRange();
  this->ConstrainRange();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetOrientation(int arg)
{
  if (this->Orientation == arg ||
      arg < vtkKWRange::ORIENTATION_HORIZONTAL ||
      arg > vtkKWRange::ORIENTATION_VERTICAL)
    {
    return;
    }

  this->Orientation = arg;

  this->Modified();

  this->Pack();

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetInverted(int arg)
{
  if (this->Inverted == arg)
    {
    return;
    }

  this->Inverted = arg;

  this->Pack();

  this->RedrawCanvas();
}

// ----------------------------------------------------------------------------
void vtkKWRange::SetShowEntries(int _arg)
{
  if (this->ShowEntries == _arg)
    {
    return;
    }
  this->ShowEntries = _arg;
  this->Modified();

  if (this->ShowEntries)
    {
    this->CreateEntries();
    }

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWRange::SetEntry1Position(int arg)
{
  if (arg < vtkKWRange::EntryPositionDefault)
    {
    arg = vtkKWRange::EntryPositionDefault;
    }
  else if (arg > vtkKWRange::EntryPositionRight)
    {
    arg = vtkKWRange::EntryPositionRight;
    }

  if (this->Entry1Position == arg)
    {
    return;
    }

  this->Entry1Position = arg;

  this->Modified();

  this->Pack();
}

// ----------------------------------------------------------------------------
void vtkKWRange::SetEntry2Position(int arg)
{
  if (arg < vtkKWRange::EntryPositionDefault)
    {
    arg = vtkKWRange::EntryPositionDefault;
    }
  else if (arg > vtkKWRange::EntryPositionRight)
    {
    arg = vtkKWRange::EntryPositionRight;
    }

  if (this->Entry2Position == arg)
    {
    return;
    }

  this->Entry2Position = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetEntriesWidth(int arg)
{
  if (this->EntriesWidth == arg || arg <= 0)
    {
    return;
    }

  this->EntriesWidth = arg;

  this->Modified();

  for (int i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    if (this->Entries[i])
      {
      this->Entries[i]->SetWidth(this->EntriesWidth);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::SetThickness(int arg)
{
  if (this->Thickness == arg || arg < VTK_KW_RANGE_MIN_THICKNESS)
    {
    return;
    }

  this->Thickness = arg;

  this->Modified();

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetInternalThickness(double arg)
{
  if (this->InternalThickness == arg || 
      arg < 0.0 || arg > 1.0)
    {
    return;
    }

  this->InternalThickness = arg;

  this->Modified();

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetSliderSize(int arg)
{
  if (this->SliderSize == arg || 
      arg < VTK_KW_RANGE_MIN_SLIDER_SIZE)
    {
    return;
    }

  this->SliderSize = arg;

  this->Modified();

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetRangeColor(double r, double g, double b)
{
  if ((r == this->RangeColor[0] &&
       g == this->RangeColor[1] &&
       b == this->RangeColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->RangeColor[0] = r;
  this->RangeColor[1] = g;
  this->RangeColor[2] = b;

  this->Modified();

  this->UpdateColors();
}

//----------------------------------------------------------------------------
void vtkKWRange::SetRangeInteractionColor(double r, double g, double b)
{
  if ((r == this->RangeInteractionColor[0] &&
       g == this->RangeInteractionColor[1] &&
       b == this->RangeInteractionColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->RangeInteractionColor[0] = r;
  this->RangeInteractionColor[1] = g;
  this->RangeInteractionColor[2] = b;

  this->Modified();

  this->UpdateColors();
}

//----------------------------------------------------------------------------
void vtkKWRange::GetWholeRangeColor(int type, int &r, int &g, int &b)
{
  if (!this->IsCreated())
    {
    return;
    }

  double fr, fg, fb;
  double fh, fs, fv;

  switch (type)
    {
    case vtkKWRange::DARK_SHADOW_COLOR:
    case vtkKWRange::LIGHT_SHADOW_COLOR:
    case vtkKWRange::HIGHLIGHT_COLOR:

      this->GetWholeRangeColor(vtkKWRange::BACKGROUND_COLOR, r, g, b);

      fr = (double)r / 255.0;
      fg = (double)g / 255.0;
      fb = (double)b / 255.0;

      if (fr == fg && fg == fb)
        {
        fh = fs = 0.0;
        fv = fr;
        }
      else
        {
        vtkMath::RGBToHSV(fr, fg, fb, &fh, &fs, &fv);
        }

      if (type == vtkKWRange::DARK_SHADOW_COLOR)
        {
        fv *= 0.3;
        }
      else if (type == vtkKWRange::LIGHT_SHADOW_COLOR)
        {
        fv *= 0.6;
        }
      else
        {
        fv = 1.0;
        }

      vtkMath::HSVToRGB(fh, fs, fv, &fr, &fg, &fb);

      r = (int)(fr * 255.0);
      g = (int)(fg * 255.0);
      b = (int)(fb * 255.0);

      break;

    case vtkKWRange::BACKGROUND_COLOR:
    default:

      this->Canvas->GetBackgroundColor(&r, &g, &b);
      
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::GetRangeColor(int type, int &r, int &g, int &b)
{
  if (!this->IsCreated())
    {
    return;
    }

  double fr, fg, fb;
  double fh, fs, fv;
  double *rgb;

  switch (type)
    {
    case vtkKWRange::DARK_SHADOW_COLOR:
    case vtkKWRange::LIGHT_SHADOW_COLOR:
    case vtkKWRange::HIGHLIGHT_COLOR:

      this->GetRangeColor(vtkKWRange::BACKGROUND_COLOR, r, g, b);

      fr = (double)r / 255.0;
      fg = (double)g / 255.0;
      fb = (double)b / 255.0;

      if (fr == fg && fg == fb)
        {
        fh = fs = 0.0;
        fv = fr;
        }
      else
        {
        vtkMath::RGBToHSV(fr, fg, fb, &fh, &fs, &fv);
        }

      if (type == vtkKWRange::DARK_SHADOW_COLOR)
        {
        fv *= 0.3;
        }
      else if (type == vtkKWRange::LIGHT_SHADOW_COLOR)
        {
        fv *= 0.6;
        }
      else
        {
        fv = 1.0;
        }

      vtkMath::HSVToRGB(fh, fs, fv, &fr, &fg, &fb);

      r = (int)(fr * 255.0);
      g = (int)(fg * 255.0);
      b = (int)(fb * 255.0);

      break;

    case vtkKWRange::BACKGROUND_COLOR:
    default:

      rgb = (this->InInteraction ? 
             this->RangeInteractionColor : this->RangeColor);

      if (rgb[0] < 0 || rgb[1] < 0 || rgb[2] < 0)
        {
        this->GetWholeRangeColor(vtkKWRange::BACKGROUND_COLOR, r, g, b);
        }
      else
        {
        r = (int)(rgb[0] * 255.0);
        g = (int)(rgb[1] * 255.0);
        b = (int)(rgb[2] * 255.0);
        }

      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::GetSliderColor(int type, int &r, int &g, int &b)
{
  this->GetWholeRangeColor(type, r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWRange::InvokeCommand()
{
  if (this->Command && !this->DisableCommands)
    {
    this->Script("eval %s",this->Command);
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::InvokeStartCommand()
{
  if (this->StartCommand && !this->DisableCommands)
    {
    this->Script("eval %s", this->StartCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::InvokeEndCommand()
{
  if (this->EndCommand && !this->DisableCommands)
    {
    this->Script("eval %s", this->EndCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::InvokeEntriesCommand()
{
  if (this->EntriesCommand && !this->DisableCommands)
    {
    this->Script("eval %s", this->EntriesCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::SetCommand(vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->Command, object, method);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetStartCommand(vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->StartCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetEndCommand(vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->EndCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWRange::SetEntriesCommand(vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->EntriesCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWRange::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->CanvasFrame)
    {
    this->CanvasFrame->SetEnabled(this->Enabled);
    }

  if (this->Canvas)
    {
    this->Canvas->SetEnabled(this->Enabled);
    }

  for (int i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    if (this->Entries[i])
      {
      this->Entries[i]->SetEnabled(this->Enabled);
      }
    }

  if (this->Enabled)
    {
    this->Bind();
    }
  else
    {
    this->UnBind();
    }
}

// ---------------------------------------------------------------------------
void vtkKWRange::SetBalloonHelpString(const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Canvas)
    {
    this->Canvas->SetBalloonHelpString(string);
    }

  for (int i = 0; i < VTK_KW_RANGE_NB_ENTRIES; i++)
    {
    if (this->Entries[i])
      {
      this->Entries[i]->SetBalloonHelpString(string);
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWRange::HasTag(const char *tag, const char *suffix)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  const char *res = this->Script(
    "%s gettags %s%s", 
    this->Canvas->GetWidgetName(), tag, (suffix ? suffix : ""));
  if (!res || !*res)
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWRange::RedrawCanvas()
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // Resize the canvas

  int width, height;

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    width = atoi(this->Script("winfo width %s", 
                              this->CanvasFrame->GetWidgetName()));
    if (width < VTK_KW_RANGE_MIN_LENGTH)
      {
      width = VTK_KW_RANGE_MIN_LENGTH;
      }
    height = this->Thickness;
    }
  else
    {
    width = this->Thickness;
    height = atoi(this->Script("winfo height %s", 
                               this->CanvasFrame->GetWidgetName()));
    if (height < VTK_KW_RANGE_MIN_LENGTH)
      {
      height = VTK_KW_RANGE_MIN_LENGTH;
      }
    }

  this->Script("%s config -width %d -height %d -scrollregion {0 0 %d %d}",
               canv, width, height, width - 1, height - 1);

  // Draw the elements

  this->RedrawWholeRange();
  this->RedrawRange();
  this->RedrawSliders();

  this->UpdateColors();
}

//----------------------------------------------------------------------------
void vtkKWRange::RedrawWholeRange()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;
  const char *canv = this->Canvas->GetWidgetName();
  const char *tag = VTK_KW_RANGE_WHOLE_RANGE_TAG;
  int was_created = this->HasTag(tag);

  int in_thick = (int)(this->Thickness * InternalThickness);
  if (in_thick < VTK_KW_RANGE_MIN_INTERNAL_THICKNESS)
    {
    in_thick = VTK_KW_RANGE_MIN_INTERNAL_THICKNESS;
    }

  int x_min, x_max, y_min, y_max;

  /* 
     x_min          x_max
     |               |
     v               v
     DDDDDDDDDDDDDDDDH <- y_min
     DLLLLLLLLLLLLLL.H
     DL..............H
     DL..............H
     DL..............H 
     D...............H
     HHHHHHHHHHHHHHHHH <- y_max
  */

  // Draw depending on the orientation

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    x_min = 0;
    x_max = atoi(this->Script("%s cget -width", canv)) - 1;
    y_min = (this->Thickness - in_thick) / 2;
    y_max = y_min + in_thick - 1;
    }
  else
    {
    x_min = (this->Thickness - in_thick) / 2;
    x_max = x_min + in_thick - 1;
    y_min = 0;
    y_max = atoi(this->Script("%s cget -height", canv)) - 1;
    }

  // '.' part (background)

  if (!was_created)
    {
    tk_cmd << canv << " create rectangle 0 0 0 0 "
           << "-tag {rtag wbgc " << tag << " " << tag << "b1}\n";
    }
    
  tk_cmd << canv << " coords " << tag << "b1 "
         << x_min + 1 << " " << y_min + 1 << " " 
         << x_max - 1 + RSTRANGE << " " << y_max - 1 + RSTRANGE << endl;

  // 'D' part (dark shadow)

  if (!was_created)
    {
    tk_cmd << canv << " create line 0 0 0 0 "
           << "-tag {ltag wdsc " << tag << " " << tag << "l1}\n";
    }

  tk_cmd << canv << " coords " << tag << "l1 "
         << x_min << " " << y_max - 1 << " "
         << x_min << " " << y_min << " " 
         << x_max - 1 + LSTRANGE << " " << y_min << endl;

  // 'H' part (highlight)

  if (!was_created)
    {
    tk_cmd << canv << " create line 0 0 0 0 "
           << "-tag {ltag whlc " << tag << " " << tag << "l2}\n";
    }

  tk_cmd << canv << " coords " << tag << "l2 "
         << x_max << " " << y_min << " "
         << x_max << " " << y_max << " "
         << x_min - LSTRANGE << " " << y_max << endl;

  // 'L' part (light shadow)

  if (!was_created)
    {
    tk_cmd << canv << " create line 0 0 0 0 "
           << "-tag {ltag wlsc " << tag << " " << tag << "l3}\n";
    }

  tk_cmd << canv << " coords " << tag << "l3 "
         << x_min + 1 << " " << y_max - 2 << " "
         << x_min + 1 << " " << y_min + 1 << " " 
         << x_max - 2 + LSTRANGE << " " << y_min + 1 << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::GetSlidersPositions(int pos[2])
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();
  int i, pos_min = 0, pos_max, pos_range;

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    pos_max = atoi(this->Script("%s cget -width", canv)) - 1;
    }
  else
    {
    pos_max = atoi(this->Script("%s cget -height", canv)) - 1;
    }

  pos_range = pos_max - pos_min;

  double r0, r1;

  if (this->WholeRangeAdjusted[1] == this->WholeRangeAdjusted[0])
    {
    r0 = r1 = 0.0;
    }
  else
    {
    double whole_range = 
      (this->WholeRangeAdjusted[1] - this->WholeRangeAdjusted[0]);
    r0 = (this->RangeAdjusted[0] - this->WholeRangeAdjusted[0]) / whole_range;
    r1 = (this->RangeAdjusted[1] - this->WholeRangeAdjusted[0]) / whole_range;
    }

  pos[0] = (int)((double)pos_range * r0);
  pos[1] = (int)((double)pos_range * r1);

  if (this->Inverted)
    {
    pos[0] = pos_max - pos[0];
    pos[1] = pos_max - pos[1];
    }
  else
    {
    pos[0] += pos_min;
    pos[1] += pos_min;
    }

  // Leave room for the slider so that it remains inside the widget

  for (i = 0; i < 2; i++)
    {
    if (pos[i] - this->SliderSize < 0)
      {
      pos[i] = this->SliderSize;
      }
    else if (pos[i] + this->SliderSize > pos_max)
      {
      pos[i] = pos_max - this->SliderSize;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::RedrawRange()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;
  const char *canv = this->Canvas->GetWidgetName();
  const char *tag = VTK_KW_RANGE_RANGE_TAG;
  int was_created = this->HasTag(tag);

  int pos[2];
  this->GetSlidersPositions(pos);

  int in_thick = (int)(this->Thickness * InternalThickness);
  if (in_thick < VTK_KW_RANGE_MIN_INTERNAL_THICKNESS)
    {
    in_thick = VTK_KW_RANGE_MIN_INTERNAL_THICKNESS;
    }

  if (!was_created)
    {
    // '.' part (background)
  
    tk_cmd << canv << " create rectangle 0 0 0 0 "
           << "-tag {rtag rbgc " << tag << " " << tag << "b1}\n";
    
    // 'D' part (dark shadow)

    tk_cmd << canv << " create line 0 0 0 0 "
           << "-tag {ltag rdsc " << tag << " " << tag << "l1}\n";

    // 'H' part (highlight)

    tk_cmd << canv << " create line 0 0 0 0 "
           << "-tag {ltag rhlc " << tag << " " << tag << "l2}\n";
  
    // 'L' part (light shadow)
  
    tk_cmd << canv << " create line 0 0 0 0 "
           << "-tag {ltag rlsc " << tag << " " << tag << "l3}\n";
    }

  // Draw depending on the orientation

  int min = (this->Thickness - in_thick) / 2;
  int max = min + in_thick - 1;

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    /* 
      pos[0]         pos[1]
       |               |
       v               v
       HHHHHHHHHHHHHHHHH <- min
       .................
       .................
       LLLLLLLLLLLLLLLLL
       DDDDDDDDDDDDDDDDD <- max
    */

    // '.' part (background)
  
    tk_cmd << canv << " coords " << tag << "b1 "
           << pos[0] << " " << min + 1 << " " 
           << pos[1] + RSTRANGE << " " << max - 2 + RSTRANGE << endl;

    // 'D' part (dark shadow)

    tk_cmd << canv << " coords " << tag << "l1 "
           << pos[0] << " " << max << " "
           << pos[1]  + LSTRANGE << " " << max << endl;

    // 'H' part (highlight)

    tk_cmd << canv << " coords " << tag << "l2 "
           << pos[0] << " " << min << " "
           << pos[1]  + LSTRANGE << " " << min << endl;

    // 'L' part (light shadow)

    tk_cmd << canv << " coords " << tag << "l3 "
           << pos[0] << " " << max - 1 << " "
           << pos[1]  + LSTRANGE << " " << max - 1 << endl;
    }
  else
    {
    /* 
      min  max
       |   |
       v   v
       H..LD <- pos[0]
       H..LD
       H..LD
       H..LD
       H..LD <- pos[1]
    */

    // '.' part (background)
  
    tk_cmd << canv << " coords " << tag << "b1 "
           << min + 1 << " " << pos[0] << " " 
           << max - 2 + RSTRANGE << " " << pos[1] + RSTRANGE << endl;

    // 'D' part (dark shadow)

    tk_cmd << canv << " coords " << tag << "l1 "
           << max << " " << pos[0] << " "
           << max << " " << pos[1] + LSTRANGE << endl;

    // 'H' part (highlight)

    tk_cmd << canv << " coords " << tag << "l2 "
           << min << " " << pos[0] << " "
           << min << " " << pos[1] + LSTRANGE << endl;

    // 'L' part (light shadow)

    tk_cmd << canv << " coords " << tag << "l3 "
           << max -1 << " " << pos[0] << " "
           << max - 1 << " " << pos[1] + LSTRANGE << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::RedrawSliders()
{
  // Get the position of the sliders

  int pos[2];
  this->GetSlidersPositions(pos);

  // Draw the sliders

  this->RedrawSlider(pos[0], vtkKWRange::SLIDER_INDEX_1);
  this->RedrawSlider(pos[1], vtkKWRange::SLIDER_INDEX_2);
}

//----------------------------------------------------------------------------
void vtkKWRange::RedrawSlider(int pos, int slider_idx)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *tag = "";
  if (slider_idx == SLIDER_INDEX_1)
    {
    tag = VTK_KW_RANGE_SLIDER1_TAG;
    }
  else
    {
    tag = VTK_KW_RANGE_SLIDER2_TAG;
    }

  ostrstream tk_cmd;
  const char *canv = this->Canvas->GetWidgetName();
  int sw = this->SliderSize;
  const char *stag = VTK_KW_RANGE_SLIDERS_TAG;
  int was_created = this->HasTag(tag);

  int in_thick = (int)(this->Thickness * InternalThickness);
  if (in_thick < VTK_KW_RANGE_MIN_INTERNAL_THICKNESS)
    {
    in_thick = VTK_KW_RANGE_MIN_INTERNAL_THICKNESS;
    }
    
  int x_min, x_max, y_min, y_max, min_temp, max_temp;
  
  /*    x_min + sw - 1 
      x_min|pos (horiz)  
         | ||
         v vv 
         HHHHHHD <- y_min
         H.....D
         H.DDH.D <- y_min + sw - 1
         H.D H.D <- pos (vert)
         H.HHH.D <- y_max - sw + 1
         H.....D
         DDDDDDD <- y_max
             ^ ^
             | |
             | x_max
            x_max - sw + 1
  */

  // Draw depending on the orientation

  x_min = pos - sw;
  x_max = pos + sw;
#if 1
  y_min = 0;
  y_max = this->Thickness - 1;
#else
  y_min = (slider_idx == SLIDER_INDEX_1 ? 
           0 : (this->Thickness - in_thick) / 2);
  y_max = (slider_idx == SLIDER_INDEX_1 ? 
           (this->Thickness + in_thick) / 2 - 1 : this->Thickness - 1);
#endif

  if (this->Orientation == vtkKWRange::ORIENTATION_VERTICAL)
    {
    min_temp = x_min;
    max_temp = x_max;
    x_min = y_min;
    x_max = y_max;
    y_min = min_temp;
    y_max = max_temp;
    }

  // '.' part (background)

  if (sw > 2)
    {
    if (!this->HasTag(tag, "b1"))
      {
      tk_cmd << canv << " create rectangle 0 0 0 0 "
          << "-tag {rtag sbgc " << tag << " " << stag << " " << tag << "b1}\n";

      tk_cmd << canv << " create rectangle 0 0 0 0 "
          << "-tag {rtag sbgc " << tag << " " << stag << " " << tag << "b2}\n";

      tk_cmd << canv << " create rectangle 0 0 0 0 "
          << "-tag {rtag sbgc " << tag << " " << stag << " " << tag << "b3}\n";

      tk_cmd << canv << " create rectangle 0 0 0 0 "
          << "-tag {rtag sbgc " << tag << " " << stag << " " << tag << "b4}\n";
      }

    tk_cmd << canv << " coords " << tag << "b1 "
           << x_min + 1 << " " << y_min + 1 << " "
           << x_min + sw - 2 + RSTRANGE << " " << y_max - 1 + RSTRANGE << endl;

    tk_cmd << canv << " coords " << tag << "b2 "
           << x_max - sw + 2 << " " << y_min + 1 << " "
           << x_max - 1 + RSTRANGE << " " << y_max - 1 + RSTRANGE << endl;

    tk_cmd << canv << " coords " << tag << "b3 "
           << x_min + sw - 1 << " " << y_min + 1 << " "
           << x_max - sw + 1 + RSTRANGE << " " << y_min + sw - 2 + RSTRANGE
           << endl;

    tk_cmd << canv << " coords " << tag << "b4 "
           << x_min + sw - 1 << " " << y_max - sw + 2 << " "
           << x_max - sw + 1 + RSTRANGE << " " << y_max - 1 + RSTRANGE 
           << endl;
    }
    
  // 'D' part (dark shadow)

  if (!was_created)
    {
    tk_cmd << canv << " create line 0 0 0 0 "
         << " -tag {ltag sdsc " << tag << " " << stag << " " << tag << "l1}\n";
        
    tk_cmd << canv << " create line 0 0 0 0 "
        << " -tag {ltag sdsc " << tag << " " << stag << " " << tag << "l2}\n";
    }

  tk_cmd << canv << " coords " << tag << "l1 "
         << x_max << " " << y_min << " "
         << x_max << " " << y_max << " " 
         << x_min - LSTRANGE << " " << y_max << endl;

  tk_cmd << canv << " coords " << tag << "l2 "
         << x_min + sw - 1 << " " << y_max - sw << " "
         << x_min + sw - 1 << " " << y_min + sw - 1 << " " 
         << x_max - sw + LSTRANGE << " " << y_min + sw - 1 << endl;

  // 'H' part (highlight)

  if (!was_created)
    {
    tk_cmd << canv << " create line 0 0 0 0 "
        << " -tag {ltag shlc " << tag << " " << stag << " " << tag << "l3}\n";

    tk_cmd << canv << " create line 0 0 0 0 "
        << " -tag {ltag shlc " << tag << " " << stag << " " << tag << "l4}\n";
    }

  tk_cmd << canv << " coords " << tag << "l3 "
         << x_min << " " << y_max - 1 << " "
         << x_min << " " << y_min << " "
         << x_max - 1 + LSTRANGE << " " << y_min << endl;

  tk_cmd << canv << " coords " << tag << "l4 "
         << x_max - sw + 1 << " " << y_min + sw - 1 << " "
         << x_max - sw + 1 << " " << y_max - sw + 1 << " " 
         << x_min + sw - 1 - LSTRANGE << " " << y_max - sw + 1 << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::UpdateRangeColors()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;
  const char *canv = this->Canvas->GetWidgetName();

  char bgcolor[10], dscolor[10], lscolor[10], hlcolor[10];
  int r, g, b;

  // Set the color of the Range

  this->GetRangeColor(vtkKWRange::BACKGROUND_COLOR, r, g, b);
  sprintf(bgcolor, "#%02x%02x%02x", r, g, b);

  this->GetRangeColor(vtkKWRange::DARK_SHADOW_COLOR, r, g, b);
  sprintf(dscolor, "#%02x%02x%02x", r, g, b);

  this->GetRangeColor(vtkKWRange::HIGHLIGHT_COLOR, r, g, b);
  sprintf(hlcolor, "#%02x%02x%02x", r, g, b);

  this->GetRangeColor(vtkKWRange::LIGHT_SHADOW_COLOR, r, g, b);
  sprintf(lscolor, "#%02x%02x%02x", r, g, b);

  tk_cmd << canv << " itemconfigure rbgc -outline {} -fill "<< bgcolor << endl;
  tk_cmd << canv << " itemconfigure rdsc -fill " << dscolor << endl;
  tk_cmd << canv << " itemconfigure rhlc -fill " << hlcolor << endl;
  tk_cmd << canv << " itemconfigure rlsc -fill " << lscolor << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWRange::UpdateColors()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;
  const char *canv = this->Canvas->GetWidgetName();

  char bgcolor[10], dscolor[10], lscolor[10], hlcolor[10];
  int r, g, b;

  // Set the color of the Whole Range

  this->GetWholeRangeColor(vtkKWRange::BACKGROUND_COLOR, r, g, b);
  sprintf(bgcolor, "#%02x%02x%02x", r, g, b);

  this->GetWholeRangeColor(vtkKWRange::DARK_SHADOW_COLOR, r, g, b);
  sprintf(dscolor, "#%02x%02x%02x", r, g, b);

  this->GetWholeRangeColor(vtkKWRange::HIGHLIGHT_COLOR, r, g, b);
  sprintf(hlcolor, "#%02x%02x%02x", r, g, b);

  this->GetWholeRangeColor(vtkKWRange::LIGHT_SHADOW_COLOR, r, g, b);
  sprintf(lscolor, "#%02x%02x%02x", r, g, b);

  tk_cmd << canv << " itemconfigure wbgc -outline {} -fill "<< bgcolor << endl;
  tk_cmd << canv << " itemconfigure wdsc -fill " << dscolor << endl;
  tk_cmd << canv << " itemconfigure whlc -fill " << hlcolor << endl;
  tk_cmd << canv << " itemconfigure wlsc -fill " << lscolor << endl;

  // Set the color of the Range

  this->UpdateRangeColors();

  // Set the color of all Sliders

  this->GetSliderColor(vtkKWRange::BACKGROUND_COLOR, r, g, b);
  sprintf(bgcolor, "#%02x%02x%02x", r, g, b);

  this->GetSliderColor(vtkKWRange::DARK_SHADOW_COLOR, r, g, b);
  sprintf(dscolor, "#%02x%02x%02x", r, g, b);

  this->GetSliderColor(vtkKWRange::HIGHLIGHT_COLOR, r, g, b);
  sprintf(hlcolor, "#%02x%02x%02x", r, g, b);

  tk_cmd << canv << " itemconfigure sbgc -outline {} -fill "<< bgcolor << endl;
  tk_cmd << canv << " itemconfigure sdsc -fill " << dscolor << endl;
  tk_cmd << canv << " itemconfigure shlc -fill " << hlcolor << endl;

  // Set line style

  tk_cmd << canv << " itemconfigure ltag -capstyle round " << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

// ---------------------------------------------------------------------------
void vtkKWRange::EntriesUpdateCallback(int i)
{
  if (i < 0 || i >= VTK_KW_RANGE_NB_ENTRIES ||
      !this->Entries[i] || !this->Entries[i]->IsCreated())
    {
    return;
    }

  double value = this->Entries[i]->GetValueAsFloat();
  double old_value = this->Range[i];

  if (i == 0)
    {
    this->SetRange(value, this->Range[1]);
    }
  else
    {
    this->SetRange(this->Range[0], value);
    }

  if (this->Range[i] != old_value)
    {
    this->InvokeEntriesCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWRange::ConfigureCallback()
{
  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWRange::MaximizeRangeCallback()
{
  this->SetRange(this->GetWholeRange());
}

//----------------------------------------------------------------------------
void vtkKWRange::EnlargeRangeCallback()
{
  double *range = this->GetRange();
  double delta2 = (((double)range[1] - (double)range[0]) / 2.0) * 2.0;
  double center = ((double)range[1] + (double)range[0]) / 2.0;
  this->SetRange((double)(center - delta2), (double)(center + delta2));
}

//----------------------------------------------------------------------------
void vtkKWRange::ShrinkRangeCallback()
{
  double *range = this->GetRange();
  double delta2 = ((range[1] - range[0]) / 2.0) / 2.0;
  double center = (range[1] + range[0]) / 2.0;
  this->SetRange((center - delta2), (center + delta2));
}

//----------------------------------------------------------------------------
void vtkKWRange::StartInteractionCallback(int x, int y)
{
  if (this->InInteraction)
    {
    return;
    }

  this->InInteraction = 1;

  // Save the current range and mouse position

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    this->StartInteractionPos = x;
    }
  else
    {
    this->StartInteractionPos = y;
    }
  this->StartInteractionRange[0] = this->RangeAdjusted[0];
  this->StartInteractionRange[1] = this->RangeAdjusted[1];

  this->UpdateRangeColors();
  this->InvokeStartCommand();
}

//----------------------------------------------------------------------------
void vtkKWRange::EndInteractionCallback()
{
  if (!this->InInteraction)
    {
    return;
    }

  this->InInteraction = 0;
  this->UpdateRangeColors();
  this->InvokeEndCommand();
}

//----------------------------------------------------------------------------
void vtkKWRange::SliderMotionCallback(int slider_idx, int x, int y)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();
  double whole_range = 
    this->WholeRangeAdjusted[1] - this->WholeRangeAdjusted[0];

  // Update depending on the orientation

  int min, max, pos;

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    pos = x;
    min = 0;
    max = atoi(this->Script("%s cget -width", canv)) - 1;
    }
  else
    {
    pos = y;
    min = 0;
    max = atoi(this->Script("%s cget -height", canv)) - 1;
    }

  double new_value;
  if (this->Inverted)
    {
    new_value = (double)(max - pos);
    }
  else
    {
    new_value = (double)(pos - min);
    }
  new_value = (double)
    ((double)this->WholeRangeAdjusted[0] + 
     ((double)new_value / (double)(max - min)) * whole_range);

  double new_range[2];

  if (slider_idx == vtkKWRange::SLIDER_INDEX_1)
    {
    new_range[0] = new_value;
    new_range[1] = this->RangeAdjusted[1];
    }
  else
    {
    new_range[0] = this->RangeAdjusted[0];
    new_range[1] = new_value;
    }

  this->ConstrainRangeToWholeRange(
    new_range, this->WholeRangeAdjusted, this->RangeAdjusted);
  this->ConstrainRangeToResolution(new_range, 0);
  this->SetRange(new_range);
}

//----------------------------------------------------------------------------
void vtkKWRange::RangeMotionCallback(int x, int y)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();
  double whole_range = 
    this->WholeRangeAdjusted[1] - this->WholeRangeAdjusted[0];

  // Update depending on the orientation

  int pos, min, max;

  if (this->Orientation == vtkKWRange::ORIENTATION_HORIZONTAL)
    {
    pos = x;
    min = 0;
    max = atoi(this->Script("%s cget -width", canv)) - 1;
    }
  else
    {
    pos = y;
    min = 0;
    max = atoi(this->Script("%s cget -height", canv)) - 1;
    }

  double rel_delta = 
    whole_range * 
    (double)((pos - this->StartInteractionPos) - min) / (double)(max - min);
  if (this->Inverted)
    {
    rel_delta = -rel_delta;
    }

  double new_range[2];
  new_range[0] = (this->StartInteractionRange[0] + rel_delta);
  new_range[1] = (this->StartInteractionRange[1] + rel_delta);

  this->ConstrainRangeToWholeRange(
    new_range, this->WholeRangeAdjusted, this->RangeAdjusted);
  this->ConstrainRangeToResolution(new_range, 0);
    
  // Check if the constrained new range has the same "width" as the old one

  if (!this->SliderCanPush)
    {
    double old_delta = 
      (double)this->StartInteractionRange[1] - 
      (double)this->StartInteractionRange[0];
    if (fabs(old_delta - ((double)new_range[1] - (double)new_range[0])) 
        >= this->Resolution)
      {
      if (whole_range * rel_delta > 0)  // I just care of the sign
        {
        new_range[0] = (new_range[1] - old_delta);
        }
      else
        {
        new_range[1] = (new_range[0] + old_delta);
        }
      }
    }

  this->SetRange(new_range);
}

//----------------------------------------------------------------------------
void vtkKWRange::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WholeRange: " 
     << this->WholeRange[0] << "..." <<  this->WholeRange[1] << endl;
  os << indent << "Range: " 
     << this->Range[0] << "..." <<  this->Range[1] << endl;
  os << indent << "ClampRange: " << (this->ClampRange ? "On" : "Off") << endl;
  os << indent << "Resolution: " << this->Resolution << endl;
  os << indent << "Thickness: " << this->Thickness << endl;
  os << indent << "InternalThickness: " << this->InternalThickness << endl;
  os << indent << "Orientation: "<< this->Orientation << endl;
  os << indent << "Inverted: "
     << (this->Inverted ? "On" : "Off") << endl;
  os << indent << "SliderSize: "<< this->SliderSize << endl;
  os << indent << "DisableCommands: "
     << (this->DisableCommands ? "On" : "Off") << endl;
  os << indent << "RangeColor: ("
     << this->RangeColor[0] << ", " 
     << this->RangeColor[1] << ", " 
     << this->RangeColor[2] << ")" << endl;
  os << indent << "RangeInteractionColor: ("
     << this->RangeInteractionColor[0] << ", " 
     << this->RangeInteractionColor[1] << ", " 
     << this->RangeInteractionColor[2] << ")" << endl;
  os << indent << "ShowEntries: " 
     << (this->ShowEntries ? "On" : "Off") << endl;
  os << indent << "Entry1Position: " << this->Entry1Position << endl;
  os << indent << "Entry2Position: " << this->Entry2Position << endl;
  os << indent << "EntriesWidth: " << this->EntriesWidth << endl;
  os << indent << "SliderCanPush: "
     << (this->SliderCanPush ? "On" : "Off") << endl;
  os << indent << "AdjustResolution: "
     << (this->AdjustResolution ? "On" : "Off") << endl;

  os << indent << "Canvas: ";
  if (this->Canvas)
    {
    os << endl;
    this->Canvas->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}

