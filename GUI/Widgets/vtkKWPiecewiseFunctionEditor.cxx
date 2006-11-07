/*=========================================================================

  Module:    vtkKWPiecewiseFunctionEditor.cxx,v

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWPiecewiseFunctionEditor.h"

#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWRange.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"

#include <vtksys/stl/string>

vtkStandardNewMacro(vtkKWPiecewiseFunctionEditor);
vtkCxxRevisionMacro(vtkKWPiecewiseFunctionEditor, "1.51");

#define EPSILON 0.0001
#define EPSILON_MIN_WINDOW (EPSILON * 2.5)

//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::vtkKWPiecewiseFunctionEditor()
{
  this->PiecewiseFunction                = NULL;
  this->PointColorTransferFunction       = NULL;

  this->WindowLevelMode                  = 0;
  this->WindowLevelModeLockEndPointValue = 0;
  this->WindowLevelModeButtonVisibility  = 0;
  this->ValueEntryVisibility             = 1;

  this->Window                           = 1.0;
  this->Level                            = 1.0;

  this->WindowLevelModeChangedCommand    = NULL;

  this->ValueEntry                       = vtkKWEntryWithLabel::New();

  this->WindowLevelModeCheckButton       = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::~vtkKWPiecewiseFunctionEditor()
{
  if (this->WindowLevelModeChangedCommand)
    {
    delete [] this->WindowLevelModeChangedCommand;
    this->WindowLevelModeChangedCommand = NULL;
    }

  if (this->ValueEntry)
    {
    this->ValueEntry->Delete();
    this->ValueEntry = NULL;
    }

  if (this->WindowLevelModeCheckButton)
    {
    this->WindowLevelModeCheckButton->Delete();
    this->WindowLevelModeCheckButton = NULL;
    }

  this->SetPiecewiseFunction(NULL);
  this->SetPointColorTransferFunction(NULL);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetPiecewiseFunction(
  vtkPiecewiseFunction *arg)
{
  if (this->PiecewiseFunction == arg)
    {
    return;
    }

  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->UnRegister(this);
    }
    
  this->PiecewiseFunction = arg;

  if (this->PiecewiseFunction)
    {
    this->PiecewiseFunction->Register(this);

    // Reset the whole parameter range to the function range.
    // This is done to avoid extreme case where the current parameter range
    // would be several order of magnitudes smaller than the function range:
    // the next Update() would redraw the function at a very high zoom level
    // which could produce a very unreasonable number of segments if the
    // function was to be sampled at regular pixels interval.

    this->SetWholeParameterRangeToFunctionRange();
    }

  this->Modified();

  this->LastRedrawFunctionTime = 0;

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetPointColorTransferFunction(
  vtkColorTransferFunction *arg)
{
  if (this->PointColorTransferFunction == arg)
    {
    return;
    }

  if (this->PointColorTransferFunction)
    {
    this->PointColorTransferFunction->UnRegister(this);
    }
    
  this->PointColorTransferFunction = arg;

  if (this->PointColorTransferFunction)
    {
    this->PointColorTransferFunction->Register(this);
    }

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointCanBeAdded()
{
  return (this->Superclass::FunctionPointCanBeAdded() &&
          !this->WindowLevelMode);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointCanBeRemoved(int id)
{
  return (this->Superclass::FunctionPointCanBeRemoved(id) &&
          !this->WindowLevelMode);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointParameterIsLocked(int id)
{
  return (this->Superclass::FunctionPointParameterIsLocked(id) ||
          (this->HasFunction() &&
           this->WindowLevelMode &&
           (id == 0 || 
            (this->GetFunctionSize() && id == this->GetFunctionSize() - 1))));
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointValueIsLocked(int id)
{
  return 
    (this->Superclass::FunctionPointValueIsLocked(id) ||
     (this->HasFunction() &&
      this->WindowLevelMode && 
      this->WindowLevelModeLockEndPointValue &&
      ((this->GetFunctionSize() > 0 && 
        (id == 0 || id == this->GetFunctionSize() - 1)) ||
       (this->GetFunctionSize() > 1 && 
        (id == 1 || id == this->GetFunctionSize() - 2))
        )));
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointMidPointIsLocked(int id)
{
  return (this->Superclass::FunctionPointMidPointIsLocked(id) ||
          this->WindowLevelMode);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointSharpnessIsLocked(
  int id)
{
  return (this->Superclass::FunctionPointSharpnessIsLocked(id) ||
          this->WindowLevelMode);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::HasFunction()
{
  return this->PiecewiseFunction ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionSize()
{
  return this->PiecewiseFunction ? this->PiecewiseFunction->GetSize() : 0;
}

//----------------------------------------------------------------------------
unsigned long vtkKWPiecewiseFunctionEditor::GetFunctionMTime()
{
  return this->PiecewiseFunction ? this->PiecewiseFunction->GetMTime() : 0;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointParameter(
  int id, double *parameter)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() || 
      !parameter)
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  *parameter = node_value[0];
#else
  *parameter = this->PiecewiseFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
#endif
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointDimensionality()
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointValues(
  int id, double *values)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      !values)
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  values[0] = node_value[1];
#else
  values[0] = this->PiecewiseFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality()) + 1];
#endif
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::SetFunctionPointValues(
  int id, const double *values)
{
  double parameter;
  if (!values || !this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }

  // Clamp

  double value = 0.0;
  vtkMath::ClampValue(values[0], this->GetWholeValueRange(), &value);

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  if (node_value[1] != value)
    {
    this->PiecewiseFunction->AddPoint(
      parameter, value, node_value[2], node_value[3]);
    }
#else
  this->PiecewiseFunction->AddPoint(parameter, value);
#endif
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::InterpolateFunctionPointValues(
  double parameter, double *values)
{
  if (!this->HasFunction() || !values)
    {
    return 0;
    }

  values[0] = this->PiecewiseFunction->GetValue(parameter);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::AddFunctionPoint(
  double parameter, const double *values, int *id)
{
  if (!this->HasFunction() || !values || !id)
    {
    return 0;
    }

  // Clamp

  vtkMath::ClampValue(&parameter, this->GetWholeParameterRange());
  double value = 0.0;
  vtkMath::ClampValue(values[0], this->GetWholeValueRange(), &value);

  // Add the point

  int old_size = this->GetFunctionSize();
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  if (this->GetFunctionPointId(parameter, id))
    {
    double node_value[4];
    this->PiecewiseFunction->GetNodeValue(*id, node_value);
    if (node_value[1] != value)
      {
      *id = this->PiecewiseFunction->AddPoint(
        parameter, value, node_value[2], node_value[3]);
      }
    }
  else
#endif
    {
    *id = this->PiecewiseFunction->AddPoint(parameter, value);
    }
  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::SetFunctionPoint(
  int id, double parameter, const double *values)
{
  if (!this->HasFunction() || !values)
    {
    return 0;
    }

  double old_parameter;
  if (!this->GetFunctionPointParameter(id, &old_parameter))
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
#endif

  // Clamp

  vtkMath::ClampValue(&parameter, this->GetWholeParameterRange());
  double value = 0.0;
  vtkMath::ClampValue(values[0], this->GetWholeValueRange(), &value);

  if (parameter != old_parameter)
    {
    this->PiecewiseFunction->RemovePoint(old_parameter);
    }
  
  int new_id;
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  if (parameter != old_parameter || node_value[1] != value)
    {
    new_id = this->PiecewiseFunction->AddPoint(
      parameter, value, node_value[2], node_value[3]);
    }
  else
    {
    new_id = id;
    }
#else
  new_id = this->PiecewiseFunction->AddPoint(parameter, value);
#endif

  if (new_id != id)
    {
    vtkWarningMacro(<< "Setting a function point (id: " << id <<
      ") parameter/values resulted in a different point (id:" <<
      new_id << "). Inconsistent.");
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::RemoveFunctionPoint(int id)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  // Remove the point

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  double parameter = node_value[0];
#else
  double parameter = this->PiecewiseFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
#endif

  int old_size = this->GetFunctionSize();
  this->PiecewiseFunction->RemovePoint(parameter);
  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::MoveFunctionPoint(
  int id, double parameter, const double *values)
{
  int res = this->Superclass::MoveFunctionPoint(id, parameter, values);
  if (!res)
    {
    return res;
    }

  // In window-level mode, the first and second point are value-constrained
  // (so are the last and last - 1 points)

  int fsize = this->GetFunctionSize();
  if (this->WindowLevelMode && (id <= 1 || (fsize >= 2 && id >= fsize - 2)))
    {
    // Do not use 'values', as it might have been clamped or adjusted
    double current_values[
      vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
    if (!this->GetFunctionPointValues(id, current_values))
      {
      return 0;
      }
    int constrained_id;
    if (id <= 1)
      {
      constrained_id = (id == 0) ? 1 : 0;
      }
    else
      {
      constrained_id = (id == fsize - 2) ? fsize - 1 : fsize - 2;
      }
    unsigned long mtime = this->GetFunctionMTime();
    this->SetFunctionPointValues(constrained_id, current_values);
    if (this->GetFunctionMTime() > mtime)
      {
      this->RedrawFunctionDependentElements();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointColorInCanvas(
  int id, double rgb[3])
{
  double parameter;
  if (this->PointColorTransferFunction && 
      this->GetFunctionPointParameter(id, &parameter))
    {
    this->PointColorTransferFunction->GetColor(parameter, rgb);
    return 1;
    }

  return this->Superclass::GetFunctionPointColorInCanvas(id, rgb);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointMidPoint(
  int id, double *pos)
{
  if (id < 0 || id >= this->GetFunctionSize() || !pos)
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  *pos = node_value[2];
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::SetFunctionPointMidPoint(
  int id, double pos)
{
  if (id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  if (pos < 0.0)
    {
    pos = 0.0;
    }
  else if (pos > 1.0)
    {
    pos = 1.0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  if (node_value[2] != pos)
    {
    this->PiecewiseFunction->AddPoint(
      node_value[0], node_value[1], pos, node_value[3]);
    }
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointSharpness(
  int id, double *sharpness)
{
  if (id < 0 || id >= this->GetFunctionSize() || !sharpness)
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  *sharpness = node_value[3];
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::SetFunctionPointSharpness(
  int id, double sharpness)
{
  if (id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  if (sharpness < 0.0)
    {
    sharpness = 0.0;
    }
  else if (sharpness > 1.0)
    {
    sharpness = 1.0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
  if (node_value[3] != sharpness)
    {
    this->PiecewiseFunction->AddPoint(
      node_value[0], node_value[1], node_value[2], sharpness);
    }
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionLineIsSampledBetweenPoints(
  int id1, int vtkNotUsed(id2))
{
  if (!this->HasFunction() || id1 < 0 || id1 >= this->GetFunctionSize())
    {
    return 0;
    }

  // If sharpness == 0.0 and midpoint = 0.5, then it's the good 
  // old piecewise linear and we do not need to sample, the default
  // superclass implementation (staight line between id1 and id2) is fine

  double midpoint, sharpness;
  if (this->GetFunctionPointMidPoint(id1, &midpoint) &&
      this->GetFunctionPointSharpness(id1, &sharpness))
    {
    return (sharpness == 0.0 && midpoint == 0.5) ? 0 : 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::GetLineCoordinates(
  int id1, int id2, ostrstream *tk_cmd)
{
  // We want to intercept specific case like
  // sharpness = 1.0: step (3 segments), could not be done using sampling
  // sharpness = 0.0 and midpoint != 0.5: two segments, for efficiency
  //      (also mid_point should be != 0.0 or 1.0 otherwise the midpoint
  //       parameter is the same as one of the end-point, and its value
  //       (vertical position) is wrong).

  // We assume all parameters are OK, they were checked by RedrawLine

  double midpoint, sharpness, p;
  this->GetFunctionPointMidPoint(id1, &midpoint);
  this->GetFunctionPointSharpness(id1, &sharpness);

  int sharp_1 = (sharpness == 1.0) ? 1 : 0;
  int sharp_0 = (sharpness == 0.0 && 
                 midpoint != 0.5 && 
                 midpoint != 0.0 && 
                 midpoint != 1.0) ? 1 : 0;
  if (!sharp_1 && !sharp_0)
    {
    this->Superclass::GetLineCoordinates(id1, id2, tk_cmd);
    return;
    }

  // Get end-point coordinates

  int x1, y1, x2, y2, xp, yp;
  this->GetFunctionPointCanvasCoordinates(id1, &x1, &y1);
  this->GetFunctionPointCanvasCoordinates(id2, &x2, &y2);

  // Get midpoint coordinates

  this->GetMidPointCanvasCoordinates(id1, &xp, &yp, &p);

  *tk_cmd << " " << x1 << " " << y1;
  if (sharp_1)
    {
    *tk_cmd << " " << xp << " " << y1 
            << " " << xp << " " << y2;
    }
  else
    {
    *tk_cmd << " " << xp << " " << yp; 
    }
  *tk_cmd << " " << x2 << " " << y2;
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::UpdatePointEntries(int id)
{
  this->Superclass::UpdatePointEntries(id);

  if (!this->ValueEntry || !this->HasFunction())
    {
    return;
    }

  // No point ? Empty the entry and disable

  if (id < 0 || id >= this->GetFunctionSize())
    {
    this->ValueEntry->GetWidget()->SetValue("");
    this->ValueEntry->SetEnabled(0);
    return;
    }

  // Disable entry if value is locked

  this->ValueEntry->SetEnabled(
    this->FunctionPointValueIsLocked(id) ? 0 : this->GetEnabled());

  // Get the value

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
  this->PiecewiseFunction->GetNodeValue(id, node_value);
#else
  double *node_value = this->PiecewiseFunction->GetDataPointer() + id * 2;
#endif

  this->ValueEntry->GetWidget()->SetValueAsFormattedDouble(node_value[1], 3);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PiecewiseFunctionEditor already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // Create the value entry

  if (this->ValueEntryVisibility && this->PointEntriesVisibility)
    {
    this->CreateValueEntry();
    }

  // Window/Level mode

  if (this->WindowLevelModeButtonVisibility)
    {
    this->CreateWindowLevelModeCheckButton();
    }

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::CreateWindowLevelModeCheckButton()
{
  if (this->WindowLevelModeCheckButton && 
      !this->WindowLevelModeCheckButton->IsCreated())
    {
    this->CreateTopLeftFrame();
    this->WindowLevelModeCheckButton->SetParent(this->TopLeftFrame);
    this->WindowLevelModeCheckButton->Create();
    this->WindowLevelModeCheckButton->SetPadX(0);
    this->WindowLevelModeCheckButton->SetPadY(0);
    this->WindowLevelModeCheckButton->SetHighlightThickness(0);
    this->WindowLevelModeCheckButton->IndicatorVisibilityOff();
    this->WindowLevelModeCheckButton->SetBalloonHelpString(
      k_("Place the editor in window/level mode."));
    this->WindowLevelModeCheckButton->SetCommand(
      this, "WindowLevelModeCallback");
    this->WindowLevelModeCheckButton->SetImageToPredefinedIcon(
      vtkKWIcon::IconWindowLevel);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::CreateValueEntry()
{
  if (this->ValueEntry && !this->ValueEntry->IsCreated())
    {
    this->CreatePointEntriesFrame();
    this->ValueEntry->SetParent(this->PointEntriesFrame);
    this->ValueEntry->Create();
    this->ValueEntry->GetWidget()->SetWidth(6);
    this->ValueEntry->GetLabel()->SetText(
      ks_("Transfer Function Editor|Value|V:"));

    this->UpdatePointEntries(this->GetSelectedPoint());

    this->ValueEntry->GetWidget()->SetCommand(
      this, "ValueEntryCallback");
    }
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::IsTopLeftFrameUsed()
{
  return (this->Superclass::IsTopLeftFrameUsed() || 
          this->WindowLevelModeButtonVisibility);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::IsPointEntriesFrameUsed()
{
  return (this->Superclass::IsPointEntriesFrameUsed());
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Pack the whole widget

  this->Superclass::Pack();

  ostrstream tk_cmd;

  // Window/Level mode (in top left frame)

  if (this->WindowLevelModeButtonVisibility &&
      this->WindowLevelModeCheckButton && 
      this->WindowLevelModeCheckButton->IsCreated())
    {
    tk_cmd << "pack " << this->WindowLevelModeCheckButton->GetWidgetName() 
           << " -side left -fill both -padx 0" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::PackPointEntries()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Pack the other entries

  this->Superclass::PackPointEntries();

  ostrstream tk_cmd;

  // Value entry (in top right frame)

  if (this->HasSelection() &&
      this->ValueEntryVisibility  && 
      this->PointEntriesVisibility && 
      this->ValueEntry && this->ValueEntry->IsCreated())
    {
    tk_cmd << "pack " << this->ValueEntry->GetWidgetName() 
           << " -side left" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::Update()
{
  this->Superclass::Update();

  // Window/Level mode

  if (this->WindowLevelModeCheckButton)
    {
    this->WindowLevelModeCheckButton->SetSelectedState(this->WindowLevelMode);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ValueEntry);
  this->PropagateEnableState(this->WindowLevelModeCheckButton);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::InvokeFunctionChangedCommand()
{
  if (this->WindowLevelMode)
    {
    this->UpdateWindowLevelFromPoints();
    float fargs[2];
    fargs[0] = this->GetWindow();
    fargs[1] = this->GetLevel();
    this->InvokeEvent(vtkKWEvent::WindowLevelChangedEvent, fargs);
    }

  this->Superclass::InvokeFunctionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::InvokeFunctionChangingCommand()
{
  if (this->WindowLevelMode)
    {
    this->UpdateWindowLevelFromPoints();
    float fargs[2];
    fargs[0] = this->GetWindow();
    fargs[1] = this->GetLevel();
    this->InvokeEvent(vtkKWEvent::WindowLevelChangingEvent, fargs);
    }

  this->Superclass::InvokeFunctionChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetWindowLevelMode(int arg)
{
  if (this->WindowLevelMode == arg)
    {
    return;
    }

  this->WindowLevelMode = arg;
  this->Modified();

  if (this->WindowLevelMode)
    {
    // Use the whole value range

    double parameter;
    double *v_w_range = this->GetWholeValueRange();
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
    double node_value[4];
#endif

    if (this->GetFunctionSize() > 0 && 
        this->GetFunctionPointParameter(0, &parameter))
      {
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
      this->PiecewiseFunction->GetNodeValue(0, node_value);
      if (node_value[1] != v_w_range[0])
        {
        this->PiecewiseFunction->AddPoint(
          parameter, v_w_range[0], node_value[2], node_value[3]);
        }
#else
      this->PiecewiseFunction->AddPoint(parameter, v_w_range[0]);
#endif
      }
    if (this->GetFunctionSize() > 1 &&
        this->GetFunctionPointParameter(this->GetFunctionSize()-1, &parameter))
      {
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
      this->PiecewiseFunction->GetNodeValue(
        this->GetFunctionSize() - 1, node_value);
      if (node_value[1] != v_w_range[1])
        {
        this->PiecewiseFunction->AddPoint(
          parameter, v_w_range[1], node_value[2], node_value[3]);
        }
#else
      this->PiecewiseFunction->AddPoint(parameter, v_w_range[1]);
#endif
      }
    }

  this->InvokeWindowLevelModeChangedCommand(this->WindowLevelMode);

  this->UpdatePointsFromWindowLevel();
  this->Update();
  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetValueEntryVisibility(int arg)
{
  if (this->ValueEntryVisibility == arg)
    {
    return;
    }

  this->ValueEntryVisibility = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ValueEntryVisibility && 
      this->PointEntriesVisibility && 
      this->IsCreated())
    {
    this->CreateValueEntry();
    }

  this->UpdatePointEntries(this->GetSelectedPoint());

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetMidPointVisibility()
{
  return this->Superclass::GetMidPointVisibility() && !this->WindowLevelMode;
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetWindowLevelModeButtonVisibility(int arg)
{
  if (this->WindowLevelModeButtonVisibility == arg)
    {
    return;
    }

  this->WindowLevelModeButtonVisibility = arg;

  // Make sure that if the button has to be shown, we create it on the fly if
  // needed

  if (this->WindowLevelModeButtonVisibility && this->IsCreated())
    {
    this->CreateWindowLevelModeCheckButton();
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetWindowLevel(double window, double level)
{
  if (this->Window == window && this->Level == level)
    {
    return;
    }

  this->Window = window;
  this->Level = level;

  if (this->WindowLevelMode)
    {
    this->UpdatePointsFromWindowLevel();
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetInteractiveWindowLevel(
  double window, double level)
{
  if (this->Window == window && this->Level == level)
    {
    return;
    }

  this->Window = window;
  this->Level = level;

  if (this->WindowLevelMode)
    {
    this->UpdatePointsFromWindowLevel(1);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::UpdateWindowLevelFromPoints()
{
  if (this->WindowLevelMode && this->GetFunctionSize() >= 4)
    {
    double p0, p1, p2, p3;
    if (this->GetFunctionPointParameter(0, &p0) && 
        this->GetFunctionPointParameter(1, &p1) && 
        this->GetFunctionPointParameter(2, &p2) && 
        this->GetFunctionPointParameter(3, &p3))
      {
      // we had to cheat here sadly because we can't have 2 points the same
      // see UpdatePointsFromWindowLevel. Fix it for this special case.
      if (p1 <= p0 + EPSILON)
        {
        p1 = p0;
        }
      if (p2 >= p3 - EPSILON)
        {
        p2 = p3;
        }
      double v1, v2;
      v1 = this->PiecewiseFunction->GetValue(p1);
      v2 = this->PiecewiseFunction->GetValue(p2);
      this->Window = (v1 <= v2 ? (p2 - p1) : p1 - p2);
      if (fabs(this->Window) <= EPSILON_MIN_WINDOW * 1.1)
        {
        this->Window = 0;
        }
      this->Level = (p1 + p2) * 0.5;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::UpdatePointsFromWindowLevel(int interactive)
{
  if (!this->HasFunction())
    {
    return;
    }

  unsigned long mtime = this->GetFunctionMTime();

  double *p_w_range = this->GetWholeParameterRange();
  double *v_w_range = this->GetWholeValueRange();

  double parameter;
  int id;
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[4];
#endif

  // We are in not WindowLevel mode, make sure our points are within the
  // range (while in W/L mode, those points can be out of the parameter range)

  if (!this->WindowLevelMode)
    {
    int done;
    do
      {
      done = 1;
      for (id = 0; id < this->GetFunctionSize(); id++)
        {
        if (this->GetFunctionPointParameter(id, &parameter) &&
            (parameter < p_w_range[0] || parameter > p_w_range[1]))
          {
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
          this->PiecewiseFunction->GetNodeValue(id, node_value);
#endif
          double value = this->PiecewiseFunction->GetValue(parameter);
          this->PiecewiseFunction->RemovePoint(parameter);
          this->PiecewiseFunction->AddPoint(
            (parameter < p_w_range[0] ? p_w_range[0] : p_w_range[1]), value
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
            , node_value[2], node_value[3]
#endif
            );
          done = 0;
          break;
          }
        }
      } while (!done);
    }

  // We are in WindowLevel mode, make sure we have 4 points representing
  // the ramp

  else
    {
    // Get the current value bounds (default to the whole range if no points)

    double start_v, end_v;
    start_v = v_w_range[0];
    end_v = v_w_range[1];

    // Make sure that if Window < 0 the ramp is going down (if > 0, going up)

    if ((this->Window < 0 && start_v < end_v) ||
        (this->Window > 0 && start_v > end_v))
      {
      double temp = start_v;
      start_v = end_v;
      end_v = temp;
      }

    // Compute the 4 points parameters 

    double points[4];
    double window = this->Window > 0 ? this->Window : -this->Window;

    // We need to make sure window < EPSILON_MIN_WINDOW because each point
    // is around the level at window * 0.5, then to avoid point 0,1 and
    // point 2,3 to coincide we distance them by EPSILON, so we need
    // at least the window > EPSILON * 2.0 to make sure things "work".

    if (window < EPSILON_MIN_WINDOW)
      {
      window = EPSILON_MIN_WINDOW;
      }

    points[1] = (this->Level - window * 0.5);
    points[0] = (points[1] > p_w_range[0]) ? p_w_range[0] : points[1];
    if (points[1] == points[0])
      {
      // we have to cheat here sadly because we can't have 2 points the same
      points[1] += EPSILON; 
      }
    points[2] = (this->Level + window * 0.5);
    points[3] = (points[2] < p_w_range[1]) ? p_w_range[1] : points[2];
    if (points[2] == points[3])
      {
      // we have to cheat here sadly because we can't have 2 points the same
      points[2] -= EPSILON; 
      }
  
    // Remove all extra-points

    while (this->GetFunctionSize() > 4)
      {
      if (this->GetFunctionPointParameter(
            this->GetFunctionSize() - 1, &parameter))
        {
        this->PiecewiseFunction->RemovePoint(parameter);
        }
      }

    // Check if modification is needed (if any of those points is different,
    // just remove everything)

    for (id = 0; id < 4; id++)
      {
      double value = id < 2 ? start_v : end_v;
      if (!this->GetFunctionPointParameter(id, &parameter) ||
          parameter != points[id] ||
          this->PiecewiseFunction->GetValue(parameter) != value)
        {
        this->PiecewiseFunction->RemoveAllPoints();
        break;
        }
      }

    // Set the points

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
    int size = this->GetFunctionSize();
#endif
    for (id = 0; id < 4; id++)
      {
      double value = id < 2 ? start_v : end_v;
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
      if (id < size)
        {
        this->PiecewiseFunction->GetNodeValue(id, node_value);
        if (node_value[1] != value)
          {
          this->PiecewiseFunction->AddPoint(
            points[id], value, node_value[2], node_value[3]);
          }
        }
      else
#endif
        {
        this->PiecewiseFunction->AddPoint(points[id], value);
        }
      this->SetFunctionPointMidPoint(id, 0.5);
      this->SetFunctionPointSharpness(id, 0.0);
      }
    }

  // Was the function modified ?

  if (this->GetFunctionMTime() > mtime)
    {
    this->RedrawFunctionDependentElements();
    if (interactive)
      {
      this->InvokeFunctionChangingCommand();
      }
    else
      {
      this->InvokeFunctionChangedCommand();
      }
    }
}
  
//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::ValueEntryCallback(const char*)
{
  if (!this->ValueEntry || !this->HasSelection())
    {
    return;
    }

  // Get the parameter

  double parameter;
  if (!this->GetFunctionPointParameter(this->GetSelectedPoint(), &parameter))
    {
    return;
    }

  // Get the value from the entry

  double value = this->ValueEntry->GetWidget()->GetValueAsDouble();

  // Move the point, check if something has really been moved

  unsigned long mtime = this->GetFunctionMTime();

  this->MoveFunctionPoint(this->GetSelectedPoint(), parameter, &value);

  if (this->GetFunctionMTime() > mtime)
    {
    this->InvokePointChangedCommand(this->GetSelectedPoint());
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetWindowLevelModeChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->WindowLevelModeChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::InvokeWindowLevelModeChangedCommand(int mode)
{
  if (this->WindowLevelModeChangedCommand && 
      *this->WindowLevelModeChangedCommand && 
      this->GetApplication())
    {
    this->Script("%s %d", this->WindowLevelModeChangedCommand, mode);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::WindowLevelModeCallback(int state)
{
  this->SetWindowLevelMode(state);
}

//----------------------------------------------------------------------------
unsigned long vtkKWPiecewiseFunctionEditor::GetRedrawFunctionTime()
{
  unsigned long t = this->Superclass::GetRedrawFunctionTime();
  if (this->PointColorTransferFunction &&
      this->PointColorTransferFunction->GetMTime() > t)
    {
    return this->PointColorTransferFunction->GetMTime();
    }

  return t;
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ValueEntryVisibility: "
     << (this->ValueEntryVisibility ? "On" : "Off") << endl;

  os << indent << "WindowLevelMode: "
     << (this->WindowLevelMode ? "On" : "Off") << endl;

  os << indent << "WindowLevelModeButtonVisibility: "
     << (this->WindowLevelModeButtonVisibility ? "On" : "Off") << endl;

  os << indent << "WindowLevelModeLockEndPointValue: "
     << (this->WindowLevelModeLockEndPointValue ? "On" : "Off") << endl;

  os << indent << "Window: " << this->Window << endl;
  os << indent << "Level: " << this->Level << endl;

  os << indent << "PiecewiseFunction: ";
  if (this->PiecewiseFunction)
    {
    os << endl;
    this->PiecewiseFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ValueEntry: ";
  if (this->ValueEntry)
    {
    os << endl;
    this->ValueEntry->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "PointColorTransferFunction: ";
  if (this->PointColorTransferFunction)
    {
    os << endl;
    this->PointColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "WindowLevelModeCheckButton: ";
  if (this->WindowLevelModeCheckButton)
    {
    os << endl;
    this->WindowLevelModeCheckButton->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}

