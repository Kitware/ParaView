/*=========================================================================

  Module:    vtkKWColorTransferFunctionEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWColorTransferFunctionEditor.h"

#include "vtkColorTransferFunction.h"
#include "vtkKWApplication.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWHistogram.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWRange.h"
#include "vtkKWCanvas.h"
#include "vtkKWTkUtilities.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <vtkstd/string>

vtkStandardNewMacro(vtkKWColorTransferFunctionEditor);
vtkCxxRevisionMacro(vtkKWColorTransferFunctionEditor, "1.19");

#define VTK_KW_CTFE_RGB_LABEL "RGB"
#define VTK_KW_CTFE_HSV_LABEL "HSV"

#define VTK_KW_CTFE_NB_ENTRIES 3

#define VTK_KW_CTFE_COLOR_RAMP_HEIGHT_MIN 2

//----------------------------------------------------------------------------
vtkKWColorTransferFunctionEditor::vtkKWColorTransferFunctionEditor()
{
  this->ColorTransferFunction          = NULL;
  this->ColorRampTransferFunction      = NULL;

  this->ComputePointColorFromValue     = 1;
  this->ComputeHistogramColorFromValue = 0;
  this->ShowValueEntries               = 1;
  this->ShowColorSpaceOptionMenu       = 1;
  this->ShowColorRamp                  = 1;
  this->ColorRampHeight                = 10;
  this->LastRedrawColorRampTime        = 0;
  this->ColorRampPosition              = vtkKWColorTransferFunctionEditor::ColorRampPositionAtDefault;
  this->ColorRampOutlineStyle          = vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSolid;

  this->ColorSpaceOptionMenu           = vtkKWOptionMenu::New();
  this->ColorRamp                      = vtkKWLabel::New();

  int i;
  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->ValueEntries[i] = vtkKWLabeledEntry::New();
    }
}

//----------------------------------------------------------------------------
vtkKWColorTransferFunctionEditor::~vtkKWColorTransferFunctionEditor()
{
  this->SetColorTransferFunction(NULL);
  this->SetColorRampTransferFunction(NULL);

  if (this->ColorSpaceOptionMenu)
    {
    this->ColorSpaceOptionMenu->Delete();
    this->ColorSpaceOptionMenu = NULL;
    }

  if (this->ColorRamp)
    {
    this->ColorRamp->Delete();
    this->ColorRamp = NULL;
    }

  int i;
  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    if (this->ValueEntries[i])
      {
      this->ValueEntries[i]->Delete();
      this->ValueEntries[i] = NULL;
      }
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

  this->LastRedrawFunctionTime = 0;

  // If we are using this function to color the ramp, then reset that time
  // too (otherwise leave that to SetColorRampTransferFunction)

  if (!this->ColorRampTransferFunction)
    {
    this->LastRedrawColorRampTime = 0;
    }

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorRampTransferFunction(
  vtkColorTransferFunction *arg)
{
  if (this->ColorRampTransferFunction == arg)
    {
    return;
    }

  if (this->ColorRampTransferFunction)
    {
    this->ColorRampTransferFunction->UnRegister(this);
    }
    
  this->ColorRampTransferFunction = arg;

  if (this->ColorRampTransferFunction)
    {
    this->ColorRampTransferFunction->Register(this);
    }

  this->Modified();

  this->LastRedrawColorRampTime = 0;

  this->RedrawColorRamp();
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
int vtkKWColorTransferFunctionEditor::GetFunctionPointParameter(
  int id, double *parameter)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  *parameter = this->ColorTransferFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointDimensionality()
{
  return 3;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointValues(
  int id, double *values)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      !values)
    {
    return 0;
    }
  
  int dim = this->GetFunctionPointDimensionality();
  memcpy(values, 
         (this->ColorTransferFunction->GetDataPointer() + id * (1 + dim) + 1), 
         dim * sizeof(double));
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::SetFunctionPointValues(
  int id, const double *values)
{
  double parameter;
  if (!values || !this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }
  
  double clamped_values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  vtkMath::ClampValues(values, this->GetFunctionPointDimensionality(), 
                       this->GetWholeValueRange(), clamped_values);

  this->ColorTransferFunction->AddRGBPoint(
    parameter, clamped_values[0], clamped_values[1], clamped_values[2]);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::InterpolateFunctionPointValues(
  double parameter, double *values)
{
  if (!this->HasFunction() || !values)
    {
    return 0;
    }

  this->ColorTransferFunction->GetColor(parameter, values);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::AddFunctionPoint(
  double parameter, const double *values, int *id)
{
  if (!this->HasFunction() || !values || !id)
    {
    return 0;
    }

  // Clamp

  vtkMath::ClampValue(&parameter, this->GetWholeParameterRange());
  double clamped_values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  vtkMath::ClampValues(values, this->GetFunctionPointDimensionality(),
                       this->GetWholeValueRange(), clamped_values);

  // Add the point
 
  int old_size = this->GetFunctionSize();
  *id = this->ColorTransferFunction->AddRGBPoint(
    parameter, clamped_values[0], clamped_values[1], clamped_values[2]);
  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::SetFunctionPoint(
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
  double clamped_values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  vtkMath::ClampValues(values, this->GetFunctionPointDimensionality(),
                       this->GetWholeValueRange(), clamped_values);
  
  if (parameter != old_parameter)
    {
    this->ColorTransferFunction->RemovePoint(old_parameter);
    }
  int new_id = this->ColorTransferFunction->AddRGBPoint(
    parameter, clamped_values[0], clamped_values[1], clamped_values[2]);
  
  if (new_id != id)
    {
    vtkWarningMacro(<< "Setting a function point (id: " << id << ") parameter/values resulted in a different point (id:" << new_id << "). Inconsistent.");
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::RemoveFunctionPoint(int id)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  // Remove the point

  double parameter = this->ColorTransferFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];

  int old_size = this->GetFunctionSize();
  this->ColorTransferFunction->RemovePoint(parameter);
  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::MoveFunctionPointInColorSpace(
  int id, double parameter, const double *values, int colorspace)
{
  // RGB space is native space, so use the superclass default implem

  if (colorspace == VTK_CTF_RGB)
    {
    return this->Superclass::MoveFunctionPoint(id, parameter, values);
    }

  // Otherwise convert from HSV to RGB

  double rgb[3];
  vtkMath::HSVToRGB(values[0], values[1], values[2], rgb, rgb + 1, rgb + 2);
  return this->Superclass::MoveFunctionPoint(id, parameter, rgb);

  // The old implementation used to work differently: instead of converting
  // the values to RGB space, we would try to stay in HSV space as much as
  // possible, by getting the previous values for that point in HSV space.
  // I don't remember why that choice, it seems to work with the above
  // implem. If the problem comes back, check the CVS.

  /*
    if (colorspace == VTK_CTF_HSV)
    {
    vtkMath::RGBToHSV(old_values, hsv);
    old_values = hsv;
    }

    [...]

    double values_in_rgb[3];
    if (colorspace == VTK_CTF_HSV)
    {
    vtkMath::HSVToRGB(values, values_in_rgb);
    values = values_in_rgb;
    }
  */
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdatePointEntries(
  int id)
{
  this->Superclass::UpdatePointEntries(id);

  if (!this->IsCreated())
    {
    return;
    }

  int i;
  
  // No point ? Empty the entries and disable

  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      this->ValueEntries[i]->GetEntry()->SetValue("");
      this->ValueEntries[i]->SetEnabled(0);
      }
    return;
    }

  // Disable entries if value is locked

  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->ValueEntries[i]->SetEnabled(
      this->FunctionPointValueIsLocked(id) ? 0 : this->Enabled);
    }

  // Get the values in the right color space

  double *point = this->ColorTransferFunction->GetDataPointer() + id * 4;

  double *values = point + 1, hsv[3];
  if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_HSV)
    {
    vtkMath::RGBToHSV(values, hsv);
    values = hsv;
    }

  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->ValueEntries[i]->GetEntry()->SetValue(values[i], 2);
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdatePointEntriesLabel()
{
  if (VTK_KW_CTFE_NB_ENTRIES != 3 ||
      !this->ColorTransferFunction ||
      (this->ColorTransferFunction->GetColorSpace() != VTK_CTF_HSV &&
       this->ColorTransferFunction->GetColorSpace() != VTK_CTF_RGB))
    {
    for (int i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      this->ValueEntries[i]->SetLabel("");
      }
    return;
    }

  if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_HSV)
    {
    this->ValueEntries[0]->SetLabel("H:");
    this->ValueEntries[1]->SetLabel("S:");
    this->ValueEntries[2]->SetLabel("V:");
    }
  else if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_RGB)
    {
    this->ValueEntries[0]->SetLabel("R:");
    this->ValueEntries[1]->SetLabel("G:");
    this->ValueEntries[2]->SetLabel("B:");
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdateColorSpaceOptionMenu()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->ColorSpaceOptionMenu && this->ColorTransferFunction)
    {
    switch (this->ColorTransferFunction->GetColorSpace())
      {
      case VTK_CTF_HSV:
        this->ColorSpaceOptionMenu->SetValue(VTK_KW_CTFE_HSV_LABEL);
        break;
      default:
      case VTK_CTF_RGB:
        this->ColorSpaceOptionMenu->SetValue(VTK_KW_CTFE_RGB_LABEL);
        break;
      }
    }
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

  if (this->ShowColorSpaceOptionMenu)
    {
    this->CreateColorSpaceOptionMenu(app);
    }

  // Create the value entries

  if (this->ShowValueEntries)
    {
    this->CreateValueEntries(app);
    }

  // Create the ramp

  if (this->ShowColorRamp)
    {
    this->CreateColorRamp(app);
    }

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateColorSpaceOptionMenu(
  vtkKWApplication *app)
{
  if (this->ColorSpaceOptionMenu && !this->ColorSpaceOptionMenu->IsCreated())
    {
    this->CreateTopLeftFrame(app);
    this->ColorSpaceOptionMenu->SetParent(this->TopLeftFrame);
    this->ColorSpaceOptionMenu->Create(app, "-padx 1 -pady 0");
    this->ColorSpaceOptionMenu->IndicatorOff();
    this->ColorSpaceOptionMenu->SetBalloonHelpString(
      "Change the interpolation color space to RGB or HSV.");

    char callback[128];
    sprintf(callback, "ColorSpaceCallback %d", VTK_CTF_RGB);
    this->ColorSpaceOptionMenu->AddEntryWithCommand(
      VTK_KW_CTFE_RGB_LABEL, this, callback);
    
    sprintf(callback, "ColorSpaceCallback %d", VTK_CTF_HSV);
    this->ColorSpaceOptionMenu->AddEntryWithCommand(
      VTK_KW_CTFE_HSV_LABEL, this, callback);

    this->UpdateColorSpaceOptionMenu();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateColorRamp(
  vtkKWApplication *app)
{
  if (this->ColorRamp && !this->ColorRamp->IsCreated())
    {
    this->ColorRamp->SetParent(this);
    this->ColorRamp->Create(app, "-bd 0 -anchor nw -highlightthickness 0");
    if (!this->IsColorRampUpToDate())
      {
      this->RedrawColorRamp();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateValueEntries(
  vtkKWApplication *app)
{
  if (this->ValueEntries[0] && !this->ValueEntries[0]->IsCreated())
    {
    this->CreateTopRightFrame(app);
    int i;
    for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      this->ValueEntries[i]->SetParent(this->TopRightFrame);
      this->ValueEntries[i]->Create(app, "");
      this->ValueEntries[i]->GetEntry()->SetWidth(4);
      this->ValueEntries[i]->GetEntry()->BindCommand(
        this, "ValueEntriesCallback");
      }

    this->UpdatePointEntriesLabel();
    this->UpdatePointEntries(this->SelectedPoint);
    }
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::IsTopLeftFrameUsed()
{
  return (this->Superclass::IsTopLeftFrameUsed() || 
          this->ShowColorSpaceOptionMenu);
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::IsTopRightFrameUsed()
{
  return (this->Superclass::IsTopRightFrameUsed() || 
          this->ShowValueEntries);
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

  // Add the color space menu (in top left frame)

  if (this->ShowColorSpaceOptionMenu && 
      this->ColorSpaceOptionMenu->IsCreated())
    {
    tk_cmd << "pack " << this->ColorSpaceOptionMenu->GetWidgetName() 
           << " -side left -fill both -padx 0" << endl;
    }

  // Value entries (in top right frame)

  if (this->ShowValueEntries)
    {
    int i;
    for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      if (this->ValueEntries[i]->IsCreated())
        {
        tk_cmd << "pack " << this->ValueEntries[i]->GetWidgetName() 
               << " -side left -pady 0" << endl;
        }
      }
    }

  // Color ramp

  if (this->ShowColorRamp && 
      (this->ColorRampPosition == 
       vtkKWColorTransferFunctionEditor::ColorRampPositionAtDefault) &&
      this->ColorRamp->IsCreated())
    {
    // Get the current position of the parameter range, and move it one
    // row below. Otherwise get the current number of rows and insert
    // the ramp at the end

    int col, row, nb_cols;
    if (this->ShowParameterRange && 
        this->ParameterRange->IsCreated() &&
        vtkKWTkUtilities::GetGridPosition(
          this->GetApplication()->GetMainInterp(), 
          this->ParameterRange->GetWidgetName(), 
          &col, &row))
      {
      tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
             << " -row " << row + 1 << endl;
      }
    else
      {
      col = 2;
      if (!vtkKWTkUtilities::GetGridSize(
            this->GetApplication()->GetMainInterp(), 
            this->ColorRamp->GetParent()->GetWidgetName(), 
            &nb_cols, &row))
        {
        row = 2 + (this->ShowParameterTicks ? 1 : 0);
        }
      }
    tk_cmd << "grid " << this->ColorRamp->GetWidgetName() 
           << " -columnspan 2 -sticky w -pady 2 -padx 0 "
           << " -column " << col << " -row " << row << endl;
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
  // And the entries

  this->UpdateColorSpaceOptionMenu();

  this->UpdatePointEntriesLabel();

  // No selection, disable value entries

  int i;
  if (!this->HasSelection())
    {
    for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      this->ValueEntries[i]->SetEnabled(0);
      }
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

  int i;
  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    if (this->ValueEntries[i])
      {
      this->ValueEntries[i]->SetEnabled(this->Enabled);
      }
    }

  if (this->ColorRamp)
    {
    this->ColorRamp->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::ColorSpaceCallback(int v)
{
  if (this->ColorTransferFunction &&
      this->ColorTransferFunction->GetColorSpace() != v)
    {
    this->ColorTransferFunction->SetColorSpace(v);
    this->Update();
    if (this->HasSelection())
      {
      this->UpdatePointEntries(this->SelectedPoint);
      }
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::ValueEntriesCallback()
{
  if (!this->HasSelection())
    {
    return;
    }

  double parameter;
  if (!this->GetFunctionPointParameter(this->GetSelectedPoint(), &parameter))
    {
    return;
    }

  // Get the values from the entries

  double values[VTK_KW_CTFE_NB_ENTRIES];

  int i;
  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    if (!this->ValueEntries[i])
      {
      return;
      }
    values[i] = this->ValueEntries[i]->GetEntry()->GetValueAsFloat();
    }

  // Move the point, check if something has really been moved

  unsigned long mtime = this->GetFunctionMTime();

  this->MoveFunctionPointInColorSpace(
    this->GetSelectedPoint(), 
    parameter, values, this->ColorTransferFunction->GetColorSpace());
  
  if (this->GetFunctionMTime() > mtime)
    {
    this->InvokePointMovedCommand(this->SelectedPoint);
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetShowColorSpaceOptionMenu(int arg)
{
  if (this->ShowColorSpaceOptionMenu == arg)
    {
    return;
    }

  this->ShowColorSpaceOptionMenu = arg;

  // Make sure that if the button has to be shown, we create it on the fly if
  // needed

  if (this->ShowColorSpaceOptionMenu && this->IsCreated())
    {
    this->CreateColorSpaceOptionMenu(this->GetApplication());
    }

  this->UpdateColorSpaceOptionMenu();

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetShowColorRamp(int arg)
{
  if (this->ShowColorRamp == arg)
    {
    return;
    }
    
  this->ShowColorRamp = arg;

  // Make sure that if the ramp has to be shown, we create it on the fly if
  // needed.

  if (this->ShowColorRamp)
    {
    if (this->IsCreated() && !this->ColorRamp->IsCreated())
      {
      this->CreateColorRamp(this->GetApplication());
      }
    }

  this->RedrawColorRamp(); // if we are hidding it, need to update in canvas

  this->Pack();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorRampPosition(int arg)
{
  if (arg < vtkKWColorTransferFunctionEditor::ColorRampPositionAtDefault)
    {
    arg = vtkKWColorTransferFunctionEditor::ColorRampPositionAtDefault;
    }
  else if (arg > 
           vtkKWColorTransferFunctionEditor::ColorRampPositionAtCanvas)
    {
    arg = vtkKWColorTransferFunctionEditor::ColorRampPositionAtCanvas;
    }

  if (this->ColorRampPosition == arg)
    {
    return;
    }

  // If the ramp was drawn in the canvas before, make sure we remove it now

  if (this->ColorRampPosition == 
      vtkKWColorTransferFunctionEditor::ColorRampPositionAtCanvas)
    {
    this->CanvasRemoveTag(VTK_KW_CTFE_COLOR_RAMP_TAG);
    }

  this->ColorRampPosition = arg;

  this->Modified();

  this->RedrawColorRamp();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorRampOutlineStyle(int arg)
{
  if (arg < vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleNone)
    {
    arg = vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleNone;
    }
  else if (arg > 
           vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSunken)
    {
    arg = vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSunken;
    }

  if (this->ColorRampOutlineStyle == arg)
    {
    return;
    }

  this->ColorRampOutlineStyle = arg;

  this->Modified();

  this->RedrawColorRamp();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorRampHeight(int arg)
{
  if (this->ColorRampHeight == arg || 
      arg < VTK_KW_CTFE_COLOR_RAMP_HEIGHT_MIN)
    {
    return;
    }
    
  this->ColorRampHeight = arg;

  this->RedrawColorRamp();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetShowValueEntries(int arg)
{
  if (this->ShowValueEntries == arg)
    {
    return;
    }

  this->ShowValueEntries = arg;

  // Make sure that if the entries have to be shown, we create it on the fly if
  // needed

  if (this->ShowValueEntries && this->IsCreated())
    {
    this->CreateValueEntries(this->GetApplication());
    }

  this->UpdatePointEntriesLabel();
  this->UpdatePointEntries(this->SelectedPoint);

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetPointColorAsRGB(int id, double rgb[3])
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }

  this->ColorTransferFunction->GetColor(parameter, rgb);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::SetPointColorAsRGB(
  int id, const double rgb[3])
{
  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }

  return this->MoveFunctionPointInColorSpace(id, parameter, rgb, VTK_CTF_RGB);
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetPointColorAsHSV(int id, double hsv[3])
{
  double rgb[3];
  if (!this->GetPointColorAsRGB(id, rgb))
    {
    return 0;
    }

  vtkMath::RGBToHSV(rgb, hsv);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::SetPointColorAsHSV(
  int id, const double hsv[3])
{
  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }

  return this->MoveFunctionPointInColorSpace(id, parameter, hsv, VTK_CTF_HSV);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::Redraw()
{
  this->Superclass::Redraw();

  // In any cases, check if we have to redraw the color ramp
  // Since the color ramp uses a color function that might not
  // be the color function being edited, we need to check.

  if (!this->IsColorRampUpToDate())
    {
    this->RedrawColorRamp();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::RedrawSizeDependentElements()
{
  this->Superclass::RedrawSizeDependentElements();

  this->RedrawColorRamp();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::RedrawPanOnlyDependentElements()
{
  this->Superclass::RedrawPanOnlyDependentElements();

  this->RedrawColorRamp();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::RedrawFunctionDependentElements()
{
  this->Superclass::RedrawFunctionDependentElements();

  // This method is called each time the color tfunc has changed
  // but since the histogram depends on the point too if we color by values
  // the update the histogram

  if (this->Histogram && this->ComputeHistogramColorFromValue)
    {
    this->RedrawHistogram();
    }

  // The color ramp may (or may not) have to be redrawn, depending on which
  // color tfunc is used in the ramp, so it is also catched by Redraw()
  // and in this method (which can be called independently.

  if (!this->IsColorRampUpToDate())
    {
    this->RedrawColorRamp();
    }
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::IsColorRampUpToDate()
{
  // Which function to use ?

  vtkColorTransferFunction *func = 
    this->ColorRampTransferFunction ? 
    this->ColorRampTransferFunction : this->ColorTransferFunction;

  return (func &&
          this->ShowColorRamp &&
          this->LastRedrawColorRampTime < func->GetMTime()) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::RedrawColorRamp()
{
  if (!this->ColorRamp->IsCreated() || 
      !this->HasFunction() ||
      this->DisableRedraw)
    {
    return;
    }

  double p_v_range_ext[2];

  if (this->ShowColorRamp)
    {
    // Which function to use ?

    vtkColorTransferFunction *func = 
      this->ColorRampTransferFunction ? 
      this->ColorRampTransferFunction : this->ColorTransferFunction;

    this->LastRedrawColorRampTime = func->GetMTime();

    int bounds[2], margins[2];
    this->GetCanvasHorizontalSlidingBounds(p_v_range_ext, bounds, margins);
    
    int in_canvas = 
      (this->ColorRampPosition == 
       vtkKWColorTransferFunctionEditor::ColorRampPositionAtCanvas);

    // Create the image buffer

    int img_width = bounds[1] - bounds[0] + 1;
    int img_height = this->ColorRampHeight;
    int img_offset_x = 0;

    int i;
    unsigned char *img_ptr;

    // Allocate the image
    // If ramp below the canvas as a label, we have to manually create a margin
    // on the left of the image to align it properly, as this can not 
    // be done properly with -padx on the label 

    int table_width = img_width;
    if (!in_canvas)
      {
      img_width += margins[0];
      img_offset_x = margins[0];
      }

    unsigned char *img_buffer = new unsigned char [img_width * img_height * 3];

    // Get the LUT for the parameter range and copy it in the first row

    double *table = new double[table_width * 3];
    func->GetTable(p_v_range_ext[0], p_v_range_ext[1], table_width, table);

    double *table_ptr = table;
    img_ptr = img_buffer + img_offset_x * 3;
    for (i = 0; i < table_width * 3; i++)
      {
      *img_ptr++ = (unsigned char)(255.0 * *table_ptr++);
      }

    // If ramp below the canvas as a label, fill the margin with
    // background color on the first row

    if (!in_canvas)
      {
      int bg_r, bg_g, bg_b;
      this->ColorRamp->GetBackgroundColor(&bg_r, &bg_g, &bg_b);
      img_ptr = img_buffer;
      for (i = 0; i < img_offset_x; i++)
        {
        *img_ptr++ = (unsigned char)bg_r;
        *img_ptr++ = (unsigned char)bg_g;
        *img_ptr++ = (unsigned char)bg_b;
        }
      }

    // Insert the outline border on that first row if needed

    unsigned char bg_rgb[3], ds_rgb[3], ls_rgb[3], hl_rgb[3];

    if (this->ColorRampOutlineStyle == 
        vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSolid)
      {
      img_ptr = img_buffer + img_offset_x * 3;
      *img_ptr++ = 0; *img_ptr++ = 0; *img_ptr++ = 0;
      img_ptr = img_buffer + (img_width - 1) * 3;
      *img_ptr++ = 0; *img_ptr++ = 0; *img_ptr++ = 0;
      }
    else if (this->ColorRampOutlineStyle == 
             vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSunken)
      {
      /* 
         DDDDDDDDDDDDDDDDH <- B
         DLLLLLLLLLLLLLLBH <- C
         DL.............BH
         DL.............BH <- A
         DBBBBBBBBBBBBBBBH <- D 
         HHHHHHHHHHHHHHHHH <- E
      */
      // Sunken Outline: Part A
      this->GetColorRampOutlineSunkenColors(bg_rgb, ds_rgb, ls_rgb, hl_rgb);
      img_ptr = img_buffer + img_offset_x * 3;
      *img_ptr++ = ds_rgb[0]; *img_ptr++ = ds_rgb[1]; *img_ptr++ = ds_rgb[2];
      *img_ptr++ = ls_rgb[0]; *img_ptr++ = ls_rgb[1]; *img_ptr++ = ls_rgb[2];
      img_ptr = img_buffer + (img_width - 2) * 3;
      *img_ptr++ = bg_rgb[0]; *img_ptr++ = bg_rgb[1]; *img_ptr++ = bg_rgb[2];
      *img_ptr++ = hl_rgb[0]; *img_ptr++ = hl_rgb[1]; *img_ptr++ = hl_rgb[2];
      }

    // Replicate the first row to all other rows

    img_ptr = img_buffer + img_width * 3;
    for (i = 1; i < img_height; i++)
      {
      memcpy(img_ptr, img_buffer, img_width * 3);
      img_ptr += img_width * 3;
      }

    // Complete the outline top/bottom

    if (this->ColorRampOutlineStyle == 
        vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSolid)
      {
      memset(img_buffer + img_offset_x * 3, 0, table_width * 3);
      memset(img_buffer + (img_width * (img_height - 1) + img_offset_x) * 3, 
             0, table_width * 3);
      }
    else if (this->ColorRampOutlineStyle == 
             vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSunken)
      {
      // Sunken Outline: Part B
      img_ptr = img_buffer + img_offset_x * 3;
      for (i = 0; i < table_width - 1; i++)
        {
        *img_ptr++ = ds_rgb[0]; *img_ptr++ = ds_rgb[1]; *img_ptr++ = ds_rgb[2];
        }
      *img_ptr++ = hl_rgb[0]; *img_ptr++ = hl_rgb[1]; *img_ptr++ = hl_rgb[2];

      // Sunken Outline: Part C
      img_ptr = img_buffer + (img_width + img_offset_x + 1) * 3;
      for (i = 0; i < table_width - 3; i++)
        {
        *img_ptr++ = ls_rgb[0]; *img_ptr++ = ls_rgb[1]; *img_ptr++ = ls_rgb[2];
        }
      *img_ptr++ = bg_rgb[0]; *img_ptr++ = bg_rgb[1]; *img_ptr++ = bg_rgb[2];

      // Sunken Outline: Part D
      img_ptr = img_buffer + (img_width * (img_height - 2) + img_offset_x+1)*3;
      for (i = 0; i < table_width - 2; i++)
        {
        *img_ptr++ = bg_rgb[0]; *img_ptr++ = bg_rgb[1]; *img_ptr++ = bg_rgb[2];
        }

      // Sunken Outline: Part E
      img_ptr = img_buffer + (img_width * (img_height - 1) + img_offset_x) * 3;
      for (i = 0; i < table_width; i++)
        {
        *img_ptr++ = hl_rgb[0]; *img_ptr++ = hl_rgb[1]; *img_ptr++ = hl_rgb[2];
        }
      }

    // Update the image

    this->ColorRamp->SetImageOption(
      img_buffer, img_width, img_height, 3);

    delete [] img_buffer;
    delete [] table;
    }

  // If the ramp has to be in the canvas, draw it now or remove it

  if (this->ColorRampPosition == 
      vtkKWColorTransferFunctionEditor::ColorRampPositionAtCanvas &&
      this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();
    ostrstream tk_cmd;

    // Create/remove the image item in the canvas only when needed

    int has_tag = this->CanvasHasTag(VTK_KW_CTFE_COLOR_RAMP_TAG);
    if (!has_tag)
      {
      if (this->ShowColorRamp)
        {
        vtkstd::string image_name(
          this->Script("%s cget -image", this->ColorRamp->GetWidgetName()));
        tk_cmd << canv << " create image 0 0 -anchor nw "
               << " -image " << image_name.c_str() 
               << " -tags {" << VTK_KW_CTFE_COLOR_RAMP_TAG << "}" << endl;
        tk_cmd << canv << " lower " << VTK_KW_CTFE_COLOR_RAMP_TAG
               << " {" << VTK_KW_PVFE_FUNCTION_TAG << "}" << endl;
        }
      }
    else 
      {
      if (!this->ShowColorRamp)
        {
        tk_cmd << canv << " delete " << VTK_KW_CTFE_COLOR_RAMP_TAG << endl;
        }
      }

    // Update coordinates

    if (this->ShowColorRamp)
      {
      double factors[2] = {0.0, 0.0};
      this->GetCanvasScalingFactors(factors);

      double *v_v_range = this->GetVisibleValueRange();
      double *v_w_range = this->GetWholeValueRange();

      double c_y = ceil(
        (v_w_range[1] - (v_v_range[1] + v_v_range[0]) * 0.5) * factors[1]
        - (double)this->ColorRampHeight * 0.5);
      
      tk_cmd << canv << " coords " << VTK_KW_CTFE_COLOR_RAMP_TAG 
             << " " << p_v_range_ext[0] * factors[0] << " " << c_y << endl;
      }

    tk_cmd << ends;
    this->Script(tk_cmd.str());
    tk_cmd.rdbuf()->freeze(0);
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::GetColorRampOutlineSunkenColors(
  unsigned char bg_rgb[3], 
  unsigned char ds_rgb[3], 
  unsigned char ls_rgb[3],
  unsigned char hl_rgb[3])
{
  if (!this->ColorRamp || !this->ColorRamp->IsCreated())
    {
    return;
    }

  int r, g, b;
  double fr, fg, fb;
  double fh, fs, fv;
  
  this->ColorRamp->GetBackgroundColor(&r, &g, &b);

  bg_rgb[0] = (unsigned char)r;
  bg_rgb[1] = (unsigned char)g;
  bg_rgb[2] = (unsigned char)b;

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

  vtkMath::HSVToRGB(fh, fs, fv * 0.3, &fr, &fg, &fb);
  ds_rgb[0] = (unsigned char)(fr * 255.0);
  ds_rgb[1] = (unsigned char)(fg * 255.0);
  ds_rgb[2] = (unsigned char)(fb * 255.0);

  vtkMath::HSVToRGB(fh, fs, fv * 0.6, &fr, &fg, &fb);
  ls_rgb[0] = (unsigned char)(fr * 255.0);
  ls_rgb[1] = (unsigned char)(fg * 255.0);
  ls_rgb[2] = (unsigned char)(fb * 255.0);

  vtkMath::HSVToRGB(fh, fs, 1.0, &fr, &fg, &fb);
  hl_rgb[0] = (unsigned char)(fr * 255.0);
  hl_rgb[1] = (unsigned char)(fg * 255.0);
  hl_rgb[2] = (unsigned char)(fb * 255.0);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdateHistogramImageDescriptor(
  vtkKWHistogram::ImageDescriptor *desc)
{
  this->Superclass::UpdateHistogramImageDescriptor(desc);

  if (this->ComputeHistogramColorFromValue)
    {
    desc->ColorTransferFunction = 
      this->ColorRampTransferFunction 
      ? this->ColorRampTransferFunction : this->ColorTransferFunction;
    desc->DrawGrid = 1;
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowValueEntries: "
     << (this->ShowValueEntries ? "On" : "Off") << endl;

  os << indent << "ShowColorSpaceOptionMenu: "
     << (this->ShowColorSpaceOptionMenu ? "On" : "Off") << endl;

  os << indent << "ShowColorRamp: "
     << (this->ShowColorRamp ? "On" : "Off") << endl;

  os << indent << "ColorRampHeight: " << this->ColorRampHeight << endl;
  os << indent << "ColorRampPosition: " << this->ColorRampPosition << endl;
  os << indent << "ColorRampOutlineStyle: " << this->ColorRampOutlineStyle << endl;

  os << indent << "ColorTransferFunction: ";
  if (this->ColorTransferFunction)
    {
    os << endl;
    this->ColorTransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "ColorRampTransferFunction: ";
  if (this->ColorRampTransferFunction)
    {
    os << endl;
    this->ColorRampTransferFunction->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "ColorSpaceOptionMenu: ";
  if (this->ColorSpaceOptionMenu)
    {
    os << endl;
    this->ColorSpaceOptionMenu->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  int i;
  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    os << indent << "ValueEntries[" << i << "]: ";
    if (this->ValueEntries[i])
      {
      os << endl;
      this->ValueEntries[i]->PrintSelf(os, indent.GetNextIndent());
      }
    else
      {
      os << "None" << endl;
      }
    }
}

