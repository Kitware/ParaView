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
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWHistogram.h"
#include "vtkKWLabel.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWInternationalization.h"
#include "vtkKWScaleWithEntry.h"
#include "vtkKWMenu.h"
#include "vtkKWMenuButton.h"
#include "vtkKWRange.h"
#include "vtkKWCanvas.h"
#include "vtkKWTkUtilities.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/string>

vtkStandardNewMacro(vtkKWColorTransferFunctionEditor);
vtkCxxRevisionMacro(vtkKWColorTransferFunctionEditor, "1.53");

#define VTK_KW_CTFE_COLOR_RAMP_TAG "color_ramp_tag"

#define VTK_KW_CTFE_NB_ENTRIES 3

#define VTK_KW_CTFE_COLOR_RAMP_HEIGHT_MIN 2

//----------------------------------------------------------------------------
vtkKWColorTransferFunctionEditor::vtkKWColorTransferFunctionEditor()
{
  this->ColorTransferFunction          = NULL;
  this->ColorRampTransferFunction      = NULL;

  this->ComputePointColorFromValue     = 1;
  this->ComputeHistogramColorFromValue = 0;
  this->ValueEntriesVisibility               = 1;
  this->ColorSpaceOptionMenuVisibility       = 1;
  this->ColorRampVisibility                  = 1;
  this->ColorRampHeight                = 10;
  this->LastRedrawColorRampTime        = 0;
  this->ColorRampPosition              = vtkKWColorTransferFunctionEditor::ColorRampPositionDefault;
  this->ColorRampOutlineStyle          = vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSolid;

  this->ColorSpaceOptionMenu           = vtkKWMenuButton::New();
  this->ColorRamp                      = vtkKWLabel::New();

  int i;
  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->ValueEntries[i] = vtkKWEntryWithLabel::New();
    }

  this->ValueRangeVisibilityOff();
}

//----------------------------------------------------------------------------
vtkKWColorTransferFunctionEditor::~vtkKWColorTransferFunctionEditor()
{
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

  this->SetColorTransferFunction(NULL);
  this->SetColorRampTransferFunction(NULL);
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

  this->LastRedrawFunctionTime = 0;

  // If we are using this function to color the ramp, then reset that time
  // too (otherwise leave that to SetColorRampTransferFunction)

  if (!this->ColorRampTransferFunction)
    {
    this->LastRedrawColorRampTime = 0;
    }

  if (this->ColorTransferFunction)
    {
    this->ColorTransferFunction->Register(this);
    this->SetWholeParameterRangeToFunctionRange();
    }

  this->Modified();

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

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  *parameter = node_value[0];
#else
  *parameter = this->ColorTransferFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
#endif
  
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

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  memcpy(values, node_value + 1, dim * sizeof(double));
#else
  memcpy(values, 
         (this->ColorTransferFunction->GetDataPointer() + id * (1 + dim) + 1), 
         dim * sizeof(double));
#endif
  
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
  
  // Clamp

  double clamped_values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  vtkMath::ClampValues(values, this->GetFunctionPointDimensionality(), 
                       this->GetWholeValueRange(), clamped_values);

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
#endif

  this->ColorTransferFunction->AddRGBPoint(
    parameter, 
    clamped_values[0], clamped_values[1], clamped_values[2]
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
    , node_value[4], node_value[5]
#endif
    );
  
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
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  if (this->GetFunctionPointId(parameter, id))
    {
    double node_value[6];
    this->ColorTransferFunction->GetNodeValue(*id, node_value);
    *id = this->ColorTransferFunction->AddRGBPoint(
      parameter, 
      clamped_values[0], clamped_values[1], clamped_values[2],
      node_value[4], node_value[5]);
    }
  else
#endif
    {
    *id = this->ColorTransferFunction->AddRGBPoint(
      parameter, clamped_values[0], clamped_values[1], clamped_values[2]);
    }
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

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
#endif

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
    parameter, 
    clamped_values[0], clamped_values[1], clamped_values[2]
#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
    , node_value[4], node_value[5]
#endif
    );
  
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

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  double parameter = node_value[0];
#else
  double parameter = this->ColorTransferFunction->GetDataPointer()[
    id * (1 + this->GetFunctionPointDimensionality())];
