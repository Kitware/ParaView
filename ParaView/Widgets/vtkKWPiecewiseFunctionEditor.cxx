/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkKWPiecewiseFunctionEditor.h"

#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkMath.h"
#include "vtkKWRange.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWEvent.h"

vtkStandardNewMacro(vtkKWPiecewiseFunctionEditor);
vtkCxxRevisionMacro(vtkKWPiecewiseFunctionEditor, "1.7");

//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::vtkKWPiecewiseFunctionEditor()
{
  this->PiecewiseFunction = NULL;

  this->WindowLevelMode = 0;
  this->WindowLevelModeLockEndPointValue = 0;
  this->Window = 1.0;
  this->Level = 1.0;
}

//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::~vtkKWPiecewiseFunctionEditor()
{
  this->SetPiecewiseFunction(NULL);
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

  this->Update();
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
int vtkKWPiecewiseFunctionEditor::GetFunctionPointColor(int id, float rgb[3])
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  if (!this->ComputePointColorFromValue)
    {
    return this->Superclass::GetFunctionPointColor(id, rgb);
    }
  
  float value = this->PiecewiseFunction->GetDataPointer()[id * 2 + 1];

  float *v_w_range = this->GetWholeValueRange();
  float gray = (value - v_w_range[0]) / (v_w_range[1] - v_w_range[0]);
  rgb[0] = rgb[1] = rgb[2] = gray;
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointParameter(
  int id, float &parameter)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  parameter = this->PiecewiseFunction->GetDataPointer()[id * 2];
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointCanvasCoordinates(
  int id, int &x, int &y)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  float *point = this->PiecewiseFunction->GetDataPointer() + id * 2;

  float *v_w_range = this->GetWholeValueRange();

  x = vtkMath::Round(point[0] * factors[0]);
  y = vtkMath::Round((double)(v_w_range[1] - point[1]) * factors[1]);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::AddFunctionPointAtCanvasCoordinates( 
  int x, int y, int &id)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || 
      !this->FunctionPointCanBeAdded())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  // Get the parameter/value given the canvas coords and scaling factor

  float *v_w_range = this->GetWholeValueRange();

  float parameter = (float)((double)x / factors[0]);
  float value = (float)(v_w_range[1] - ((double)y / factors[1]));

  // Add the point and redraw if a point was really added

  int old_size = this->GetFunctionSize();
  id = this->PiecewiseFunction->AddPoint(parameter, value);
  if (old_size != this->GetFunctionSize())
    {
    this->RedrawCanvasPoint(id);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::AddFunctionPointAtParameter(
  float parameter, int &id)
{
  if (!this->HasFunction() || !this->FunctionPointCanBeAdded())
    {
    return 0;
    }

  // Get the interpolated value

  float value = this->PiecewiseFunction->GetValue(parameter);

  // Add the point and redraw if a point was really added

  int old_size = this->GetFunctionSize();
  id = this->PiecewiseFunction->AddPoint(parameter, value);
  if (old_size != this->GetFunctionSize())
    {
    this->RedrawCanvasPoint(id);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::MoveFunctionPointToCanvasCoordinates(
  int id, int x, int y)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  float *point = this->PiecewiseFunction->GetDataPointer() + id * 2;

  // Get current value if point value is locked, or new value given the y coord

  float value;
  if (this->FunctionPointValueIsLocked(id))
    {
    value = point[1];
    }
  else
    {
    float *v_w_range = this->GetWholeValueRange();
    value = (float)(v_w_range[1] - ((double)y / factors[1]));
    }

  // Get current param if point param is locked, or new param given the x coord

  float parameter = point[0];
  if (!this->FunctionPointParameterIsLocked(id))
    {
    this->PiecewiseFunction->RemovePoint(parameter);
    parameter = (float)((double)x / factors[0]);
    }

  // Add a point at this parameter/value (will be updated if already exist)

  int new_id = this->PiecewiseFunction->AddPoint(parameter, value);

  // If the point was selected and the new point does not match (which
  // should not happen anyway), reselect the new point

  if (this->HasSelection() && this->SelectedPoint == id && id != new_id)
    {
    this->SelectPoint(new_id);
    }
  else
    {
    this->RedrawCanvasPoint(new_id);
    }

  // In window-level mode, the first and second point are value-constrained
  // (so are the last and last - 1 point too)

  int fsize = this->GetFunctionSize();
  if (this->WindowLevelMode && 
      (new_id <= 1 || (fsize >= 2 && new_id >= fsize - 2)))
    {
    if (new_id <= 1)
      {
      id = (new_id == 0) ? 1 : 0;
      }
    else
      {
      id = (new_id == fsize - 2) ? fsize - 1 : fsize - 2;
      }
    this->RedrawCanvasPoint(
      this->PiecewiseFunction->AddPoint(
        this->PiecewiseFunction->GetDataPointer()[id * 2], value));
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::MoveFunctionPointToParameter(
  int id, float parameter, int interpolate)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      this->FunctionPointParameterIsLocked(id))
    {
    return 0;
    }

  float *point = this->PiecewiseFunction->GetDataPointer() + id * 2;

  float old_parameter = point[0];
  if (parameter == old_parameter)
    {
    return 0;
    }

  // Get current value if point value is locked or no interpolation

  float value;
  if (!interpolate || this->FunctionPointValueIsLocked(id))
    {
    value = point[1];
    }
  else
    {
    value = this->PiecewiseFunction->GetValue(old_parameter);
    }

  // Remove the old point

  this->PiecewiseFunction->RemovePoint(old_parameter);

  // Add new point at this parameter/value

  int new_id = this->PiecewiseFunction->AddPoint(parameter, value);

  // If the point was selected and the new point does not match (which
  // should not happen anyway), reselect the new point

  if (this->HasSelection() && this->SelectedPoint == id && id != new_id)
    {
    this->SelectPoint(new_id);
    }
  else
    {
    this->RedrawCanvasPoint(new_id);
    }

  // In window-level mode, the first and second point are value-constrained
  // (so are the last and last - 1 point too)

  int fsize = this->GetFunctionSize();
  if (this->WindowLevelMode && 
      (new_id <= 1 || (fsize >= 2 && new_id >= fsize - 2)))
    {
    if (new_id <= 1)
      {
      id = (new_id == 0) ? 1 : 0;
      }
    else
      {
      id = (new_id == fsize - 2) ? fsize - 1 : fsize - 2;
      }
    this->RedrawCanvasPoint(
      this->PiecewiseFunction->AddPoint(
        this->PiecewiseFunction->GetDataPointer()[id * 2], value));
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::RemoveFunctionPoint(int id)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      !this->FunctionPointCanBeRemoved(id))
    {
    return 0;
    }

  // If selected, deselect first

  if (id == this->SelectedPoint)
    {
    this->ClearSelection();
    }

  // Remove the point and redraw if a point was really removed

  int old_size = this->GetFunctionSize();
  this->PiecewiseFunction->RemovePoint(
    this->PiecewiseFunction->GetDataPointer()[id * 2]);
  if (old_size != this->GetFunctionSize())
    {
    this->RedrawCanvasPoint(id);
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::UpdateInfoLabelWithFunctionPoint(int id)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return;
    }

  float *point = this->PiecewiseFunction->GetDataPointer() + id * 2;

  char format[1024];
  sprintf(format, "%%d: [%%.%df, %%.%df]",
          this->ParameterRange->GetEntriesResolution(),
          this->ValueRange->GetEntriesResolution());

  char range[1024];
  sprintf(range, format, id, point[0], point[1]);
  this->InfoLabel->SetLabel2(range);
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

    float parameter;
    float *v_w_range = this->GetWholeValueRange();

    if (this->GetFunctionSize() > 0 && 
        this->GetFunctionPointParameter(0, parameter))
      {
      this->PiecewiseFunction->AddPoint(parameter, v_w_range[0]);
      }
    if (this->GetFunctionSize() > 1 &&
        this->GetFunctionPointParameter(this->GetFunctionSize()-1, parameter))
      {
      this->PiecewiseFunction->AddPoint(parameter, v_w_range[1]);
      }
    }

  this->UpdatePointsFromWindowLevel();
}

//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::SetWindowLevel(float window, float level)
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
  float window, float level)
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
    float p1, p2;
    if (this->GetFunctionPointParameter(1, p1) && 
        this->GetFunctionPointParameter(2, p2))
      {
      float v1, v2;
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

  float *p_w_range = this->GetWholeParameterRange();
  float *v_w_range = this->GetWholeValueRange();

  float parameter;

  // We are in not WindowLevel mode, make sure our points are within the
  // range (while in W/L mode, those points can be out of the parameter range)

  if (!this->WindowLevelMode)
    {
    int done;
    do
      {
      done = 1;
      for (int id = 0; id < this->GetFunctionSize(); id++)
        {
        if (this->GetFunctionPointParameter(id, parameter) &&
            (parameter < p_w_range[0] || parameter > p_w_range[1]))
          {
          float value = this->PiecewiseFunction->GetValue(parameter);
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

    float start_v, end_v;
    if (this->GetFunctionSize() > 0 && 
        this->GetFunctionPointParameter(0, parameter))
      {
      start_v = this->PiecewiseFunction->GetValue(parameter);
      }
    else
      {
      start_v = v_w_range[0];
      }
    if (this->GetFunctionSize() > 1 &&
        this->GetFunctionPointParameter(this->GetFunctionSize()-1, parameter))
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
      float temp = start_v;
      start_v = end_v;
      end_v = temp;
      }

    // Compute the 4 points parameters 

    float points[4];
    float window = this->Window > 0 ? this->Window : -this->Window;

    points[1] = this->Level - window / 2;
    points[0] = (points[1] > p_w_range[0]) ? p_w_range[0] : points[1] - 0.001;
    points[2] = this->Level + window / 2;
    points[3] = (points[2] < p_w_range[1]) ? p_w_range[1] : points[2] + 0.001;
  
    // Remove all extra-points

    while (this->GetFunctionSize() > 4)
      {
      if (this->GetFunctionPointParameter(this->GetFunctionSize()-1,parameter))
        {
        this->PiecewiseFunction->RemovePoint(parameter);
        }
      }

    // Check if modification is needed (if any of those points is different,
    // just remove everything)

    for (int id = 0; id < 4; id++)
      {
      if (!this->GetFunctionPointParameter(id, parameter) ||
          parameter != points[id] ||
          this->PiecewiseFunction->GetValue(parameter) != 
          (id < 2 ? start_v : end_v))
        {
        this->PiecewiseFunction->RemoveAllPoints();
        break;
        }
      }

    // Set the points
  
    for (int id = 0; id < 4; id++)
      {
      this->PiecewiseFunction->AddPoint(points[id], id < 2 ? start_v : end_v);
      }
    }

  // Was the function modified ?

  if (this->GetFunctionMTime() > mtime)
    {
    if (interactive)
      {
      this->InvokeFunctionChangingCommand();
      }
    else
      {
      this->InvokeFunctionChangedCommand();
      }
    this->RedrawCanvasElements();
    }
}
  
//----------------------------------------------------------------------------
void vtkKWPiecewiseFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PiecewiseFunction: " 
     << this->PiecewiseFunction << endl;

  os << indent << "WindowLevelMode: "
     << (this->WindowLevelMode ? "On" : "Off") << endl;

  os << indent << "WindowLevelModeLockEndPointValue: "
     << (this->WindowLevelModeLockEndPointValue ? "On" : "Off") << endl;
}

