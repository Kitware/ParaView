/*=========================================================================

  Module:    vtkKWColorTransferFunctionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWColorTransferFunctionEditor - a color tfunc function editor
// .SECTION Description
// A widget that allows the user to edit a color transfer function. Note that
// as a subclass of vtkKWParameterValueFunctionEditor, since the 'value' range
// is multi-dimensional (r, g, b), this widget only allows the 'parameter'
// of a function point to be changed (i.e., a point can only be moved
// horizontally).

#ifndef __vtkKWColorTransferFunctionEditor_h
#define __vtkKWColorTransferFunctionEditor_h

#include "vtkKWParameterValueFunctionEditor.h"

#define VTK_KW_CTFE_COLOR_RAMP_TAG "color_ramp_tag"

class vtkColorTransferFunction;
class vtkKWEntryLabeled;
class vtkKWOptionMenu;

class VTK_EXPORT vtkKWColorTransferFunctionEditor : public vtkKWParameterValueFunctionEditor
{
public:
  static vtkKWColorTransferFunctionEditor* New();
  vtkTypeRevisionMacro(vtkKWColorTransferFunctionEditor,vtkKWParameterValueFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the function
  //BTX
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);
  virtual void SetColorTransferFunction(vtkColorTransferFunction*);
  //ETX

  // Description:
  // Set/Get a point color. Those methodes do not trigger any commands/events.
  // Return 1 on success, 0 otherwise (if point does not exist for example)
  virtual int GetPointColorAsRGB(int id, double rgb[3]);
  virtual int GetPointColorAsHSV(int id, double hsv[3]);
  virtual int SetPointColorAsRGB(int id, const double rgb[3]);
  virtual int SetPointColorAsHSV(int id, const double hsv[3]);

  // Description:
  // Show/Hide the color ramp.
  vtkBooleanMacro(ShowColorRamp, int);
  virtual void SetShowColorRamp(int);
  vtkGetMacro(ShowColorRamp, int);

  // Description:
  // Get/Set a specific function to display in the color ramp. If not
  // specified, the ColorTransferFunction will be used.
  //BTX
  vtkGetObjectMacro(ColorRampTransferFunction, vtkColorTransferFunction);
  virtual void SetColorRampTransferFunction(vtkColorTransferFunction*);
  //ETX

  // Description:
  // Set/Get the color ramp height (in pixels).
  virtual void SetColorRampHeight(int);
  vtkGetMacro(ColorRampHeight, int);

  // Description:
  // Show the color ramp at the default position (under the canvas), or 
  // in the canvas itself.
  // The ShowColorRamp parameter still has to be On for the ramp to be
  // shown.
  //BTX
  enum
  {
    ColorRampPositionDefault = 10,
    ColorRampPositionCanvas
  };
  //ETX
  virtual void SetColorRampPosition(int);
  vtkGetMacro(ColorRampPosition, int);

  // Description:
  // Set/get the color ramp outline style.
  //BTX
  enum
  {
    ColorRampOutlineStyleNone = 0,
    ColorRampOutlineStyleSolid,
    ColorRampOutlineStyleSunken
  };
  //ETX
  virtual void SetColorRampOutlineStyle(int);
  vtkGetMacro(ColorRampOutlineStyle, int);

  // Description:
  // Show/Hide the color space option menu.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  virtual void SetShowColorSpaceOptionMenu(int);
  vtkBooleanMacro(ShowColorSpaceOptionMenu, int);
  vtkGetMacro(ShowColorSpaceOptionMenu, int);

  // Description:
  // Show the value entries UI.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowValueEntries, int);
  virtual void SetShowValueEntries(int);
  vtkGetMacro(ShowValueEntries, int);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Pack the widget
  virtual void Pack();

  // Description:
  // Callbacks
  virtual void ColorSpaceCallback();
  virtual void ValueEntriesCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Proxy to the function. 
  // IMPLEMENT those functions in the subclasses.
  // See protected: section too.
  virtual int HasFunction();
  virtual int GetFunctionSize();

protected:
  vtkKWColorTransferFunctionEditor();
  ~vtkKWColorTransferFunctionEditor();

  // Description:
  // Proxy to the function. 
  // Those are low-level manipulators, they do not check if points can
  // be added/removed/locked, it is up to the higer-level methods to do it.
  // IMPLEMENT those functions in the subclasses.
  // See public: section too.
  virtual unsigned long GetFunctionMTime();
  virtual int GetFunctionPointParameter(int id, double *parameter);
  virtual int GetFunctionPointDimensionality();
  virtual int GetFunctionPointValues(int id, double *values);
  virtual int SetFunctionPointValues(int id, const double *values);
  virtual int InterpolateFunctionPointValues(double parameter, double *values);
  virtual int AddFunctionPoint(double parameter, const double *values, int *id);
  virtual int SetFunctionPoint(int id, double parameter, const double *values);
  virtual int RemoveFunctionPoint(int id);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  MoveFunctionPointInColorSpace(
    int id, double parameter, const double *values, int colorspace);

  virtual void UpdatePointEntries(int id);

  vtkColorTransferFunction *ColorTransferFunction;
  vtkColorTransferFunction *ColorRampTransferFunction;

  int ShowValueEntries;
  int ShowColorSpaceOptionMenu;
  int ShowColorRamp;
  int ColorRampHeight;
  int ColorRampPosition;
  int ColorRampOutlineStyle;
  unsigned long LastRedrawColorRampTime;

  // GUI

  vtkKWOptionMenu   *ColorSpaceOptionMenu;
  vtkKWEntryLabeled *ValueEntries[3];
  vtkKWLabel        *ColorRamp;

  // Description:
  // Redraw
  virtual void Redraw();
  virtual void RedrawSizeDependentElements();
  virtual void RedrawPanOnlyDependentElements();
  virtual void RedrawFunctionDependentElements();

  // Description:
  // Redraw the histogram
  //BTX
  virtual void UpdateHistogramImageDescriptor(vtkKWHistogram::ImageDescriptor*);
  //ETX

  // Description:
  // Redraw the color ramp
  virtual void RedrawColorRamp();
  virtual int IsColorRampUpToDate();
  virtual void GetColorRampOutlineSunkenColors(
    unsigned char bg_rgb[3], unsigned char ds_rgb[3], unsigned char ls_rgb[3],
    unsigned char hl_rgb[3]);

  // Description:
  // Update the entries label (depending on the color space)
  // and the color space menu
  virtual void UpdatePointEntriesLabel();
  virtual void UpdateColorSpaceOptionMenu();

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateColorSpaceOptionMenu(vtkKWApplication *app);
  virtual void CreateColorRamp(vtkKWApplication *app);
  virtual void CreateValueEntries(vtkKWApplication *app);
  virtual int IsTopLeftFrameUsed();
  virtual int IsTopRightFrameUsed();

private:
  vtkKWColorTransferFunctionEditor(const vtkKWColorTransferFunctionEditor&); // Not implemented
  void operator=(const vtkKWColorTransferFunctionEditor&); // Not implemented
};

#endif

