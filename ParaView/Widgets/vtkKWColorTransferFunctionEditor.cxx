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
#include "vtkKWColorTransferFunctionEditor.h"

#include "vtkColorTransferFunction.h"
#include "vtkKWFrame.h"
#include "vtkKWImageLabel.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWRange.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkKWColorTransferFunctionEditor);
vtkCxxRevisionMacro(vtkKWColorTransferFunctionEditor, "1.14");

#define VTK_KW_CTF_EDITOR_RGB_LABEL "RGB"
#define VTK_KW_CTF_EDITOR_HSV_LABEL "HSV"

//----------------------------------------------------------------------------
vtkKWColorTransferFunctionEditor::vtkKWColorTransferFunctionEditor()
{
  this->ColorTransferFunction = NULL;

  this->ComputePointColorFromValue = 1;

  this->ColorSpaceOptionMenu = vtkKWOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkKWColorTransferFunctionEditor::~vtkKWColorTransferFunctionEditor()
{
  this->SetColorTransferFunction(NULL);

  if (this->ColorSpaceOptionMenu)
    {
    this->ColorSpaceOptionMenu->Delete();
    this->ColorSpaceOptionMenu = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorTransferFunction(
  vtkColorTransferFunction *arg)
{
  if (this->ColorTransferFunction == arg)
    {
    return;
    }

  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->UnRegister(this);
    }
    
  this->ColorTransferFunction = arg;

  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->Register(this);
    }

  this->Modified();

  this->LastRedrawCanvasElementsTime = 0;

  this->Update();
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::HasFunction()
{
  return this->ColorTransferFunction ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionSize()
{
  return this->ColorTransferFunction ? 
    this->ColorTransferFunction->GetSize() : 0;
}

//----------------------------------------------------------------------------
unsigned long vtkKWColorTransferFunctionEditor::GetFunctionMTime()
{
  return this->ColorTransferFunction ? 
    this->ColorTransferFunction->GetMTime() : 0;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::FunctionPointValueIsLocked(int id)
{
  // We have no control on the vaue space, so always lock it

  return this->Superclass::FunctionPointValueIsLocked(id) || 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointColor(
  int id, float rgb[3])
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  if (!this->ComputePointColorFromValue)
    {
    return this->Superclass::GetFunctionPointColor(id, rgb);
    }

  float *p_rgb = this->ColorTransferFunction->GetDataPointer() + id * 4 + 1;

  rgb[0] = p_rgb[0];
  rgb[1] = p_rgb[1];
  rgb[2] = p_rgb[2];
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointParameter(
  int id, float &parameter)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  parameter = this->ColorTransferFunction->GetDataPointer()[id * 4];
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointCanvasCoordinates(
  int id, int &x, int &y)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  float parameter = this->ColorTransferFunction->GetDataPointer()[id * 4];

  // Since the 'value' range is multi-dimensional (color), just place
  // the point in the middle of the current value range

  float *v_w_range = this->GetWholeValueRange();

  x = vtkMath::Round(parameter * factors[0]);
  y = vtkMath::Round((double)(v_w_range[1] - v_w_range[0]) * 0.5 * factors[1]);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::AddFunctionPointAtCanvasCoordinates(
  int x, int vtkNotUsed(y), int &id)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || 
      !this->FunctionPointCanBeAdded())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  // Since the 'value' range is multi-dimensional (color), just add
  // a point interpolated from the previous and next point (if any)

  float parameter = (float)((double)x / factors[0]);
  float rgb[3] = { 1.0, 1.0, 1.0 };
  this->ColorTransferFunction->GetColor(parameter, rgb);

  // Add the point

  int old_size = this->GetFunctionSize();

  id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::AddFunctionPointAtParameter(
  float parameter, int &id)
{
  if (!this->HasFunction() || !this->FunctionPointCanBeAdded())
    {
    return 0;
    }

  // Get the interpolated value

  float rgb[3] = { 1.0, 1.0, 1.0 };
  this->ColorTransferFunction->GetColor(parameter, rgb);

  // Add the point

  int old_size = this->GetFunctionSize();

  id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::MoveFunctionPointToCanvasCoordinates(
  int id, int x, int vtkNotUsed(y))
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  float *point = this->ColorTransferFunction->GetDataPointer() + id * 4;

  // Since the 'value' range is multi-dimensional (color), just move
  // the point in the 'parameter' range (ignore the 'y')

  float rgb[3];
  rgb[0] = point[1];
  rgb[1] = point[2];
  rgb[2] = point[3];

  // Get current param if point param is locked, or new param given the x coord

  float parameter = point[0];
  if (!this->FunctionPointParameterIsLocked(id))
    {
    this->ColorTransferFunction->RemovePoint(parameter);
    parameter = (float)((double)x / factors[0]);
    }

  // Add a point at this parameter/value (will be updated if already exist)

  int new_id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  // Redraw the point
  // Note that we *imply* here that new_id == id (i.e., we are moving the
  // point without changing the order of the points)

  this->RedrawCanvasPoint(new_id);
  if (new_id == this->SelectedPoint)
    {
    this->UpdatePointLabelWithFunctionPoint(new_id);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::MoveFunctionPointToParameter(
  int id, float parameter, int interpolate)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      this->FunctionPointParameterIsLocked(id))
    {
    return 0;
    }

  float *point = this->ColorTransferFunction->GetDataPointer() + id * 4;

  float old_parameter = point[0];
  if (parameter == old_parameter)
    {
    return 0;
    }

  // Get current value if point value is locked or no interpolation

  float rgb[3];
  if (!interpolate || this->FunctionPointValueIsLocked(id))
    {
    rgb[0] = point[1];
    rgb[1] = point[2];
    rgb[2] = point[3];
    }
  else
    {
    this->ColorTransferFunction->GetColor(parameter, rgb);
    }

  // Remove the old point

  this->ColorTransferFunction->RemovePoint(old_parameter);

  // Add new point at this parameter/value

  int new_id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  // Redraw the point
  // Note that we *imply* here that new_id == id (i.e., we are moving the
  // point without changing the order of the points)

  this->RedrawCanvasPoint(new_id);
  if (new_id == this->SelectedPoint)
    {
    this->UpdatePointLabelWithFunctionPoint(new_id);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::RemoveFunctionPoint(int id)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      !this->FunctionPointCanBeRemoved(id))
    {
    return 0;
    }

  // Remove the point

  int old_size = this->GetFunctionSize();

  this->ColorTransferFunction->RemovePoint(
    this->ColorTransferFunction->GetDataPointer()[id * 4]);

  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdatePointLabelWithFunctionPoint(int id)
{
  if (!this->IsCreated())
    {
    return;
    }
  
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    this->PointLabel->SetLabel2("");
    return;
    }

  float *point = this->ColorTransferFunction->GetDataPointer() + id * 4;
  float parameter = point[0];
  float *rgb = point + 1;

  char format[1024];
  sprintf(format, "%%d: (%%.%df; %%.2f, %%.2f, %%.2f)",
          this->ParameterRange->GetEntriesResolution());
  
  char range[1024];
  if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_HSV)
    {
    float hsv[3];
    vtkMath::RGBToHSV(rgb, hsv);
    sprintf(range, format, id, parameter, hsv[0], hsv[1], hsv[2]);
    }
  else
    {
    sprintf(range, format, id, parameter, rgb[0], rgb[1], rgb[2]);
    }
  this->PointLabel->SetLabel2(range);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::Create(vtkKWApplication *app, 
                                               const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("ColorTransferFunctionEditor already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app, args);

  // Add the color space option menu

  this->ColorSpaceOptionMenu->SetParent(this->TitleFrame);
  this->ColorSpaceOptionMenu->Create(app, "-padx 1 -pady 1");
  this->ColorSpaceOptionMenu->IndicatorOff();
  this->ColorSpaceOptionMenu->SetBalloonHelpString(
    "Change the interpolation color space to RGB or HSV.");

  this->ColorSpaceOptionMenu->AddEntryWithCommand(
    VTK_KW_CTF_EDITOR_RGB_LABEL, this, "ColorSpaceToRGBCallback");

  this->ColorSpaceOptionMenu->AddEntryWithCommand(
    VTK_KW_CTF_EDITOR_HSV_LABEL, this, "ColorSpaceToHSVCallback");

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Pack the whole widget

  this->Superclass::Pack();

  ostrstream tk_cmd;

  // Add the color space menu

  if (this->ColorSpaceOptionMenu->IsCreated())
    {
    tk_cmd << "pack " << this->ColorSpaceOptionMenu->GetWidgetName() 
           << " -side left -fill x -padx 0" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::Update()
{
  this->Superclass::Update();

  if (!this->IsCreated())
    {
    return;
    }

  // Update the color space menu the reflect the current color space

  if (this->ColorSpaceOptionMenu && this->ColorTransferFunction)
    {
    switch (this->ColorTransferFunction->GetColorSpace())
      {
      case VTK_CTF_HSV:
        this->ColorSpaceOptionMenu->SetValue(VTK_KW_CTF_EDITOR_HSV_LABEL);
        break;
      default:
      case VTK_CTF_RGB:
        this->ColorSpaceOptionMenu->SetValue(VTK_KW_CTF_EDITOR_RGB_LABEL);
        break;
      }
    }

  // Since the 'value' range is multi-dimensional (color), there is no
  // point in changing it

  if (this->ValueRange)
    {
    this->ValueRange->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->ColorSpaceOptionMenu)
    {
    this->ColorSpaceOptionMenu->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdateRangeLabelWithRange()
{
  if (!this->IsCreated() || !this->RangeLabel || !this->RangeLabel->IsAlive())
    {
    return;
    }

  ostrstream ranges;
  int nb_ranges = 0;

  float *param = GetVisibleParameterRange();
  if (param && !this->HideParameterRange)
    {
    char format[1024], range[1024];
    sprintf(format, "[%%.%df, %%.%df]",
            this->ParameterRange->GetEntriesResolution(),
            this->ParameterRange->GetEntriesResolution());
    sprintf(range, format, param[0], param[1]);
    ranges << range;
    nb_ranges++;
    }

  ranges << ends;
  this->RangeLabel->SetLabel(ranges.str());
  ranges.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::ColorSpaceToRGBCallback()
{
  if (this->ColorTransferFunction &&
      this->ColorTransferFunction->GetColorSpace() != VTK_CTF_RGB)
    {
    this->ColorTransferFunction->SetColorSpaceToRGB();
    this->Update();
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::ColorSpaceToHSVCallback()
{
  if (this->ColorTransferFunction &&
      this->ColorTransferFunction->GetColorSpace() != VTK_CTF_HSV)
    {
    this->ColorTransferFunction->SetColorSpaceToHSV();
    this->Update();
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ColorTransferFunction: " 
     << this->ColorTransferFunction << endl;
}

