/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWPiecewiseFunctionEditor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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

vtkStandardNewMacro(vtkKWPiecewiseFunctionEditor);
vtkCxxRevisionMacro(vtkKWPiecewiseFunctionEditor, "1.1");

//----------------------------------------------------------------------------
vtkKWPiecewiseFunctionEditor::vtkKWPiecewiseFunctionEditor()
{
  this->PiecewiseFunction = NULL;

  this->WindowLevelMode = 0;
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
int vtkKWPiecewiseFunctionEditor::FunctionPointParameterIsLocked(int id)
{
  return (this->HasFunction() &&
          (this->LockEndPoints || this->WindowLevelMode) &&
          (id == 0 || id == this->GetFunctionSize() - 1));
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::FunctionPointValueIsLocked(int id)
{
  return (this->HasFunction() &&
          this->WindowLevelMode &&
          (id == this->GetFunctionSize() - 1 ||
           this->GetFunctionSize() > 1 && id == this->GetFunctionSize() - 2));
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
  
  float *v_w_range = this->GetWholeValueRange();
  float value = this->PiecewiseFunction->GetDataPointer()[id * 2 + 1];
  float gray = (value - v_w_range[0]) / (v_w_range[1] - v_w_range[0]);
  rgb[0] = rgb[1] = rgb[2] = gray;
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::GetFunctionPointAsCanvasCoordinates(
  int id, int &x, int &y)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  float *v_w_range = this->GetWholeValueRange();
  float *point = this->PiecewiseFunction->GetDataPointer() + id * 2;

  x = vtkMath::Round(point[0] * factors[0]);
  y = vtkMath::Round((double)(v_w_range[1] - point[1]) * factors[1]);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::AddFunctionPointAtCanvasCoordinates( 
  int x, int y, int &new_id)
{
  if (!this->IsCreated() || !this->HasFunction() || this->DisableAddAndRemove)
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  float *v_w_range = this->GetWholeValueRange();
  float parameter = (float)((double)x / factors[0]);
  float value = (float)(v_w_range[1] - ((double)y / factors[1]));

  new_id = this->PiecewiseFunction->AddPoint(parameter, value);

  this->RedrawCanvasElements();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::UpdateFunctionPointFromCanvasCoordinates(
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
  float parameter = point[0];

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

  if (!this->FunctionPointParameterIsLocked(id))
    {
    this->PiecewiseFunction->RemovePoint(parameter);
    parameter = (float)((double)x / factors[0]);
    }

  int new_id = this->PiecewiseFunction->AddPoint(parameter, value);

  // If the point was selected and the new point does not match (which
  // should not happen anyway), reselect the new point

  if (this->SelectedPoint >= 0 && this->SelectedPoint == id && id != new_id)
    {
    this->SelectPoint(new_id);
    }
  else
    {
    this->RedrawCanvasPoint(new_id);
    }

  // In window-level mode, the first and second point are constrained

  if (this->WindowLevelMode && new_id <= 1)
    {
    float constrained_parameter = this->PiecewiseFunction->GetDataPointer()[ 
      (new_id == 0 ? 1 : 0) * 2];
    this->RedrawCanvasPoint(
      this->PiecewiseFunction->AddPoint(constrained_parameter, value));
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWPiecewiseFunctionEditor::RemoveFunctionPoint(int id)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      !this->FunctionPointIsRemovable(id))
    {
    return 0;
    }

  if (id == this->SelectedPoint)
    {
    this->ClearSelection();
    }

  this->PiecewiseFunction->RemovePoint(
    this->PiecewiseFunction->GetDataPointer()[id * 2]);

  this->RedrawCanvasElements();

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
void vtkKWPiecewiseFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "PiecewiseFunction: " 
     << this->PiecewiseFunction << endl;

  os << indent << "WindowLevelMode: "
     << (this->WindowLevelMode ? "On" : "Off") << endl;
}
