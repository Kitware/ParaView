/*=========================================================================

  Module:    vtkKWPiecewiseFunctionEditor.cxx

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
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWRange.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"

vtkStandardNewMacro(vtkKWPiecewiseFunctionEditor);
vtkCxxRevisionMacro(vtkKWPiecewiseFunctionEditor, "1.19");


int vtkKWPiecewiseFunctionEditorCommand(ClientData cd, Tcl_Interp *interp,
                             int argc, char *argv[]);
//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::vtkKWPiecewiseFunctionEditor()
{
  this->PiecewiseFunction                = NULL;

  this->WindowLevelMode                  = 0;
  this->WindowLevelModeLockEndPointValue = 0;
  this->ShowWindowLevelModeButton        = 0;
  this->ShowValueEntry                   = 1;

  this->Window                           = 1.0;
  this->Level                            = 1.0;

  this->WindowLevelModeChangedCommand    = NULL;

  this->ValueEntry                       = vtkKWLabeledEntry::New();

  this->WindowLevelModeCheckButton       = vtkKWCheckButton::New();
}

//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::~vtkKWPiecewiseFunctionEditor()
{
  this->SetPiecewiseFunction(NULL);

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
    }

  this->Modified();

  this->LastRedrawFunctionTime = 0;

  this->Update();
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
  return (this->Superclass::FunctionPointValueIsLocked(id) ||
          (this->HasFunction() &&
           this->WindowLevelMode && 
           this->WindowLevelModeLockEndPointValue &&
           ((this->GetFunctionSize() > 0 && id==this->GetFunctionSize()-1) ||
            (this->GetFunctionSize() > 1 && id==this->GetFunctionSize()-2))));
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

  *parameter = this->PiecewiseFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
  
  return 1;
}

//----------------------------------------------------------------------------
inline int vtkKWPiecewiseFunctionEditor::GetFunctionPointDimensionality()
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

  values[0] = this->PiecewiseFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality()) + 1];
  
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

  double value;
  vtkMath::ClampValue(values[0], this->GetWholeValueRange(), &value);

  this->PiecewiseFunction->AddPoint(parameter, value);
  
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
  double value;
  vtkMath::ClampValue(values[0], this->GetWholeValueRange(), &value);

  // Add the point

  int old_size = this->GetFunctionSize();
  *id = this->PiecewiseFunction->AddPoint(parameter, value);
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

  // Clamp

  vtkMath::ClampValue(&parameter, this->GetWholeParameterRange());
  double value;
  vtkMath::ClampValue(values[0], this->GetWholeValueRange(), &value);

  if (parameter != old_parameter)
    {
    this->PiecewiseFunction->RemovePoint(old_parameter);
    }
  int new_id = this->PiecewiseFunction->AddPoint(parameter, value);

  if (new_id != id)
    {
    vtkWarningMacro(<< "Setting a function point (id: " << id << ") parameter/values resulted in a different point (id:" << new_id << "). Inconsistent.");
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

  double parameter = this->PiecewiseFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
  
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
void vtkKWPiecewiseFunctionEditor::UpdatePointEntries(int id)
{
  this->Superclass::UpdatePointEntries(id);

  if (!this->IsCreated())
    {
    return;
    }
  
  // No point ? Empty the entry and disable

  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    this->ValueEntry->GetEntry()->SetValue("");
    this->ValueEntry->SetEnabled(0);
    return;
    }

  // Disable entry if value is locked

  this->ValueEntry->SetEnabled(
    this->FunctionPointValueIsLocked(id) ? 0 : this->Enabled);

  // Get the value

  double *point = this->PiecewiseFunction->GetDataPointer() + id * 2;

  this->ValueEntry->GetEntry()->SetValue(point[1], 3);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::Create(vtkKWApplication *app, 
                                          const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("PiecewiseFunctionEditor already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app, args);

  // Create the value entry

  if (this->ShowValueEntry)
    {
    this->CreateValueEntry(app);
    }

  // Window/Level mode

  if (this->ShowWindowLevelModeButton)
    {
    this->CreateWindowLevelModeCheckButton(app);
    }

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::CreateWindowLevelModeCheckButton(
  vtkKWApplication *app)
{
  if (this->WindowLevelModeCheckButton && 
      !this->WindowLevelModeCheckButton->IsCreated())
    {
    this->CreateTopLeftFrame(app);
    this->WindowLevelModeCheckButton->SetParent(this->TopLeftFrame);
    this->WindowLevelModeCheckButton->Create(
      app, "-padx 0 -pady 0 -highlightthickness 0");
    this->WindowLevelModeCheckButton->SetIndicator(0);
    this->WindowLevelModeCheckButton->SetBalloonHelpString(
      "Place the editor in window/level mode.");
    this->WindowLevelModeCheckButton->SetCommand(
      this, "WindowLevelModeCallback");
    this->WindowLevelModeCheckButton->SetImageOption(
      vtkKWIcon::ICON_WINDOW_LEVEL);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::CreateValueEntry(
  vtkKWApplication *app)
{
  if (this->ValueEntry && !this->ValueEntry->IsCreated())
    {
    this->CreateTopRightFrame(app);
    this->ValueEntry->SetParent(this->TopRightFrame);
    this->ValueEntry->Create(app, "");
    this->ValueEntry->GetEntry()->SetWidth(6);
    this->ValueEntry->SetLabel("V:");

    this->UpdatePointEntries(this->SelectedPoint);

    this->ValueEntry->GetEntry()->BindCommand(
      this, "ValueEntryCallback");
    }
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::IsTopLeftFrameUsed()
{
  return (this->Superclass::IsTopLeftFrameUsed() || 
          this->ShowWindowLevelModeButton);
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::IsTopRightFrameUsed()
{
  return (this->Superclass::IsTopRightFrameUsed() || 
          this->ShowValueEntry);
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

  // Value entry (in top right frame)

  if (this->ShowValueEntry && this->ValueEntry->IsCreated())
    {
    tk_cmd << "pack " << this->ValueEntry->GetWidgetName() 
           << " -side left" << endl;
    }

  // Window/Level mode (in top left frame)

  if (this->ShowWindowLevelModeButton &&
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
void vtkKWPiecewiseFunctionEditor::Update()
{
  this->Superclass::Update();

  // No selection, disable value entry

  if (!this->HasSelection())
    {
    this->ValueEntry->SetEnabled(0);
    }

  // Window/Level mode

  if (this->WindowLevelModeCheckButton)
    {
    this->WindowLevelModeCheckButton->SetState(this->WindowLevelMode);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->ValueEntry)
    {
    this->ValueEntry->SetEnabled(this->Enabled);
    }

  if (this->WindowLevelModeCheckButton)
    {
    this->WindowLevelModeCheckButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::InvokeFunctionChangedCommand()
{
  if (this->WindowLevelMode)
    {
    this->UpdateWindowLevelFromPoints();
    }

  this->Superclass::InvokeFunctionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::InvokeFunctionChangingCommand()
{
  if (this->WindowLevelMode)
    {
    this->UpdateWindowLevelFromPoints();
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

    if (this->GetFunctionSize() > 0 && 
        this->GetFunctionPointParameter(0, &parameter))
      {
      this->PiecewiseFunction->AddPoint(parameter, v_w_range[0]);
      }
    if (this->GetFunctionSize() > 1 &&
        this->GetFunctionPointParameter(this->GetFunctionSize()-1, &parameter))
      {
      this->PiecewiseFunction->AddPoint(parameter, v_w_range[1]);
      }
    }

  this->InvokeWindowLevelModeChangedCommand();

  this->UpdatePointsFromWindowLevel();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetShowValueEntry(int arg)
{
  if (this->ShowValueEntry == arg)
    {
    return;
    }

  this->ShowValueEntry = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ShowValueEntry && this->IsCreated())
    {
    this->CreateValueEntry(this->GetApplication());
    }

  this->UpdatePointEntries(this->SelectedPoint);

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetShowWindowLevelModeButton(int arg)
{
  if (this->ShowWindowLevelModeButton == arg)
    {
    return;
    }

  this->ShowWindowLevelModeButton = arg;

  // Make sure that if the button has to be shown, we create it on the fly if
  // needed

  if (this->ShowWindowLevelModeButton && this->IsCreated())
    {
    this->CreateWindowLevelModeCheckButton(this->GetApplication());
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
    double p1, p2;
    if (this->GetFunctionPointParameter(1, &p1) && 
        this->GetFunctionPointParameter(2, &p2))
      {
      double v1, v2;
      v1 = this->PiecewiseFunction->GetValue(p1);
      v2 = this->PiecewiseFunction->GetValue(p2);
      this->Window = (v1 < v2 ? (p2 - p1) : p1 - p2);
      this->Level = (p1 + p2) / 2.0;
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
          double value = this->PiecewiseFunction->GetValue(parameter);
          this->PiecewiseFunction->RemovePoint(parameter);
          this->PiecewiseFunction->AddPoint(
            (parameter < p_w_range[0] ? p_w_range[0] : p_w_range[1]), value);
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
    if (this->GetFunctionSize() > 0 && 
        this->GetFunctionPointParameter(0, &parameter))
      {
      start_v = this->PiecewiseFunction->GetValue(parameter);
      }
    else
      {
      start_v = v_w_range[0];
      }
    if (this->GetFunctionSize() > 1 &&
        this->GetFunctionPointParameter(this->GetFunctionSize()-1, &parameter))
      {
      end_v = this->PiecewiseFunction->GetValue(parameter);
      }
    else
      {
      end_v = v_w_range[1];
      }

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

    points[1] = (this->Level - window / 2.0);
    points[0] = (points[1] > p_w_range[0]) ? p_w_range[0] : points[1] - 0.001;
    points[2] = (this->Level + window / 2.0);
    points[3] = (points[2] < p_w_range[1]) ? p_w_range[1] : points[2] + 0.001;
  
    // Remove all extra-points

    while (this->GetFunctionSize() > 4)
      {
      if (this->GetFunctionPointParameter(this->GetFunctionSize()-1, &parameter))
        {
        this->PiecewiseFunction->RemovePoint(parameter);
        }
      }

    // Check if modification is needed (if any of those points is different,
    // just remove everything)

    for (id = 0; id < 4; id++)
      {
      if (!this->GetFunctionPointParameter(id, &parameter) ||
          parameter != points[id] ||
          this->PiecewiseFunction->GetValue(parameter) != 
          (id < 2 ? start_v : end_v))
        {
        this->PiecewiseFunction->RemoveAllPoints();
        break;
        }
      }

    // Set the points
  
    for (id = 0; id < 4; id++)
      {
      this->PiecewiseFunction->AddPoint(points[id], id < 2 ? start_v : end_v);
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
void vtkKWPiecewiseFunctionEditor::ValueEntryCallback()
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

  double value = this->ValueEntry->GetEntry()->GetValueAsFloat();

  // Move the point, check if something has really been moved

  unsigned long mtime = this->GetFunctionMTime();

  this->MoveFunctionPoint(this->GetSelectedPoint(), parameter, &value);

  if (this->GetFunctionMTime() > mtime)
    {
    this->InvokePointMovedCommand(this->SelectedPoint);
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::InvokeWindowLevelModeChangedCommand()
{
  this->InvokeCommand(this->WindowLevelModeChangedCommand);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetWindowLevelModeChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->WindowLevelModeChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::WindowLevelModeCallback()
{
  if (!this->WindowLevelModeCheckButton)
    {
    return;
    }

  this->SetWindowLevelMode(this->WindowLevelModeCheckButton->GetState());
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowValueEntry: "
     << (this->ShowValueEntry ? "On" : "Off") << endl;

  os << indent << "WindowLevelMode: "
     << (this->WindowLevelMode ? "On" : "Off") << endl;

  os << indent << "ShowWindowLevelModeButton: "
     << (this->ShowWindowLevelModeButton ? "On" : "Off") << endl;

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

