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
vtkCxxRevisionMacro(vtkKWColorTransferFunctionEditor, "1.5");

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
int vtkKWColorTransferFunctionEditor::FunctionPointValueIsLocked(int)
{
  return 1;
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

  float *v_w_range = this->GetWholeValueRange();
  float parameter = this->ColorTransferFunction->GetDataPointer()[id * 4];

  // Since the 'value' range is multi-dimensional (color), just place
  // the point in the middle of the current value range

  x = vtkMath::Round(parameter * factors[0]);
  y = vtkMath::Round((double)(v_w_range[1] - v_w_range[0]) * 0.5 * factors[1]);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::AddFunctionPointAtCanvasCoordinates(
  int x, int vtkNotUsed(y), int &id)
{
  if (!this->IsCreated() || !this->HasFunction() || this->DisableAddAndRemove)
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

  // Add the point and redraw if a point was really added

  int old_size = this->GetFunctionSize();

  id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  if (old_size != this->GetFunctionSize())
    {
    this->RedrawCanvasElements();
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::AddFunctionPointAtParameter(
  float parameter, int &id)
{
  if (!this->IsCreated() || !this->HasFunction() || this->DisableAddAndRemove)
    {
    return 0;
    }

  // Get the interpolated value

  float rgb[3] = { 1.0, 1.0, 1.0 };
  this->ColorTransferFunction->GetColor(parameter, rgb);

  // Add the point and redraw if a point was really added

  int old_size = this->GetFunctionSize();

  id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  if (old_size != this->GetFunctionSize())
    {
    this->RedrawCanvasElements();
    }

  return 1;
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

  // Since the 'value' range is multi-dimensional (color), just move
  // the point in the 'parameter' range (ignore the 'y')

  float *point = this->ColorTransferFunction->GetDataPointer() + id * 4;
  float parameter = point[0];
  float rgb[3];
  rgb[0] = point[1];
  rgb[1] = point[2];
  rgb[2] = point[3];

  if (!this->FunctionPointParameterIsLocked(id))
    {
    this->ColorTransferFunction->RemovePoint(parameter);
    parameter = (float)((double)x / factors[0]);
    }

  int new_id = this->ColorTransferFunction->AddRGBPoint(
    parameter, rgb[0], rgb[1], rgb[2]);

  // If the point was selected and the new point does not match (which
  // should not happen anyway), reselect the new point

  if (this->SelectedPoint >= 0 && this->SelectedPoint == id && id != new_id)
    {
    this->SelectPoint(new_id);
    }
  else
    {
    this->RedrawCanvasPoint(id);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::RemoveFunctionPoint(int id)
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

  // Remove the point and redraw if a point was really removed

  int old_size = this->GetFunctionSize();

  this->ColorTransferFunction->RemovePoint(
    this->ColorTransferFunction->GetDataPointer()[id * 4]);

  if (old_size != this->GetFunctionSize())
    {
    this->RedrawCanvasElements();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdateInfoLabelWithFunctionPoint(int id)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return;
    }

  float *point = this->ColorTransferFunction->GetDataPointer() + id * 4;
  float parameter = point[0];
  float *rgb = point + 1;

  char format[1024];
  sprintf(format, "%%d: [%%.%df, (%%.2f, %%.2f, %%.2f)]",
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
  this->InfoLabel->SetLabel2(range);
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

  this->ColorSpaceOptionMenu->SetParent(this->InfoFrame);
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
           << " -side left -fill x" << endl;
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
void vtkKWColorTransferFunctionEditor::UpdateInfoLabelWithRange()
{
  if (!this->IsCreated() || !this->InfoLabel || !this->InfoLabel->IsAlive())
    {
    return;
    }

  float *param = GetVisibleParameterRange();
  if (!param)
    {
    return;
    }

  char format[1024];
  sprintf(format, "[%%.%df, %%.%df] x",
          this->ParameterRange->GetEntriesResolution(),
          this->ParameterRange->GetEntriesResolution());

  char range[1024];
  if (this->ColorTransferFunction)
    {
    if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_RGB)
      {
      sprintf(range, format, param[0], param[1], "RGB");
      }
    else if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_HSV)
      {
      sprintf(range, format, param[0], param[1], "HSV");
      }
    else
      {
      sprintf(range, format, param[0], param[1], "Unknown");
      }
    }
  else
    {
    sprintf(range, format, param[0], param[1], "Unknown");
    }

  this->InfoLabel->SetLabel2(range);
  this->InfoLabel->GetLabel()->SetImageDataName(
    this->Icons[ICON_AXES]->GetImageDataName());
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

