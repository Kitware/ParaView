/*=========================================================================

  Module:    vtkKWParameterValueHermiteFunctionEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWParameterValueHermiteFunctionEditor.h"

#include "vtkKWScaleWithEntry.h"
#include "vtkObjectFactory.h"
#include "vtkKWCanvas.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkKWParameterValueHermiteFunctionEditor, "1.3");

const char *vtkKWParameterValueHermiteFunctionEditor::MidPointTag = "midpoint_tag";
const char *vtkKWParameterValueHermiteFunctionEditor::MidPointGuidelineTag = "midpoint_guideline_tag";

// For some reasons, the end-point of a line/rectangle is not drawn on Win32. 
// Comply with that.

#ifndef _WIN32
#define LSTRANGE 0
#else
#define LSTRANGE 1
#endif
#define RSTRANGE 1

//----------------------------------------------------------------------------
vtkKWParameterValueHermiteFunctionEditor::vtkKWParameterValueHermiteFunctionEditor()
{
  this->MidPointEntry            = NULL;
  this->SharpnessEntry           = NULL;

  this->MidPointEntryVisibility  = 1;
  this->SharpnessEntryVisibility = 1;

  this->MidPointVisibility          = 1;
  this->MidPointGuidelineVisibility = 0;
  this->MidPointGuidelineValueVisibility = 0;

  this->MidPointColor[0]     = 0.2;
  this->MidPointColor[1]     = 0.2;
  this->MidPointColor[2]     = 0.2;
}

//----------------------------------------------------------------------------
vtkKWParameterValueHermiteFunctionEditor::~vtkKWParameterValueHermiteFunctionEditor()
{
  if (this->MidPointEntry)
    {
    this->MidPointEntry->Delete();
    this->MidPointEntry = NULL;
    }

  if (this->SharpnessEntry)
    {
    this->SharpnessEntry->Delete();
    this->SharpnessEntry = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdatePointEntries(int id)
{
  this->Superclass::UpdatePointEntries(id);

  this->UpdateMidPointEntry(id);
  this->UpdateSharpnessEntry(id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::Create(vtkKWApplication *app)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PiecewiseFunctionEditor already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  // Create the midpoint entry

  if (this->MidPointEntryVisibility && this->PointEntriesVisibility)
    {
    this->CreateMidPointEntry(app);
    }

  // Create the sharpness entry

  if (this->SharpnessEntryVisibility && this->PointEntriesVisibility)
    {
    this->CreateSharpnessEntry(app);
    }

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
vtkKWScaleWithEntry* vtkKWParameterValueHermiteFunctionEditor::GetMidPointEntry()
{
  if (!this->MidPointEntry)
    {
    this->MidPointEntry = vtkKWScaleWithEntry::New();
    this->MidPointEntry->SetResolution(0.01);
    this->MidPointEntry->SetRange(0.0, 1.0);
    this->MidPointEntry->ClampValueOn();
    if (this->MidPointEntryVisibility && 
        this->PointEntriesVisibility && 
        this->IsCreated())
      {
      this->CreateMidPointEntry(this->GetApplication());
      }
    }
  return this->MidPointEntry;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::CreateMidPointEntry(
  vtkKWApplication *app)
{
  if (this->GetMidPointEntry() && !this->MidPointEntry->IsCreated())
    {
    this->CreatePointEntriesFrame(app);

    // If we are displaying the entry in the top right frame, make sure it
    // has been created. 

    this->MidPointEntry->SetParent(this->PointEntriesFrame);
    this->MidPointEntry->PopupModeOn();
    this->MidPointEntry->Create(app);
    this->MidPointEntry->SetEntryWidth(4);
    this->MidPointEntry->SetLabelText("M:");
    this->MidPointEntry->SetLength(100);
    this->MidPointEntry->RangeVisibilityOff();
    this->MidPointEntry->SetBalloonHelpString("Mid-point position");

    this->UpdateMidPointEntry(this->GetSelectedPoint());

    this->MidPointEntry->SetCommand(this, "MidPointEntryChangingCallback");
    this->MidPointEntry->SetEndCommand(this, "MidPointEntryChangedCallback");
    this->MidPointEntry->SetEntryCommand(this, "MidPointEntryChangedCallback");
    }
}

//----------------------------------------------------------------------------
vtkKWScaleWithEntry* vtkKWParameterValueHermiteFunctionEditor::GetSharpnessEntry()
{
  if (!this->SharpnessEntry)
    {
    this->SharpnessEntry = vtkKWScaleWithEntry::New();
    this->SharpnessEntry->SetResolution(0.01);
    this->SharpnessEntry->SetRange(0.0, 1.0);
    this->SharpnessEntry->ClampValueOn();
    if (this->SharpnessEntryVisibility && 
        this->PointEntriesVisibility &&
        this->IsCreated())
      {
      this->CreateSharpnessEntry(this->GetApplication());
      }
    }
  return this->SharpnessEntry;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::CreateSharpnessEntry(
  vtkKWApplication *app)
{
  if (this->GetSharpnessEntry() && !this->SharpnessEntry->IsCreated())
    {
    this->CreatePointEntriesFrame(app);

    // If we are displaying the entry in the top right frame, make sure it
    // has been created. 

    this->SharpnessEntry->SetParent(this->PointEntriesFrame);
    this->SharpnessEntry->PopupModeOn();
    this->SharpnessEntry->Create(app);
    this->SharpnessEntry->SetEntryWidth(4);
    this->SharpnessEntry->SetLabelText("S:");
    this->SharpnessEntry->SetLength(100);
    this->SharpnessEntry->RangeVisibilityOff();
    this->SharpnessEntry->SetBalloonHelpString("Sharpness");

    this->UpdateSharpnessEntry(this->GetSelectedPoint());

    this->SharpnessEntry->SetCommand(this, "SharpnessEntryChangingCallback");
    this->SharpnessEntry->SetEndCommand(this, "SharpnessEntryChangedCallback");
    this->SharpnessEntry->SetEntryCommand(
      this, "SharpnessEntryChangedCallback");
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::IsPointEntriesFrameUsed()
{
  return (this->Superclass::IsPointEntriesFrameUsed() || 
          (this->PointEntriesVisibility && (this->MidPointEntryVisibility ||
                                            this->SharpnessEntryVisibility)));
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Pack the whole widget

  this->Superclass::Pack();

  ostrstream tk_cmd;

  // Midpoint entry
  
  if (this->MidPointEntryVisibility && 
      this->PointEntriesVisibility &&
      this->MidPointEntry && this->MidPointEntry->IsCreated())
    {
    tk_cmd << "pack " << this->MidPointEntry->GetWidgetName() 
           << " -side left -padx 2 " << endl;
    }
  
  // Sharpness entry
  
  if (this->SharpnessEntryVisibility && 
      this->PointEntriesVisibility && 
      this->SharpnessEntry && this->SharpnessEntry->IsCreated())
    {
    tk_cmd << "pack " << this->SharpnessEntry->GetWidgetName() 
           << " -side left -padx 2 " << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->MidPointEntry);
  this->PropagateEnableState(this->SharpnessEntry);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetMidPointEntryVisibility(int arg)
{
  if (this->MidPointEntryVisibility == arg)
    {
    return;
    }

  this->MidPointEntryVisibility = arg;

  // Make sure that if the entry has to be shown, we create it on the fly if
  // needed, including all dependents widgets (like its container)

  if (this->MidPointEntryVisibility && 
      this->PointEntriesVisibility && 
      this->IsCreated())
    {
    this->CreateMidPointEntry(this->GetApplication());
    }

  this->UpdateMidPointEntry(this->GetSelectedPoint());

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdateMidPointEntry(int id)
{
  if (!this->MidPointEntry || !this->HasFunction())
    {
    return;
    }

  double pos;
  if (id < 0 || id >= this->GetFunctionSize() ||
      !this->GetFunctionMidPoint(id, &pos))
    {
    this->MidPointEntry->SetEnabled(0);
    }
  else
    {
    this->MidPointEntry->SetValue(pos);
    this->MidPointEntry->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::MidPointEntryChangedCallback()
{
  if (this->MidPointEntry && this->HasSelection())
    {
    unsigned long mtime = this->GetFunctionMTime();
    double pos = this->MidPointEntry->GetValue();
    this->SetFunctionMidPoint(this->GetSelectedPoint(), pos);
    if (this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(this->GetSelectedPoint());
      this->InvokePointChangedCommand(this->GetSelectedPoint());
      this->InvokeFunctionChangedCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::MidPointEntryChangingCallback()
{
  if (this->MidPointEntry && this->HasSelection())
    {
    unsigned long mtime = this->GetFunctionMTime();
    double pos = this->MidPointEntry->GetValue();
    this->SetFunctionMidPoint(this->GetSelectedPoint(), pos);
    if (this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(this->GetSelectedPoint());
      this->InvokePointChangingCommand(this->GetSelectedPoint());
      this->InvokeFunctionChangingCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetSharpnessEntryVisibility(
  int arg)
{
  if (this->SharpnessEntryVisibility == arg)
    {
    return;
    }

  this->SharpnessEntryVisibility = arg;

  // Make sure that if the entry has to be shown, we create it on the fly if
  // needed, including all dependents widgets (like its container)

  if (this->SharpnessEntryVisibility && 
      this->PointEntriesVisibility && 
      this->IsCreated())
    {
    this->CreateSharpnessEntry(this->GetApplication());
    }

  this->UpdateSharpnessEntry(this->GetSelectedPoint());

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdateSharpnessEntry(int id)
{
  if (!this->SharpnessEntry || !this->HasFunction())
    {
    return;
    }

  double sharpness;
  if (id < 0 || id >= this->GetFunctionSize() ||
      !this->GetFunctionSharpness(id, &sharpness))
      
    { 
    this->SharpnessEntry->SetEnabled(0);
    }
  else
    {
    this->SharpnessEntry->SetValue(sharpness);
    this->SharpnessEntry->SetEnabled(this->GetEnabled());
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SharpnessEntryChangedCallback()
{
  if (this->SharpnessEntry && this->HasSelection())
    {
    unsigned long mtime = this->GetFunctionMTime();
    if (this->SetFunctionSharpness(
          this->GetSelectedPoint(), this->SharpnessEntry->GetValue()) &&
        this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(this->GetSelectedPoint());
      this->InvokePointChangedCommand(this->GetSelectedPoint());
      this->InvokeFunctionChangedCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SharpnessEntryChangingCallback()
{
  if (this->SharpnessEntry && this->HasSelection())
    {
    unsigned long mtime = this->GetFunctionMTime();
    if (this->SetFunctionSharpness(
          this->GetSelectedPoint(), this->SharpnessEntry->GetValue()) &&
        this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(this->GetSelectedPoint());
      this->InvokePointChangingCommand(this->GetSelectedPoint());
      this->InvokeFunctionChangingCommand();
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::MergePointFromEditor(
  vtkKWParameterValueFunctionEditor *editor, int editor_id, int &new_id)
{
  int added = 
    this->Superclass::MergePointFromEditor(editor, editor_id, new_id);

  vtkKWParameterValueHermiteFunctionEditor *h_editor = 
    vtkKWParameterValueHermiteFunctionEditor::SafeDownCast(editor);
  if (h_editor && added)
    {
    double midpoint, editor_midpoint, sharpness, editor_sharpness;

    h_editor->GetFunctionMidPoint(editor_id, &editor_midpoint);
    h_editor->GetFunctionSharpness(editor_id, &editor_sharpness);

    this->GetFunctionMidPoint(new_id, &midpoint);
    this->GetFunctionSharpness(new_id, &sharpness);
    
    if (midpoint != editor_midpoint || sharpness != editor_sharpness)
      {
      this->SetFunctionMidPoint(new_id, editor_midpoint);
      this->SetFunctionSharpness(new_id, editor_sharpness);
      this->RedrawSinglePointDependentElements(new_id);
      }
    }
  return added;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::CopyPointFromEditor(
  vtkKWParameterValueFunctionEditor *editor, int id)
{
  int copied = this->Superclass::CopyPointFromEditor(editor, id);

  vtkKWParameterValueHermiteFunctionEditor *h_editor = 
    vtkKWParameterValueHermiteFunctionEditor::SafeDownCast(editor);
  if (h_editor && copied)
    {
    double midpoint, editor_midpoint, sharpness, editor_sharpness;

    h_editor->GetFunctionMidPoint(id, &editor_midpoint);
    h_editor->GetFunctionSharpness(id, &editor_sharpness);

    this->GetFunctionMidPoint(id, &midpoint);
    this->GetFunctionSharpness(id, &sharpness);
    
    if (midpoint != editor_midpoint || sharpness != editor_sharpness)
      {
      this->SetFunctionMidPoint(id, editor_midpoint);
      this->SetFunctionSharpness(id, editor_sharpness);
      this->RedrawSinglePointDependentElements(id);
      }
    }
  return copied;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetMidPointVisibility(int arg)
{
  if (this->MidPointVisibility == arg)
    {
    return;
    }

  this->MidPointVisibility = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetMidPointGuidelineVisibility(int arg)
{
  if (this->MidPointGuidelineVisibility == arg)
    {
    return;
    }

  this->MidPointGuidelineVisibility = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetMidPointGuidelineValueVisibility(int arg)
{
  if (this->MidPointGuidelineValueVisibility == arg)
    {
    return;
    }

  this->MidPointGuidelineValueVisibility = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetMidPointColor(
  double r, double g, double b)
{
  if ((r == this->MidPointColor[0] &&
       g == this->MidPointColor[1] &&
       b == this->MidPointColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->MidPointColor[0] = r;
  this->MidPointColor[1] = g;
  this->MidPointColor[2] = b;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::RedrawLine(
  int id, int id2, ostrstream *tk_cmd)
{
  // Redraw the line

  this->Superclass::RedrawLine(id, id2, tk_cmd);

  // Then redraw the midpoint on this line

  if (!this->IsCreated() || !this->HasFunction() || this->DisableRedraw)
    {
    return;
    }

  // Do we have a midpoint for this segment ?

  double midpoint;
  if (!this->GetFunctionMidPoint(id, &midpoint))
    {
    return;
    }

  // If there is no stream, then it means we want to execute that command
  // right now (so create a stream)

  int stream_was_created = 0;
  if (!tk_cmd)
    {
    tk_cmd = new ostrstream;
    stream_was_created = 1;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // Is visible ? Is valid (not that there is no midpoint for the last point)

  int x, y, r;
  int is_not_visible = 0, is_not_visible_h = 0;
  int is_not_valid = (id < 0 || id >= (this->GetFunctionSize() - 1));

  // Get the midpoint coords, radius, check if the midpoint is visible
  // The radius is 80% of the point radius

  if (!is_not_valid)
    {
    double p1, p2, p;
    this->GetFunctionPointParameter(id, &p1);
    this->GetFunctionPointParameter(id + 1, &p2);
    p = p1 + (p2 - p1) * midpoint;
    this->GetFunctionPointCanvasCoordinatesAtParameter(p, x, y);

    r = (int)((double)this->PointRadius * 0.75);

    // If the midpoint is not in the visible range, hide it

    double c_x, c_y, c_x2, c_y2;
    this->GetCanvasScrollRegion(&c_x, &c_y, &c_x2, &c_y2);

    int visible_margin = r + this->PointOutlineWidth + 5;

    if (x + visible_margin < c_x || c_x2 < x - visible_margin)
      {
      is_not_visible_h = 1;
      }
    
    if (is_not_visible_h || 
        y + visible_margin < c_y || c_y2 < y - visible_margin)
      {
      is_not_visible = 1;
      }
    }

  // Create the point

  if (is_not_valid)
    {
    *tk_cmd << canv << " delete m_p" << id << endl;
    }
  else
    {
    if (is_not_visible || 
        !this->MidPointVisibility || 
        !this->CanvasVisibility)
      {
      *tk_cmd << canv << " itemconfigure m_p" << id << " -state hidden" <<endl;
      }
    else
      {
      if (!this->CanvasHasTag("m_p", &id))
        {
        *tk_cmd << canv << " create rectangle" << " 0 0 0 0 -tags {m_p" << id 
                << " "<< vtkKWParameterValueHermiteFunctionEditor::MidPointTag 
                << " " << vtkKWParameterValueFunctionEditor::FunctionTag 
                << "}" << endl;
        *tk_cmd << canv << " lower m_p" << id << " {p" << id << "||p" << id + 1
                << "}" << endl;
        }
      *tk_cmd << canv << " coords m_p" << id 
              << " " << x - r << " " << y - r 
              << " " << x + r << " " << y + r
              << endl;
      char color[10];
      sprintf(color, "#%02x%02x%02x", 
              (int)(this->MidPointColor[0] * 255.0),
              (int)(this->MidPointColor[1] * 255.0),
              (int)(this->MidPointColor[2] * 255.0));
      *tk_cmd << canv << " itemconfigure m_p" << id 
              << " -state normal  -width " << this->PointOutlineWidth
              << " -outline " << color << endl;
      }
    }

  // Create and/or update the point guideline

  if (is_not_valid)
    {
    *tk_cmd << canv << " delete m_g" << id << endl;
    }
  else
    {
    if (is_not_visible_h || 
        !this->MidPointGuidelineVisibility || 
        !this->CanvasVisibility)
      {
      *tk_cmd << canv << " itemconfigure m_g" << id << " -state hidden" <<endl;
      }
    else
      {
      if (!this->CanvasHasTag("m_g", &id))
        {
        *tk_cmd 
          << canv << " create line 0 0 0 0 -fill black -width 1 " 
          << " -tags {m_g" << id << " " 
          << vtkKWParameterValueHermiteFunctionEditor::MidPointGuidelineTag 
          << " " << vtkKWParameterValueFunctionEditor::FunctionTag
          << "}" << endl;
        *tk_cmd << canv << " lower m_g" << id << " m_p" << id << endl;
        }
  
      double factors[2] = {0.0, 0.0};
      this->GetCanvasScalingFactors(factors);
      double *v_w_range = this->GetWholeValueRange();
      int y1 = vtkMath::Round(v_w_range[0] * factors[1]);
      int y2 = vtkMath::Round(v_w_range[1] * factors[1]);
      *tk_cmd << canv << " coords m_g" << id << " "
              << x << " " << y1 << " " << x << " " << y2 << endl;
      *tk_cmd << canv << " itemconfigure m_g" << id;
      if (this->PointGuidelineStyle == 
          vtkKWParameterValueFunctionEditor::LineStyleDash)
        {
        *tk_cmd << " -dash {.}";
        }
      else
        {
        *tk_cmd << " -dash {}";
        }
      *tk_cmd << " -state normal" << endl;
      }
    }

  // Execute the command, free the stream

  if (stream_was_created)
    {
    *tk_cmd << ends;
    this->Script(tk_cmd->str());
    tk_cmd->rdbuf()->freeze(0);
    delete tk_cmd;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SharpnessEntryVisibility: "
     << (this->SharpnessEntryVisibility ? "On" : "Off") << endl;

  os << indent << "MidPointEntryVisibility: "
     << (this->MidPointEntryVisibility ? "On" : "Off") << endl;

  os << indent << "MidPointVisibility: "
     << (this->MidPointVisibility ? "On" : "Off") << endl;

  os << indent << "MidPointGuidelineVisibility: "
     << (this->MidPointGuidelineVisibility ? "On" : "Off") << endl;

  os << indent << "MidPointGuidelineValueVisibility: "
     << (this->MidPointGuidelineValueVisibility ? "On" : "Off") << endl;

  os << indent << "MidPointColor: ("
     << this->MidPointColor[0] << ", " 
     << this->MidPointColor[1] << ", " 
     << this->MidPointColor[2] << ")" << endl;

  os << indent << "MidPointEntry: ";
  if (this->MidPointEntry)
    {
    os << endl;
    this->MidPointEntry->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "SharpnessEntry: ";
  if (this->SharpnessEntry)
    {
    os << endl;
    this->SharpnessEntry->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}