#endif

  int old_size = this->GetFunctionSize();
  this->ColorTransferFunction->RemovePoint(parameter);
  return (old_size != this->GetFunctionSize());
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointMidPoint(
  int id, double *pos)
{
  if (id < 0 || id >= this->GetFunctionSize() || !pos)
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  *pos = node_value[4];
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::SetFunctionPointMidPoint(
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
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  this->ColorTransferFunction->AddRGBPoint(
    node_value[0], 
    node_value[1], node_value[2], node_value[3], 
    pos, node_value[5]);
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::GetFunctionPointSharpness(
  int id, double *sharpness)
{
  if (id < 0 || id >= this->GetFunctionSize() || !sharpness)
    {
    return 0;
    }

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  *sharpness = node_value[5];
  return 1;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::SetFunctionPointSharpness(
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
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
  this->ColorTransferFunction->AddRGBPoint(
    node_value[0], 
    node_value[1], node_value[2], node_value[3], 
    node_value[4], sharpness);
  return 1;
#else
  return 0;
#endif
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
      if (this->ValueEntries[i])
        {
        this->ValueEntries[i]->GetWidget()->SetValue("");
        this->ValueEntries[i]->SetEnabled(0);
        }
      }
    return;
    }

  // Disable entries if value is locked

  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->ValueEntries[i]->SetEnabled(
      this->FunctionPointValueIsLocked(id) ? 0 : this->GetEnabled());
    }

  // Get the values in the right color space

#if VTK_MAJOR_VERSION > 5 || (VTK_MAJOR_VERSION == 5 && VTK_MINOR_VERSION > 0)
  double node_value[6];
  this->ColorTransferFunction->GetNodeValue(id, node_value);
#else
  double *node_value = this->ColorTransferFunction->GetDataPointer() + id * 4;
#endif

  double *values = node_value + 1, hsv[3];
  if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_HSV)
    {
    vtkMath::RGBToHSV(values, hsv);
    values = hsv;
    }

  for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->ValueEntries[i]->GetWidget()->SetValueAsFormattedDouble(
      values[i], 2);
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
      if (this->ValueEntries[i])
        {
        this->ValueEntries[i]->GetLabel()->SetText("");
        }
      }
    return;
    }

  if (this->ColorTransferFunction)
    {
    if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_HSV)
      {
      if (this->ValueEntries[0])
        {
        this->ValueEntries[0]->GetLabel()->SetText(ks_("Color Space|Hue|H:"));
        }
      if (this->ValueEntries[1])
        {
        this->ValueEntries[1]->GetLabel()->SetText(ks_("Color Space|Saturation|S:"));
        }
      if (this->ValueEntries[2])
        {
        this->ValueEntries[2]->GetLabel()->SetText(ks_("Color Space|Value|V:"));
        }
      }
    else if (this->ColorTransferFunction->GetColorSpace() == VTK_CTF_RGB)
      {
      if (this->ValueEntries[0])
        {
        this->ValueEntries[0]->GetLabel()->SetText(ks_("Color Space|Red|R:"));
        }
      if (this->ValueEntries[1])
        {
        this->ValueEntries[1]->GetLabel()->SetText(ks_("Color Space|Green|G:"));
        }
      if (this->ValueEntries[2])
        {
        this->ValueEntries[2]->GetLabel()->SetText(ks_("Color Space|Blue|B:"));
        }
      }
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
        if (this->ColorTransferFunction->GetHSVWrap())
          this->ColorSpaceOptionMenu->SetValue(ks_("Color Space|HSV"));
        else
          this->ColorSpaceOptionMenu->SetValue(ks_("Color Space|HSV (2)"));
        break;
      default:
      case VTK_CTF_RGB:
        this->ColorSpaceOptionMenu->SetValue(ks_("Color Space|RGB"));
        break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("ColorTransferFunctionEditor already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // Add the color space option menu

  if (this->ColorSpaceOptionMenuVisibility)
    {
    this->CreateColorSpaceOptionMenu();
    }

  // Create the value entries

  if (this->ValueEntriesVisibility && this->PointEntriesVisibility)
    {
    this->CreateValueEntries();
    }

  // Create the ramp

  if (this->ColorRampVisibility)
    {
    this->CreateColorRamp();
    }

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateColorSpaceOptionMenu()
{
  if (this->ColorSpaceOptionMenu && !this->ColorSpaceOptionMenu->IsCreated())
    {
    this->CreateTopLeftFrame();
    this->ColorSpaceOptionMenu->SetParent(this->TopLeftFrame);
    this->ColorSpaceOptionMenu->Create();
    this->ColorSpaceOptionMenu->SetPadX(1);
    this->ColorSpaceOptionMenu->SetPadY(1);
    this->ColorSpaceOptionMenu->IndicatorVisibilityOff();
    this->ColorSpaceOptionMenu->SetBalloonHelpString(
      k_("Change the interpolation color space to RGB or HSV."));

    const char callback[] = "ColorSpaceCallback";

    vtkKWMenu *menu = this->ColorSpaceOptionMenu->GetMenu();
    menu->AddRadioButton(ks_("Color Space|RGB"), this, callback);
    menu->AddRadioButton(ks_("Color Space|HSV"), this, callback);
    menu->AddRadioButton(ks_("Color Space|HSV (2)"), this, callback);

    this->UpdateColorSpaceOptionMenu();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateColorRamp()
{
  if (this->ColorRamp && !this->ColorRamp->IsCreated())
    {
    this->ColorRamp->SetParent(this);
    this->ColorRamp->Create();
    this->ColorRamp->SetBorderWidth(0);
    this->ColorRamp->SetAnchorToNorthWest();
    if (!this->IsColorRampUpToDate())
      {
      this->RedrawColorRamp();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::CreateValueEntries()
{
  if (this->ValueEntries[0] && !this->ValueEntries[0]->IsCreated())
    {
    this->CreatePointEntriesFrame();
    int i;
    for (i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      this->ValueEntries[i]->SetParent(this->PointEntriesFrame);
      this->ValueEntries[i]->Create();
      this->ValueEntries[i]->GetWidget()->SetWidth(4);
      this->ValueEntries[i]->GetWidget()->SetCommand(
        this, "ValueEntriesCallback");
      }

    this->UpdatePointEntriesLabel();
    this->UpdatePointEntries(this->GetSelectedPoint());
    }
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::IsTopLeftFrameUsed()
{
  return (this->Superclass::IsTopLeftFrameUsed() || 
          this->ColorSpaceOptionMenuVisibility);
}

//----------------------------------------------------------------------------
int vtkKWColorTransferFunctionEditor::IsPointEntriesFrameUsed()
{
  return (this->Superclass::IsPointEntriesFrameUsed() || 
          (this->PointEntriesVisibility && this->ValueEntriesVisibility));
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

  if (this->ColorSpaceOptionMenuVisibility && 
      this->ColorSpaceOptionMenu && this->ColorSpaceOptionMenu->IsCreated())
    {
    tk_cmd << "pack " << this->ColorSpaceOptionMenu->GetWidgetName() 
           << " -side left -fill both -padx 0" << endl;
    }

  // Color ramp

  if (this->ColorRampVisibility && 
      (this->ColorRampPosition == 
       vtkKWColorTransferFunctionEditor::ColorRampPositionDefault) &&
      this->ColorRamp && this->ColorRamp->IsCreated())
    {
    // Get the current position of the parameter range, and move it one
    // row below. Otherwise get the current number of rows and insert
    // the ramp at the end

    int show_pr = 
      (this->ParameterRangeVisibility && 
       this->ParameterRange && this->ParameterRange->IsCreated()) ? 1 : 0;

    int col, row, nb_cols;
    if (show_pr &&
        (this->ParameterRangePosition == 
         vtkKWParameterValueFunctionEditor::ParameterRangePositionBottom) &&
        vtkKWTkUtilities::GetWidgetPositionInGrid(
          this->ParameterRange, &col, &row))
      {
      tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
             << " -row " << row + 1 << endl;
      }
    else
      {
      col = 2;
      if (!vtkKWTkUtilities::GetGridSize(
            this->ColorRamp->GetParent(), &nb_cols, &row))
        {
        row = 2 + (this->ParameterTicksVisibility ? 1 : 0) + 
          (show_pr &&
           (this->ParameterRangePosition == 
            vtkKWParameterValueFunctionEditor::ParameterRangePositionTop) 
           ? 1 :0 );
        }
      }
    tk_cmd << "grid " << this->ColorRamp->GetWidgetName() 
           << " -columnspan 2 -sticky w -padx 0 "
           << " -pady " << (this->CanvasVisibility ? 2 : 0)
           << " -column " << col << " -row " << row << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::PackPointEntries()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Pack the other entries

  this->Superclass::PackPointEntries();

  ostrstream tk_cmd;

  // Value entries (in top right frame)

  if (this->HasSelection() &&
      this->ValueEntriesVisibility && 
      this->PointEntriesVisibility)
    {
    for (int i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      if (this->ValueEntries[i] && this->ValueEntries[i]->IsCreated())
        {
        tk_cmd << "pack " << this->ValueEntries[i]->GetWidgetName() 
               << " -side left -pady 0" << endl;
        }
      }
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

  if (!this->HasSelection())
    {
    for (int i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
      {
      if (this->ValueEntries[i])
        {
        this->ValueEntries[i]->SetEnabled(0);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->ColorSpaceOptionMenu);

  for (int i = 0; i < VTK_KW_CTFE_NB_ENTRIES; i++)
    {
    this->PropagateEnableState(this->ValueEntries[i]);
    }

  this->PropagateEnableState(this->ColorRamp);
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::ColorSpaceCallback()
{
  if (this->ColorTransferFunction)
    {
    const char * value = this->ColorSpaceOptionMenu->GetValue();
    if( strcmp(value, ks_("Color Space|RGB")) == 0)
      {
      if( this->ColorTransferFunction->GetColorSpace() != VTK_CTF_RGB )
        {
        this->ColorTransferFunction->SetColorSpace( VTK_CTF_RGB );
        this->Update();
        if (this->HasSelection())
          {
          this->UpdatePointEntries(this->GetSelectedPoint());
          }
        this->InvokeFunctionChangedCommand();
        }
      }
    else if( strcmp(value, ks_("Color Space|HSV")) == 0)
      {
      if( this->ColorTransferFunction->GetColorSpace() != VTK_CTF_HSV ||
          !this->ColorTransferFunction->GetHSVWrap() )
        {
        this->ColorTransferFunction->SetColorSpace( VTK_CTF_HSV );
        this->ColorTransferFunction->HSVWrapOn();
        this->Update();
        if (this->HasSelection())
          {
          this->UpdatePointEntries(this->GetSelectedPoint());
          }
        this->InvokeFunctionChangedCommand();
        }
      }
    else if( strcmp(value, ks_("Color Space|HSV (2)") ) == 0)
      {
      if( this->ColorTransferFunction->GetColorSpace() != VTK_CTF_HSV ||
          this->ColorTransferFunction->GetHSVWrap() )
        {
        this->ColorTransferFunction->SetColorSpace( VTK_CTF_HSV );
        this->ColorTransferFunction->HSVWrapOff();
        this->Update();
        if (this->HasSelection())
          {
          this->UpdatePointEntries(this->GetSelectedPoint());
          }
        this->InvokeFunctionChangedCommand();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::ValueEntriesCallback(const char *)
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
    values[i] = this->ValueEntries[i]->GetWidget()->GetValueAsDouble();
    }

  // Move the point, check if something has really been moved

  unsigned long mtime = this->GetFunctionMTime();

  this->MoveFunctionPointInColorSpace(
    this->GetSelectedPoint(), 
    parameter, values, this->ColorTransferFunction->GetColorSpace());
  
  if (this->GetFunctionMTime() > mtime)
    {
    this->InvokePointChangedCommand(this->GetSelectedPoint());
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::DoubleClickOnPointCallback(
  int x, int y)
{
  this->Superclass::DoubleClickOnPointCallback(x, y);

  int id, c_x, c_y;

  // No point found

  if (!this->FindFunctionPointAtCanvasCoordinates(x, y, &id, &c_x, &c_y))
    {
    return;
    }

  // Select the point and change its color

  this->SelectPoint(id);
  
  double rgb[3];
  if (!this->FunctionPointValueIsLocked(id) &&
      this->GetPointColorAsRGB(id, rgb) &&
      vtkKWTkUtilities::QueryUserForColor(
        this->GetApplication(),
        this->GetWidgetName(),
        NULL,
        rgb[0], rgb[1], rgb[2],
        &rgb[0], &rgb[1], &rgb[2]))
    {
    unsigned long mtime = this->GetFunctionMTime();

    this->SetPointColorAsRGB(id, rgb);

    if (this->GetFunctionMTime() > mtime)
      {
      this->InvokeFunctionChangedCommand();
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorSpaceOptionMenuVisibility(int arg)
{
  if (this->ColorSpaceOptionMenuVisibility == arg)
    {
    return;
    }

  this->ColorSpaceOptionMenuVisibility = arg;

  // Make sure that if the button has to be shown, we create it on the fly if
  // needed

  if (this->ColorSpaceOptionMenuVisibility && this->IsCreated())
    {
    this->CreateColorSpaceOptionMenu();
    }

  this->UpdateColorSpaceOptionMenu();

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorRampVisibility(int arg)
{
  if (this->ColorRampVisibility == arg)
    {
    return;
    }
    
  this->ColorRampVisibility = arg;

  // Make sure that if the ramp has to be shown, we create it on the fly if
  // needed.

  if (this->ColorRampVisibility)
    {
    if (this->IsCreated() && !this->ColorRamp->IsCreated())
      {
      this->CreateColorRamp();
      }
    }

  this->RedrawColorRamp(); // if we are hidding it, need to update in canvas

  this->Pack();

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::SetColorRampPosition(int arg)
{
  if (arg < vtkKWColorTransferFunctionEditor::ColorRampPositionDefault)
    {
    arg = vtkKWColorTransferFunctionEditor::ColorRampPositionDefault;
    }
  else if (arg > 
           vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas)
    {
    arg = vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas;
    }

  if (this->ColorRampPosition == arg)
    {
    return;
    }

  // If the ramp was drawn in the canvas before, make sure we remove it now

  if (this->ColorRampPosition == 
      vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas)
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
void vtkKWColorTransferFunctionEditor::SetValueEntriesVisibility(int arg)
{
  if (this->ValueEntriesVisibility == arg)
    {
    return;
    }

  this->ValueEntriesVisibility = arg;

  // Make sure that if the entries have to be shown, we create it on the fly if
  // needed

  if (this->ValueEntriesVisibility && 
      this->PointEntriesVisibility && 
      this->IsCreated())
    {
    this->CreateValueEntries();
    }

  this->UpdatePointEntriesLabel();
  this->UpdatePointEntries(this->GetSelectedPoint());

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
int vtkKWColorTransferFunctionEditor::SetPointColorAsRGB(
  int id, double r, double g, double b)
{
  double rgb[3];
  rgb[0] = r;
  rgb[1] = g;
  rgb[2] = b;
  return this->SetPointColorAsRGB(id, rgb);
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
int vtkKWColorTransferFunctionEditor::SetPointColorAsHSV(
  int id, double h, double s, double v)
{
  double hsv[3];
  hsv[0] = h;
  hsv[1] = s;
  hsv[2] = v;
  return this->SetPointColorAsHSV(id, hsv);
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
  // then update the histogram

  if (this->Histogram && this->ComputeHistogramColorFromValue)
    {
    this->RedrawHistogram();
    }

  // The color ramp may (or may not) have to be redrawn, depending on which
  // color tfunc is used in the ramp, so it is also catched by Redraw()
  // and in this method (which can be called independently).

  if (!this->IsColorRampUpToDate())
    {
    this->RedrawColorRamp();
    }
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::RedrawSinglePointDependentElements(
  int id)
{
  this->Superclass::RedrawSinglePointDependentElements(id);

  // The histogram depends on the point too if we color by values

  if (this->Histogram && this->ComputeHistogramColorFromValue)
    {
    this->RedrawHistogram();
    }

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
          this->ColorRampVisibility &&
          this->LastRedrawColorRampTime < func->GetMTime()) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkKWColorTransferFunctionEditor::RedrawColorRamp()
{
  if (!this->ColorRamp->IsCreated() || 
      !this->GetFunctionSize() ||
      this->DisableRedraw)
    {
    return;
    }

  double p_v_range_ext[2];

  if (this->ColorRampVisibility)
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
       vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas);

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
      double bg_r, bg_g, bg_b;
      this->ColorRamp->GetBackgroundColor(&bg_r, &bg_g, &bg_b);
      unsigned char rgb[3];
      rgb[0] = (unsigned char)(bg_r * 255.0);
      rgb[1] = (unsigned char)(bg_g * 255.0);
      rgb[2] = (unsigned char)(bg_b * 255.0);
      img_ptr = img_buffer;
      for (i = 0; i < img_offset_x; i++)
        {
        *img_ptr++ = rgb[0];
        *img_ptr++ = rgb[1];
        *img_ptr++ = rgb[2];
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

    this->ColorRamp->SetImageToPixels(
      img_buffer, img_width, img_height, 3);

    delete [] img_buffer;
    delete [] table;
    }

  // If the ramp has to be in the canvas, draw it now or remove it

  if (this->ColorRampPosition == 
      vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas &&
      this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();
    ostrstream tk_cmd;

    // Create/remove the image item in the canvas only when needed

    int has_tag = this->CanvasHasTag(VTK_KW_CTFE_COLOR_RAMP_TAG);
    if (!has_tag)
      {
      if (this->ColorRampVisibility)
        {
        vtksys_stl::string image_name(
          this->ColorRamp->GetConfigurationOption("-image"));
        tk_cmd << canv << " create image 0 0 -anchor nw "
               << " -image " << image_name.c_str() 
               << " -tags {" << VTK_KW_CTFE_COLOR_RAMP_TAG << "}" << endl;
        tk_cmd << canv << " lower " << VTK_KW_CTFE_COLOR_RAMP_TAG
               << " {" << vtkKWParameterValueFunctionEditor::FunctionTag 
               << "||" << vtkKWParameterValueFunctionEditor::HistogramTag 
               << "||" << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
               << "}" << endl;
        }
      }
    else 
      {
      if (!this->ColorRampVisibility)
        {
        tk_cmd << canv << " delete " << VTK_KW_CTFE_COLOR_RAMP_TAG << endl;
        }
      }

    // Update coordinates

    if (this->ColorRampVisibility)
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
void vtkKWColorTransferFunctionEditor::RedrawHistogram()
{
  if (!this->IsCreated() || !this->Canvas || !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  int has_hist_tag, has_secondary_hist_tag; 
  if (this->ColorRampPosition == 
      vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas)
    {
    has_hist_tag = 
      this->CanvasHasTag(vtkKWParameterValueFunctionEditor::HistogramTag);
    has_secondary_hist_tag = 
      this->CanvasHasTag(vtkKWParameterValueFunctionEditor::SecondaryHistogramTag);
    }

  this->Superclass::RedrawHistogram();

  if (this->ColorRampPosition != 
      vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas)
    {
    return;
    }

  ostrstream tk_cmd;

  // If the primary histogram has just been created, raise or lower it

  if (!has_hist_tag && has_hist_tag != 
      this->CanvasHasTag(vtkKWParameterValueFunctionEditor::HistogramTag))
    {
    cout << "Raising!" << endl;
    tk_cmd << canv << " raise "
           << vtkKWParameterValueFunctionEditor::HistogramTag 
           << " " << VTK_KW_CTFE_COLOR_RAMP_TAG << endl;
    }

  // If the secondary histogram has just been created, raise or lower it

  if (!has_secondary_hist_tag && has_secondary_hist_tag !=
      this->CanvasHasTag(vtkKWParameterValueFunctionEditor::SecondaryHistogramTag))
    {
    cout << "Raising! 2" << endl;
    tk_cmd << canv << " raise " 
           << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
           << " " << VTK_KW_CTFE_COLOR_RAMP_TAG << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
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

  double fr, fg, fb;
  double fh, fs, fv;
  
  this->ColorRamp->GetBackgroundColor(&fr, &fg, &fb);

  bg_rgb[0] = (unsigned char)(fr * 255.0);
  bg_rgb[1] = (unsigned char)(fg * 255.0);
  bg_rgb[2] = (unsigned char)(fb * 255.0);

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

  os << indent << "ValueEntriesVisibility: "
     << (this->ValueEntriesVisibility ? "On" : "Off") << endl;

  os << indent << "ColorSpaceOptionMenuVisibility: "
     << (this->ColorSpaceOptionMenuVisibility ? "On" : "Off") << endl;

  os << indent << "ColorRampVisibility: "
     << (this->ColorRampVisibility ? "On" : "Off") << endl;

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

