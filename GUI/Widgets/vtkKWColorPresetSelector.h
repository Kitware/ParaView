/*=========================================================================

  Module:    vtkKWColorPresetSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWColorPresetSelector - a color preset selector.
// .SECTION Description
// This class displays a color preset selector as an option menu.
// Different type of presets can be enabled/disabled.
// .SECTION See Also
// vtkKWOptionMenu

#ifndef __vtkKWColorPresetSelector_h
#define __vtkKWColorPresetSelector_h

#include "vtkKWLabeledOptionMenu.h"

class vtkColorTransferFunction;
class vtkKWColorPresetSelectorInternals;

//----------------------------------------------------------------------------

class VTK_EXPORT vtkKWColorPresetSelector : public vtkKWLabeledOptionMenu
{
public:
  static vtkKWColorPresetSelector* New();
  vtkTypeRevisionMacro(vtkKWColorPresetSelector,vtkKWLabeledOptionMenu);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the color transfer function the preset will be applied to.
  // Note that a color transfer function is created by default.
  virtual void SetColorTransferFunction(vtkColorTransferFunction *p);
  vtkGetObjectMacro(ColorTransferFunction,vtkColorTransferFunction);

  // Description:
  // Set/Get the scalar range along which the preset will be applied.
  // Solid color presets will be created using two entries, one at each
  // end of the range.
  // Gradient, or custom presets will be scaled appropriately along
  // the range.
  // If ApplyPresetBetweenEndPoint is true, the preset is applied
  // given the scalar range defined between the two end-points (if any)
  vtkGetVector2Macro(ScalarRange, double);
  vtkSetVector2Macro(ScalarRange, double);
  vtkGetMacro(ApplyPresetBetweenEndPoints, int);
  vtkSetMacro(ApplyPresetBetweenEndPoints, int);
  vtkBooleanMacro(ApplyPresetBetweenEndPoints, int);

  // Description:
  // Add a color preset.
  // A name is required, as well as a color transfer function and a range.
  // The range specifies the scalar range of the color tfunc, and is used
  // to store a normalized version of the color transfer function so that
  // it can be applied to the ColorTransferFunction ivar according to the 
  // ScalarRange ivar. The color transfer function passed as parameter
  // is not Register()'ed.
  // Return 1 on success, 0 otherwise
  virtual int AddPreset(
    const char *name, vtkColorTransferFunction *func, double range[2]);

  // Description:
  // Remove color preset.
  // Return 1 on success, 0 otherwise
  virtual int RemovePreset(const char *name);

  // Description:
  // Add a color preset. 
  // Convenience methods to add a preset given a solid color, in RGB or HSV format.
  virtual int AddSolidRGBPreset(const char *name, double rgb[3]);
  virtual int AddSolidRGBPreset(const char *name,double r,double g, double b);
  virtual int AddSolidHSVPreset(const char *name, double hsv[3]);
  virtual int AddSolidHSVPreset(const char *name,double h,double s, double v);

  // Description:
  // Add a color preset. 
  // Convenience methods to add a gradient preset given the endpoints of
  // the gradient, in RGB or HSV format.
  virtual int AddGradientRGBPreset(
    const char *name, double rgb1[3], double rgb2[3]);
  virtual int AddGradientRGBPreset(
    const char *name, 
    double r1, double g1, double b1, 
    double r2, double g2, double b2);
  virtual int AddGradientHSVPreset(
    const char *name, double hsv1[3], double hsv2[3]);
  virtual int AddGradientHSVPreset(
    const char *name, 
    double h1, double s1, double v1, 
    double h2, double s2, double v2);

  // Description:
  // Add a color preset. 
  // Convenience methods to add a "flag" preset given the number of colors
  // in the flag, a pointer to those colors, and the number of time the
  // flag should be repeated in the scalar range.
  virtual int AddFlagRGBPreset(
    const char *name, int nb_colors, double **rgb, int repeat);

  // Description:
  // Set/Get the preview size. Each entry in the menu also displays a
  // preview of the preset.
  vtkGetMacro(PreviewSize, int);
  virtual void SetPreviewSize(int);

  // Description:
  // Hide solid color presets.
  vtkGetMacro(HideSolidColorPresets, int);
  vtkBooleanMacro(HideSolidColorPresets, int);
  virtual void SetHideSolidColorPresets(int);

  // Description:
  // Hide gradient presets (i.e. anything that is not a solid color).
  vtkGetMacro(HideGradientPresets, int);
  vtkBooleanMacro(HideGradientPresets, int);
  virtual void SetHideGradientPresets(int);

  // Description:
  // Create the widget
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Set/Get the command executed when a preset a selected.
  virtual void SetPresetSelectedCommand(
    vtkKWObject *object, const char *method);

  // Description:
  // Callbacks
  virtual void PresetSelectedCallback(const char *name);

protected:
  vtkKWColorPresetSelector();
  ~vtkKWColorPresetSelector();

  double ScalarRange[2];
  vtkColorTransferFunction *ColorTransferFunction;
  char *PresetSelectedCommand;
  int PreviewSize;
  int HideSolidColorPresets;
  int HideGradientPresets;
  int ApplyPresetBetweenEndPoints;

  // PIMPL Encapsulation for STL containers

  vtkKWColorPresetSelectorInternals *Internals;

  // Description:
  // Query if there is a preset with a given name, create a preset
  // with a given name
  // Return 1 on success, 0 otherwise
  virtual int HasPreset(const char *name);
  virtual int AllocatePreset(const char *name);

  // Description:
  // Get a preset color transfer function.
  // Return the func on success, NULL otherwise
  virtual vtkColorTransferFunction* GetPresetColorTransferFunction(
    const char *name);

  // Description:
  // Create the default presets
  virtual void CreateDefaultPresets();

  // Description:
  // Map one transfer function to another
  // Return 1 on success, 0 otherwise
  virtual int MapColorTransferFunction(
    vtkColorTransferFunction *source, double source_range[2],
    vtkColorTransferFunction *target, double target_range[2]);

  // Description:
  // Create preview (icon/image) for a preset
  virtual int CreateColorTransferFunctionPreview(
    vtkColorTransferFunction *func, const char *img_name);

  // Description:
  // Populate the preset menu
  virtual void PopulatePresetMenu();

private:
  vtkKWColorPresetSelector(const vtkKWColorPresetSelector&); // Not implemented
  void operator=(const vtkKWColorPresetSelector&); // Not Implemented
};

#endif

