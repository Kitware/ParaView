/*=========================================================================

  Module:    vtkKWParameterValueHermiteFunctionEditor.cxx,v

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
#include "vtkKWInternationalization.h"
#include "vtkMath.h"
#include "vtkCallbackCommand.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkKWParameterValueHermiteFunctionEditor, "1.22");

const char *vtkKWParameterValueHermiteFunctionEditor::MidPointTag = "midpoint_tag";
const char *vtkKWParameterValueHermiteFunctionEditor::MidPointGuidelineTag = "midpoint_guideline_tag";
const char *vtkKWParameterValueHermiteFunctionEditor::MidPointSelectedTag = "mpselected_tag";

// For some reasons, the end-point of a line/rectangle is not drawn on Win32. 
// Comply with that.

#ifndef _WIN32
#define LSTRANGE 0
#else
#define LSTRANGE 1
#endif
#define RSTRANGE 1

#define VTK_KW_PVHFE_GUIDELINE_VALUE_TEXT_SIZE          7
#define VTK_KW_PVHFE_POINT_RADIUS_FACTOR          0.76

//----------------------------------------------------------------------------
vtkKWParameterValueHermiteFunctionEditor::vtkKWParameterValueHermiteFunctionEditor()
{
  this->MidPointEntry            = NULL;
  this->SharpnessEntry           = NULL;

  this->MidPointEntryVisibility  = 1;
  this->DisplayMidPointValueInParameterDomain  = 1;
  this->SharpnessEntryVisibility = 1;

  this->MidPointVisibility          = 1;
  this->MidPointGuidelineVisibility = 0;
  this->MidPointGuidelineValueVisibility = 0;

  this->MidPointColor[0]     = this->FrameBackgroundColor[0];
  this->MidPointColor[1]     = this->FrameBackgroundColor[1];
  this->MidPointColor[2]     = this->FrameBackgroundColor[2];

  this->SelectedMidPointColor[0] = this->SelectedPointColor[0];
  this->SelectedMidPointColor[1] = this->SelectedPointColor[1];
  this->SelectedMidPointColor[2] = this->SelectedPointColor[2];

  this->SelectedMidPoint = -1;
  this->LastMidPointSelectionCanvasCoordinateX    = 0;
  this->LastMidPointSelectionCanvasCoordinateY    = 0;
  this->LastMidPointSelectionSharpness    = 0.0;

  this->MidPointGuidelineValueFormat        = NULL;
  this->SetMidPointGuidelineValueFormat("%-#6.3g");

  this->MidPointSelectionChangedCommand = NULL;
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

  if (this->MidPointGuidelineValueFormat)
    {
    delete [] this->MidPointGuidelineValueFormat;
    this->MidPointGuidelineValueFormat = NULL;
    }

  if (this->MidPointSelectionChangedCommand)
    {
    delete [] this->MidPointSelectionChangedCommand;
    this->MidPointSelectionChangedCommand = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdateMidPointEntries(int id)
{
  this->UpdateMidPointEntry(id);
  this->UpdateSharpnessEntry(id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PiecewiseFunctionEditor already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // Create the midpoint entry

  if (this->MidPointEntryVisibility && this->PointEntriesVisibility)
    {
    this->CreateMidPointEntry();
    }

  // Create the sharpness entry

  if (this->SharpnessEntryVisibility && this->PointEntriesVisibility)
    {
    this->CreateSharpnessEntry();
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
    this->MidPointEntry->ClampValueOn();
    if (this->MidPointEntryVisibility && 
        this->PointEntriesVisibility && 
        this->IsCreated())
      {
      this->CreateMidPointEntry();
      }
    }
  return this->MidPointEntry;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::CreateMidPointEntry()
{
  if (this->GetMidPointEntry() && !this->MidPointEntry->IsCreated())
    {
    this->CreatePointEntriesFrame();

    // If we are displaying the entry in the top right frame, make sure it
    // has been created. 

    this->MidPointEntry->SetParent(this->PointEntriesFrame);
    this->MidPointEntry->PopupModeOn();
    this->MidPointEntry->Create();
    this->MidPointEntry->SetEntryWidth(7);
    this->MidPointEntry->SetLabelText(
      ks_("Transfer Function Editor|MidPoint|M:"));
    this->MidPointEntry->SetLength(100);
    this->MidPointEntry->RangeVisibilityOff();
    this->MidPointEntry->SetBalloonHelpString(
      k_("Midpoint position. Enter a new value, drag the scale slider, or "
         "drag the midpoint horizontally with the left mouse button."));

    this->UpdateMidPointEntry(this->GetSelectedMidPoint());

    this->MidPointEntry->SetCommand(
      this, "MidPointEntryChangingCallback");
    this->MidPointEntry->SetEndCommand(
      this, "MidPointEntryChangedCallback");
    this->MidPointEntry->SetEntryCommand(
      this, "MidPointEntryChangedCallback");
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
      this->CreateSharpnessEntry();
      }
    }
  return this->SharpnessEntry;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::CreateSharpnessEntry()
{
  if (this->GetSharpnessEntry() && !this->SharpnessEntry->IsCreated())
    {
    this->CreatePointEntriesFrame();

    // If we are displaying the entry in the top right frame, make sure it
    // has been created. 

    this->SharpnessEntry->SetParent(this->PointEntriesFrame);
    this->SharpnessEntry->PopupModeOn();
    this->SharpnessEntry->Create();
    this->SharpnessEntry->SetEntryWidth(7);
    this->SharpnessEntry->SetLabelText(
      ks_("Transfer Function Editor|Sharpness|S:"));
    this->SharpnessEntry->SetLength(100);
    this->SharpnessEntry->RangeVisibilityOff();
    this->SharpnessEntry->SetBalloonHelpString(
      k_("Sharpness. Enter a new value, drag the scale slider, or drag "
         "the midpoint vertically with the right mouse button."));

    this->UpdateSharpnessEntry(this->GetSelectedMidPoint());

    this->SharpnessEntry->SetCommand(
      this, "SharpnessEntryChangingCallback");
    this->SharpnessEntry->SetEndCommand(
      this, "SharpnessEntryChangedCallback");
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
void vtkKWParameterValueHermiteFunctionEditor::PackPointEntries()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Pack the other entries

  this->Superclass::PackPointEntries();

  ostrstream tk_cmd;

  // Midpoint entry
  
  if (this->HasMidPointSelection() &&
      this->MidPointEntryVisibility && 
      this->PointEntriesVisibility &&
      this->MidPointEntry && this->MidPointEntry->IsCreated())
    {
    tk_cmd << "pack " << this->MidPointEntry->GetWidgetName() 
           << " -side left -padx 2 " << endl;
    }
  
  // Sharpness entry
  
  if (this->HasMidPointSelection() &&
      this->SharpnessEntryVisibility && 
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
void vtkKWParameterValueHermiteFunctionEditor::Update()
{
  this->Superclass::Update();

  this->UpdateMidPointEntries(this->GetSelectedMidPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->MidPointEntry);
  this->PropagateEnableState(this->SharpnessEntry);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::FunctionPointMidPointIsLocked(int vtkNotUsed(id))
{
  return 0;
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
    this->CreateMidPointEntry();
    }

  this->UpdateMidPointEntry(this->GetSelectedMidPoint());

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetDisplayMidPointValueInParameterDomain(int arg)
{
  if (this->DisplayMidPointValueInParameterDomain == arg)
    {
    return;
    }

  this->DisplayMidPointValueInParameterDomain = arg;

  this->UpdateMidPointEntry(this->GetSelectedMidPoint());

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UpdateMidPointEntry(int id)
{
  if (!this->MidPointEntry || !this->HasFunction())
    {
    return;
    }

  double midpoint;
  if (id < 0 || id >= this->GetFunctionSize() ||
      !this->GetFunctionPointMidPoint(id, &midpoint))
    {
    this->MidPointEntry->SetEnabled(0);
    return;
    }

  if (this->DisplayMidPointValueInParameterDomain)
    {
    double p1, p2;
    this->GetFunctionPointParameter(id, &p1);
    this->GetFunctionPointParameter(id + 1, &p2);
    this->MapParameterToDisplayedParameter(p1, &p1);
    this->MapParameterToDisplayedParameter(p2, &p2);
    double parameter = p1 + (p2 - p1) * midpoint;
    this->MidPointEntry->SetResolution((p2 - p1) / 100.0);
    this->MidPointEntry->SetRange(p1, p2);
    this->MidPointEntry->SetValue(parameter);
    }
  else
    {
    this->MidPointEntry->SetResolution(0.01);
    this->MidPointEntry->SetRange(0.0, 1.0);
    this->MidPointEntry->SetValue(midpoint);
    }

  this->MidPointEntry->SetEnabled(
    this->FunctionPointMidPointIsLocked(id) ? 0 : this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::MidPointEntryChangedCallback(
  double value)
{
  if (this->HasMidPointSelection())
    {
    int id = this->GetSelectedMidPoint();
    unsigned long mtime = this->GetFunctionMTime();
    if (this->DisplayMidPointValueInParameterDomain)
      {
      double p1, p2;
      this->GetFunctionPointParameter(id, &p1);
      this->GetFunctionPointParameter(id + 1, &p2);
      this->MapParameterToDisplayedParameter(p1, &p1);
      this->MapParameterToDisplayedParameter(p2, &p2);
      this->SetFunctionPointMidPoint(
        id, (value - p1) / (p2 - p1));
      }
    else
      {
      this->SetFunctionPointMidPoint(id, value);
      }
    if (this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(id);
      this->InvokePointChangedCommand(id);
      this->InvokeFunctionChangedCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::MidPointEntryChangingCallback(
  double value)
{
  if (this->HasMidPointSelection())
    {
    int id = this->GetSelectedMidPoint();
    unsigned long mtime = this->GetFunctionMTime();
    if (this->DisplayMidPointValueInParameterDomain)
      {
      double p1, p2;
      this->GetFunctionPointParameter(id, &p1);
      this->GetFunctionPointParameter(id + 1, &p2);
      this->MapParameterToDisplayedParameter(p1, &p1);
      this->MapParameterToDisplayedParameter(p2, &p2);
      this->SetFunctionPointMidPoint(
        id, (value - p1) / (p2 - p1));
      }
    else
      {
      this->SetFunctionPointMidPoint(id, value);
      }
    if (this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(id);
      this->InvokePointChangingCommand(id);
      this->InvokeFunctionChangingCommand();
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::FunctionPointSharpnessIsLocked(
  int vtkNotUsed(id))
{
  return 0;
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
    this->CreateSharpnessEntry();
    }

  this->UpdateSharpnessEntry(this->GetSelectedMidPoint());

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
      !this->GetFunctionPointSharpness(id, &sharpness))
      
    { 
    this->SharpnessEntry->SetEnabled(0);
    return;
    }

  this->SharpnessEntry->SetEnabled(
    this->FunctionPointSharpnessIsLocked(id) ? 0 : this->GetEnabled());

  this->SharpnessEntry->SetValue(sharpness);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SharpnessEntryChangedCallback(
  double value)
{
  if (this->HasMidPointSelection())
    {
    unsigned long mtime = this->GetFunctionMTime();
    if (this->SetFunctionPointSharpness(
          this->GetSelectedMidPoint(), value) &&
        this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(this->GetSelectedMidPoint());
      this->InvokePointChangedCommand(this->GetSelectedMidPoint());
      this->InvokeFunctionChangedCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SharpnessEntryChangingCallback(
  double value)
{
  if (this->HasMidPointSelection())
    {
    unsigned long mtime = this->GetFunctionMTime();
    if (this->SetFunctionPointSharpness(
          this->GetSelectedMidPoint(), value) &&
        this->GetFunctionMTime() > mtime)
      {
      this->RedrawSinglePointDependentElements(this->GetSelectedMidPoint());
      this->InvokePointChangingCommand(this->GetSelectedMidPoint());
      this->InvokeFunctionChangingCommand();
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::MergePointFromEditor(
  vtkKWParameterValueFunctionEditor *editor, int editor_id, int *new_id)
{
  int added = 
    this->Superclass::MergePointFromEditor(editor, editor_id, new_id);

  vtkKWParameterValueHermiteFunctionEditor *h_editor = 
    vtkKWParameterValueHermiteFunctionEditor::SafeDownCast(editor);
  if (h_editor && added)
    {
    double midpoint, editor_midpoint, sharpness, editor_sharpness;

    h_editor->GetFunctionPointMidPoint(editor_id, &editor_midpoint);
    h_editor->GetFunctionPointSharpness(editor_id, &editor_sharpness);

    this->GetFunctionPointMidPoint(*new_id, &midpoint);
    this->GetFunctionPointSharpness(*new_id, &sharpness);
    
    if (midpoint != editor_midpoint || sharpness != editor_sharpness)
      {
      this->SetFunctionPointMidPoint(*new_id, editor_midpoint);
      this->SetFunctionPointSharpness(*new_id, editor_sharpness);
      this->RedrawSinglePointDependentElements(*new_id);
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

    h_editor->GetFunctionPointMidPoint(id, &editor_midpoint);
    h_editor->GetFunctionPointSharpness(id, &editor_sharpness);

    this->GetFunctionPointMidPoint(id, &midpoint);
    this->GetFunctionPointSharpness(id, &sharpness);
    
    if (midpoint != editor_midpoint || sharpness != editor_sharpness)
      {
      this->SetFunctionPointMidPoint(id, editor_midpoint);
      this->SetFunctionPointSharpness(id, editor_sharpness);
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

  if (this->MidPointGuidelineValueVisibility && this->IsCreated())
    {
    this->CreateGuidelineValueCanvas();
    }

  this->Redraw();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SetMidPointGuidelineValueFormat(const char *arg)
{
  if (this->MidPointGuidelineValueFormat == NULL && arg == NULL) 
    { 
    return;
    }

  if (this->MidPointGuidelineValueFormat && arg && 
      (!strcmp(this->MidPointGuidelineValueFormat, arg))) 
    {
    return;
    }

  if (this->MidPointGuidelineValueFormat) 
    { 
    delete [] this->MidPointGuidelineValueFormat; 
    }

  if (arg)
    {
    this->MidPointGuidelineValueFormat = new char[strlen(arg) + 1];
    strcpy(this->MidPointGuidelineValueFormat, arg);
    }
  else
    {
    this->MidPointGuidelineValueFormat = NULL;
    }

  this->Modified();
  
  if (this->MidPointGuidelineValueVisibility)
    {
    this->RedrawFunction();
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::IsGuidelineValueCanvasUsed()
{
  return this->Superclass::IsGuidelineValueCanvasUsed() || 
    this->MidPointGuidelineValueVisibility;
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
void vtkKWParameterValueHermiteFunctionEditor::SetSelectedMidPointColor(
  double r, double g, double b)
{
  if ((r == this->SelectedMidPointColor[0] &&
       g == this->SelectedMidPointColor[1] &&
       b == this->SelectedMidPointColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->SelectedMidPointColor[0] = r;
  this->SelectedMidPointColor[1] = g;
  this->SelectedMidPointColor[2] = b;

  this->Modified();

  this->RedrawSinglePointDependentElements(this->GetSelectedMidPoint());
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::GetMidPointCanvasCoordinates(
  int id, int *x, int *y, double *p)
{
  double midpoint;
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize() - 1 ||
      !this->GetFunctionPointMidPoint(id, &midpoint))
    {
    return 0;
    }

  double p1, p2;
  this->GetFunctionPointParameter(id, &p1);
  this->GetFunctionPointParameter(id + 1, &p2);

  *p = p1 + (p2 - p1) * midpoint;

  return this->GetFunctionPointCanvasCoordinatesAtParameter(*p, x, y);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::FindMidPointAtCanvasCoordinates(
  int x, int y, int *id, int *c_x, int *c_y)
{
  if (!this->IsCreated() || !this->HasFunction())
    {
    return 0;
    }

  char item[256];
  if (!this->FindClosestItemWithTagAtCanvasCoordinates(
        x, y, 3, vtkKWParameterValueHermiteFunctionEditor::MidPointTag, 
        c_x, c_y, item))
    {
    return 0;
    }

  *id = -1;

  // Get its first tag, which should be a midpoint tag (in
  // the form of m_pid, ex: m_p0)

  const char *canv = this->Canvas->GetWidgetName();
  const char *tag = 
    this->Script("lindex [%s itemcget %s -tags] 0", canv, item);
  if (tag && strlen(tag) > 3 && !strncmp(tag, "m_p", 3) && isdigit(tag[3]))
    {
    *id = atoi(tag + 3);
    }

  return (*id < 0 || *id >= this->GetFunctionSize() - 1) ? 0 : 1;
}

//----------------------------------------------------------------------------
void 
vtkKWParameterValueHermiteFunctionEditor::RedrawFunctionDependentElements()
{
  this->Superclass::RedrawFunctionDependentElements();

  this->UpdateMidPointEntries(this->GetSelectedMidPoint());
}

//----------------------------------------------------------------------------
void 
vtkKWParameterValueHermiteFunctionEditor::RedrawSinglePointDependentElements(
  int id)
{
  this->Superclass::RedrawSinglePointDependentElements(id);

  if (id < 0 || id >= this->GetFunctionSize())
    {
    return;
    }

  if (id == this->GetSelectedMidPoint())
    {
    this->UpdateMidPointEntries(id);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::RedrawFunction()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  // Are we going to create or delete points ?

  int c_nb_points = 
    this->CanvasHasTag(vtkKWParameterValueFunctionEditor::PointTag);
  int nb_points_changed = (c_nb_points != this->GetFunctionSize());

  // Try to save the midpoint selection before (eventually) creating new points

  int s_x = 0, s_y = 0;
  if (nb_points_changed && this->HasMidPointSelection())
    {
    int item_id = atoi(
      this->Script(
        "lindex [%s find withtag %s] 0",
        this->Canvas->GetWidgetName(), 
        vtkKWParameterValueHermiteFunctionEditor::MidPointSelectedTag));
    this->GetCanvasItemCenter(item_id, &s_x, &s_y);
    }

  // Draw the function

  this->Superclass::RedrawFunction();

  // Try to restore the midpoint selection

  if (nb_points_changed && this->HasMidPointSelection())
    {
    int i, p_x, p_y, nb_points = this->GetFunctionSize();
    double p;
    for (i = 0; i < nb_points - 1; i++)
      {
      if (this->GetMidPointCanvasCoordinates(i, &p_x, &p_y, &p) &&
          p_x == s_x && p_y == s_y)
        {
        this->SelectMidPoint(i);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::RedrawLine(
  int id1, int id2, ostrstream *tk_cmd)
{
  // Redraw the line

  this->Superclass::RedrawLine(id1, id2, tk_cmd);

  // Then redraw the midpoint on this line

  if (!this->IsCreated() || !this->HasFunction() || this->DisableRedraw)
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

  double p, displayed_p;
  int x, y, rx = 0, ry = 0;
  int is_not_visible = 0, is_not_visible_h = 0;
  int is_not_valid = (id1 < 0 || id1 >= (this->GetFunctionSize() - 1));

  // Do we have a midpoint for this segment ?

  double midpoint;
  if (!this->GetFunctionPointMidPoint(id1, &midpoint))
    {
    is_not_valid = 1;
    }

  // Get the midpoint coords, radius, check if the midpoint is visible
  // The radius is 80% of the point radius

  if (!is_not_valid)
    {
    this->GetMidPointCanvasCoordinates(id1, &x, &y, &p);

    rx = (int)((double)this->PointRadiusX * VTK_KW_PVHFE_POINT_RADIUS_FACTOR);
    if (id1 == this->GetSelectedMidPoint())
      {
      rx = (int)ceil((double)rx * this->SelectedPointRadius);
      }

    ry = (int)((double)this->PointRadiusY * VTK_KW_PVHFE_POINT_RADIUS_FACTOR);
    if (id1 == this->GetSelectedMidPoint())
      {
      ry = (int)ceil((double)ry * this->SelectedPointRadius);
      }

    // If the midpoint is not in the visible range, hide it

    double c_x, c_y, c_x2, c_y2;
    this->GetCanvasScrollRegion(&c_x, &c_y, &c_x2, &c_y2);

    int visible_marginx = rx + this->PointOutlineWidth + 5;
    int visible_marginy = ry + this->PointOutlineWidth + 5;

    if (x + visible_marginx < c_x || c_x2 < x - visible_marginx)
      {
      is_not_visible_h = 1;
      }
    
    if (is_not_visible_h || 
        y + visible_marginy < c_y || c_y2 < y - visible_marginy)
      {
      is_not_visible = 1;
      }
    }

  // Create/update the midpoint

  int midpoint_exists = this->CanvasHasTag("m_p", &id1);
  if (is_not_valid)
    {
    if (midpoint_exists)
      {
      *tk_cmd << canv << " delete m_p" << id1 << endl;
      }
    }
  else
    {
    if (is_not_visible || 
        !this->GetMidPointVisibility() || 
        !this->CanvasVisibility)
      {
      if (midpoint_exists)
        {
        *tk_cmd << canv << " itemconfigure m_p" << id1 << " -state hidden\n";
        }
      }
    else
      {
      if (!midpoint_exists)
        {
        *tk_cmd << canv << " create rectangle" << " 0 0 0 0 -tags {m_p" << id1 
                << " "<< vtkKWParameterValueHermiteFunctionEditor::MidPointTag 
                << " " << vtkKWParameterValueFunctionEditor::FunctionTag 
                << "}" << endl;
        }
      if (this->GetSelectedMidPoint() == id1)
        {
        *tk_cmd << canv << " raise m_p" << id1 << " " 
                << vtkKWParameterValueFunctionEditor::FunctionTag << endl;
        }
      else
        {
        *tk_cmd << canv << " lower m_p" << id1 << " {p" << id1 << "||p" << id2
                << "}" << endl;
        }
      *tk_cmd << canv << " coords m_p" << id1 
              << " " << x - rx << " " << y - ry 
              << " " << x + rx << " " << y + ry
              << endl;
      char color[10];
      double *rgb = (id1 == this->GetSelectedMidPoint()) 
        ? this->SelectedMidPointColor : this->MidPointColor;
      sprintf(color, "#%02x%02x%02x", 
              (int)(rgb[0]*255.0), (int)(rgb[1]*255.0), (int)(rgb[2]*255.0));
      *tk_cmd << canv << " itemconfigure m_p" << id1
              << " -state normal  -width " << this->PointOutlineWidth
              << " -outline black -fill " << color << endl;
      }
    }

  // Create/update the midpoint guideline

  int midpoint_guide_exists = this->CanvasHasTag("m_g", &id1);
  if (is_not_valid)
    {
    if (midpoint_guide_exists)
      {
      *tk_cmd << canv << " delete m_g" << id1 << endl;
      }
    }
  else
    {
    if (is_not_visible_h || 
        !this->MidPointGuidelineVisibility || 
        !this->CanvasVisibility)
      {
      if (midpoint_guide_exists)
        {
        *tk_cmd << canv << " itemconfigure m_g" << id1 << " -state hidden\n";
        }
      }
    else
      {
      if (!midpoint_guide_exists)
        {
        *tk_cmd 
          << canv << " create line 0 0 0 0 -fill black -width 1 " 
          << " -tags {m_g" << id1 << " " 
          << vtkKWParameterValueHermiteFunctionEditor::MidPointGuidelineTag 
          << " " << vtkKWParameterValueFunctionEditor::FunctionTag
          << "}" << endl;
        *tk_cmd << canv << " lower m_g" << id1 << " m_p" << id1 << endl;
        }
  
      double factors[2] = {0.0, 0.0};
      this->GetCanvasScalingFactors(factors);
      double *v_w_range = this->GetWholeValueRange();
      int y1 = vtkMath::Round(v_w_range[0] * factors[1]);
      int y2 = vtkMath::Round(v_w_range[1] * factors[1]);
      *tk_cmd << canv << " coords m_g" << id1 << " "
              << x << " " << y1 << " " << x << " " << y2 << endl;
      *tk_cmd << canv << " itemconfigure m_g" << id1;
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

  // Create/update the midpoint guideline value

  if (this->IsGuidelineValueCanvasUsed() && 
      this->GuidelineValueCanvas &&
      this->GuidelineValueCanvas->IsCreated())
    {
    const char *gv_canv = this->GuidelineValueCanvas->GetWidgetName();
  
    int midpoint_guidevalue_exists = 
      this->CanvasHasTag("m_g", &id1, this->GuidelineValueCanvas);
    if (is_not_valid)
      {
      if (midpoint_guidevalue_exists)
        {
        *tk_cmd << gv_canv << " delete m_g" << id1 << endl;
        }
      }
    else
      {
      if (is_not_visible_h || 
          !this->MidPointGuidelineVisibility || 
          !this->MidPointGuidelineValueVisibility)
        {
        if (midpoint_guidevalue_exists)
          {
          *tk_cmd << gv_canv << " itemconfigure m_g"<<id1<<" -state hidden\n";
          }
        }
      else
        {
        if (!midpoint_guidevalue_exists)
          {
          *tk_cmd 
            << gv_canv << " create text 0 0 -text {} -anchor s " 
            << "-font {{fixed} " << VTK_KW_PVHFE_GUIDELINE_VALUE_TEXT_SIZE 
            << "} -tags {m_g" << id1 << " " 
            << vtkKWParameterValueHermiteFunctionEditor::MidPointGuidelineTag 
            << " " << vtkKWParameterValueFunctionEditor::FunctionTag
            << "}" << endl;
          }
        
        *tk_cmd << gv_canv << " coords m_g" << id1 << " " << x 
                << " " << this->GuidelineValueCanvas->GetHeight() + 1 << endl
                << gv_canv << " itemconfigure m_g" << id1 << " -state normal" 
                << endl;
        if (this->MidPointGuidelineValueFormat)
          {
          this->MapParameterToDisplayedParameter(p, &displayed_p);
          char buffer[256];
          sprintf(buffer, this->MidPointGuidelineValueFormat, displayed_p);
          *tk_cmd << gv_canv << " itemconfigure m_g" << id1
                  << " -text {" << buffer << "}" << endl;
          }
        }
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
void vtkKWParameterValueHermiteFunctionEditor::SelectPoint(int id)
{
  this->Superclass::SelectPoint(id);

  // Deselect any midpoint selection, (we want only one type of
  // selection at a time)

  if (this->HasSelection())
    {
    this->ClearMidPointSelection();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SelectNextPoint()
{
  if (this->HasMidPointSelection())
    {
    this->SelectPoint(this->GetSelectedMidPoint() + 1);
    }
  else if (this->HasSelection())
    {
    if (this->GetSelectedPoint() == this->GetFunctionSize() - 1)
      {
      this->SelectPoint(0);
      }
    else
      {
      double pos;
      if (this->GetMidPointVisibility() &&
          this->GetFunctionPointMidPoint(this->GetSelectedPoint(), &pos))
        {
        this->SelectMidPoint(this->GetSelectedPoint());
        }
      else
        {
        this->Superclass::SelectNextPoint();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SelectPreviousPoint()
{
  if (this->HasMidPointSelection())
    {
    this->SelectPoint(this->GetSelectedMidPoint());
    }
  else if (this->HasSelection())
    {
    if (this->GetSelectedPoint() == 0)
      {
      this->SelectPoint(this->GetFunctionSize() - 1);
      }
    else
      {
      double pos;
      if (this->GetMidPointVisibility() &&
          this->GetFunctionPointMidPoint(this->GetSelectedPoint() - 1, &pos))
        {
        this->SelectMidPoint(this->GetSelectedPoint() - 1);
        }
      else
        {
        this->Superclass::SelectPreviousPoint();
        }
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::HasMidPointSelection()
{
  return (this->GetSelectedMidPoint() >= 0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::SelectMidPoint(int id)
{
  if (!this->HasFunction() || 
      id < 0 || id >= this->GetFunctionSize() - 1 ||
      this->GetSelectedMidPoint() == id)
    {
    return;
    }

  // First deselect any selection, i.e. both the current point selection
  // *and* the midpoint selection (we want only one type at a time)

  this->ClearSelection();
  this->ClearMidPointSelection();

  // Now selects

  this->SelectedMidPoint = id;

  // Add the selection tag to the midpoint

  if (this->IsCreated())
    {
    this->Script("%s addtag %s withtag m_p%d", 
                 this->Canvas->GetWidgetName(),
                 vtkKWParameterValueHermiteFunctionEditor::MidPointSelectedTag,
                 this->GetSelectedMidPoint());
    }

  // Draw the selected midpoint accordingly and update its aspect
  
  this->RedrawSinglePointDependentElements(this->GetSelectedMidPoint());
  this->PackPointEntries();

  this->InvokeMidPointSelectionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::ClearMidPointSelection()
{
  if (!this->HasMidPointSelection())
    {
    return;
    }

  // Remove the selection tag from the selected midpoint

  if (this->IsCreated())
    {
    this->Script("%s dtag m_p%d %s", 
                 this->Canvas->GetWidgetName(),
                 this->GetSelectedMidPoint(),
                 vtkKWParameterValueHermiteFunctionEditor::MidPointSelectedTag);
    }

  // Deselect

  int old_selection = this->GetSelectedMidPoint();
  this->SelectedMidPoint = -1;

  // Redraw the midpoint that used to be selected and update its aspect

  this->RedrawSinglePointDependentElements(old_selection);

  // Show the selected midpoint description in the point label
  // Since nothing is selected, the expected side effect is to clear the
  // point label

  this->UpdateMidPointEntries(this->GetSelectedMidPoint());
  this->PackPointEntries();

  this->InvokeMidPointSelectionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::InvokeMidPointSelectionChangedCommand()
{
  this->InvokeObjectMethodCommand(this->MidPointSelectionChangedCommand);

  this->InvokeEvent(
    vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent);
}

//----------------------------------------------------------------------------
void 
vtkKWParameterValueHermiteFunctionEditor::SetMidPointSelectionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->MidPointSelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::SynchronizeSingleSelection(
  vtkKWParameterValueFunctionEditor *pvfe_b)
{
  this->Superclass::SynchronizeSingleSelection(pvfe_b);

  vtkKWParameterValueHermiteFunctionEditor *b =
    reinterpret_cast<vtkKWParameterValueHermiteFunctionEditor *>(pvfe_b);

  if (!b)
    {
    return 0;
    }
  
  // Make sure only one of those editors has a selected midpoint from now
  
  if (this->HasMidPointSelection())
    {
    b->ClearMidPointSelection();
    }
  else if (b->HasMidPointSelection())
    {
    this->ClearMidPointSelection();
    }
  
  int events[] = 
    {
      vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent
    };
  
  b->AddObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::DoNotSynchronizeSingleSelection(
  vtkKWParameterValueFunctionEditor *pvfe_b)
{
  this->Superclass::DoNotSynchronizeSingleSelection(pvfe_b);

  vtkKWParameterValueHermiteFunctionEditor *b =
    reinterpret_cast<vtkKWParameterValueHermiteFunctionEditor *>(pvfe_b);

  if (!b)
    {
    return 0;
    }
  
  int events[] = 
    {
      vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent
    };
  
  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::SynchronizeSameSelection(
  vtkKWParameterValueFunctionEditor *pvfe_b)
{
  this->Superclass::SynchronizeSameSelection(pvfe_b);

  vtkKWParameterValueHermiteFunctionEditor *b =
    reinterpret_cast<vtkKWParameterValueHermiteFunctionEditor *>(pvfe_b);

  if (!b)
    {
    return 0;
    }
  
  // Make sure those editors have the same selected midpoint from now
  
  if (this->HasMidPointSelection())
    {
    b->SelectMidPoint(this->GetSelectedMidPoint());
    }
  else if (b->HasMidPointSelection())
    {
    this->SelectMidPoint(b->GetSelectedMidPoint());
    }
  
  int events[] = 
    {
      vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent
    };
  
  b->AddObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand2);

  this->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand2);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueHermiteFunctionEditor::DoNotSynchronizeSameSelection(
  vtkKWParameterValueFunctionEditor *pvfe_b)
{
  this->Superclass::DoNotSynchronizeSameSelection(pvfe_b);

  vtkKWParameterValueHermiteFunctionEditor *b =
    reinterpret_cast<vtkKWParameterValueHermiteFunctionEditor *>(pvfe_b);

  if (!b)
    {
    return 0;
    }
  
  int events[] = 
    {
      vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent
    };
  
  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand2);
  
  this->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand2);
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::ProcessSynchronizationEvents(
  vtkObject *caller,
  unsigned long event,
  void *calldata)
{
  this->Superclass::ProcessSynchronizationEvents(caller, event, calldata);

  vtkKWParameterValueHermiteFunctionEditor *pvfe =
    reinterpret_cast<vtkKWParameterValueHermiteFunctionEditor *>(caller);
  
  switch (event)
    {
    // Synchronize single midpoint selection

    case vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent:
      if (pvfe->HasMidPointSelection())
        {
        this->ClearMidPointSelection();
        this->ClearSelection();
        }
      break;

    // Synchronize Single selection
      
    case vtkKWParameterValueFunctionEditor::SelectionChangedEvent:
      if (pvfe->HasSelection())
        {
        this->ClearMidPointSelection();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::ProcessSynchronizationEvents2(
  vtkObject *caller,
  unsigned long event,
  void *calldata)
{
  this->Superclass::ProcessSynchronizationEvents2(caller, event, calldata);
  
  vtkKWParameterValueHermiteFunctionEditor *pvfe =
    reinterpret_cast<vtkKWParameterValueHermiteFunctionEditor *>(caller);
  
  switch (event)
    {
    // Synchronize same midpoint selection

    case vtkKWParameterValueHermiteFunctionEditor::MidPointSelectionChangedEvent:
      if (pvfe->HasMidPointSelection())
        {
        this->SelectMidPoint(pvfe->GetSelectedMidPoint());
        }
      else
        {
        this->ClearMidPointSelection();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::Bind()
{
  this->Superclass::Bind();

  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueHermiteFunctionEditor::MidPointTag
           << " <B1-Motion> {" << this->GetTclName() 
           << " MoveMidPointCallback %%x %%y 1}" << endl;
     
    tk_cmd << canv << " bind " 
           << vtkKWParameterValueHermiteFunctionEditor::MidPointTag
           << " <B3-Motion> {" << this->GetTclName() 
           << " MoveMidPointCallback %%x %%y 3}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueHermiteFunctionEditor::MidPointTag 
           << " <Any-ButtonRelease> {" << this->GetTclName() 
           << " EndMidPointInteractionCallback %%x %%y}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::UnBind()
{
  this->Superclass::UnBind();

  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueHermiteFunctionEditor::MidPointTag 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueHermiteFunctionEditor::MidPointTag 
           << " <ButtonRelease-1> {}" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void 
vtkKWParameterValueHermiteFunctionEditor::StartInteractionCallback(
  int x, int y)
{
  int id, c_x, c_y, p_id, p_c_x, p_c_y;

  // If the mid-point is stuck below the point itself, let's try to detect
  // when the user tries to select twice on something already selected, and
  // pick the one below

  int found_mid_point = 
    this->FindMidPointAtCanvasCoordinates(x, y, &id, &c_x, &c_y);
  int found_point = 
    this->FindFunctionPointAtCanvasCoordinates(x, y, &p_id, &p_c_x, &p_c_y);

  if (found_mid_point && found_point)
    {
    if (this->GetSelectedMidPoint() == id)
      {
      if (this->LastMidPointSelectionCanvasCoordinateX == x &&
          this->LastMidPointSelectionCanvasCoordinateY == y &&
          (!(this->LastSelectionCanvasCoordinateX == p_c_x &&
             this->LastSelectionCanvasCoordinateY == p_c_y)))
        {
        found_mid_point = 0;
        }
      else
        {
        found_point = 0;
        }
      }
    if (this->GetSelectedPoint() == p_id)
      {
      if (this->LastSelectionCanvasCoordinateX == p_c_x &&
          this->LastSelectionCanvasCoordinateY == p_c_y &&
          !((this->LastMidPointSelectionCanvasCoordinateX == x &&
             this->LastMidPointSelectionCanvasCoordinateY == y)))
        {
        found_point = 0;
        }
      else
        {
        found_mid_point = 0;
        }
      }
    }

  if (!found_mid_point || found_point)
    {
    this->Superclass::StartInteractionCallback(x, y);
    return;
    }

  // Select the midpoint

  this->SelectMidPoint(id);
  this->LastMidPointSelectionCanvasCoordinateX = x;
  this->LastMidPointSelectionCanvasCoordinateY = y;
  this->GetFunctionPointSharpness(
    this->GetSelectedMidPoint(),
    &this->LastMidPointSelectionSharpness);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::MoveMidPointCallback(
  int x, int y, int button)
{
  if (!this->IsCreated() || !this->HasMidPointSelection())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If we are out of the canvas, clamp the coordinates

  if (x < 0)
    {
    x = 0;
    }
  else if (x > this->CurrentCanvasWidth - 1)
    {
    x = this->CurrentCanvasWidth - 1;
    }

  // Get the real canvas coordinates

  int c_x = atoi(this->Script("%s canvasx %d", canv, x));

  // We assume we can not go before or beyond the points 'id' and 'id + 1'
  // (i.e. the two end-points between which the midpoint is defined)

  int prev_x, prev_y;
  this->GetFunctionPointCanvasCoordinates(
    this->GetSelectedMidPoint(), &prev_x, &prev_y);
  if (c_x < prev_x)
    {
    c_x = prev_x;
    }

  int next_x, next_y;
  this->GetFunctionPointCanvasCoordinates(
    this->GetSelectedMidPoint() + 1, &next_x, &next_y);
  if (c_x > next_x)
    {
    c_x = next_x;
    }

  unsigned long mtime = this->GetFunctionMTime();

  const char *cursor = NULL;
  if (button == 1)
    {
    cursor = "sb_h_double_arrow";
    this->SetFunctionPointMidPoint(
      this->GetSelectedMidPoint(), 
      (double)(c_x - prev_x) / (double)(next_x - prev_x));
    }
  else if (button == 3)
    {
    cursor = "sb_v_double_arrow";
    double sharpness = this->LastMidPointSelectionSharpness - 
      ((double)(y - this->LastMidPointSelectionCanvasCoordinateY) /
       (double)this->CurrentCanvasHeight) * 2.0;
    if (sharpness < 0.0)
      {
      sharpness = 0.0;
      }
    else if (sharpness > 1.0)
      {
      sharpness = 1.0;
      }
    this->SetFunctionPointSharpness(this->GetSelectedMidPoint(), sharpness);
    }

  // Update cursor to show which interaction is going on

  if (this->ChangeMouseCursor)
    {
    this->Canvas->SetConfigurationOption("-cursor", cursor);
    }

  // Invoke the commands/callbacks

  if (this->GetFunctionMTime() > mtime)
    {
    this->RedrawSinglePointDependentElements(this->GetSelectedMidPoint());
    this->InvokePointChangingCommand(this->GetSelectedMidPoint());
    this->InvokeFunctionChangingCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::EndMidPointInteractionCallback(
  int vtkNotUsed(x), int vtkNotUsed(y))
{
  if (!this->HasMidPointSelection())
    {
    return;
    }

  // Invoke the commands/callbacks

  this->InvokePointChangedCommand(this->GetSelectedMidPoint());
  this->InvokeFunctionChangedCommand();

  // Remove any interaction icon

  if (this->Canvas && this->ChangeMouseCursor)
    {
    this->Canvas->SetConfigurationOption("-cursor", NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::DisplayHistogramOnly()
{
  this->Superclass::DisplayHistogramOnly();

  this->MidPointEntryVisibilityOff();
  this->SharpnessEntryVisibilityOff();
  this->MidPointVisibilityOff();
  this->MidPointGuidelineVisibilityOff();
  this->MidPointGuidelineValueVisibilityOff();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueHermiteFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "SharpnessEntryVisibility: "
     << (this->SharpnessEntryVisibility ? "On" : "Off") << endl;

  os << indent << "MidPointEntryVisibility: "
     << (this->MidPointEntryVisibility ? "On" : "Off") << endl;

  os << indent << "DisplayMidPointValueInParameterDomain: "
     << (this->DisplayMidPointValueInParameterDomain ? "On" : "Off") << endl;

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

  os << indent << "SelectedMidPoint: "<< this->GetSelectedMidPoint() << endl;

  os << indent << "MidPointGuidelineValueFormat: "
     << (this->MidPointGuidelineValueFormat ? this->MidPointGuidelineValueFormat : "(None)") << endl;

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

