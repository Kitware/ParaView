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
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWColorTransferFunctionEditor_h
#define __vtkKWColorTransferFunctionEditor_h

#include "vtkKWParameterValueHermiteFunctionEditor.h"

class vtkColorTransferFunction;
class vtkKWEntryWithLabel;
class vtkKWMenuButton;

class KWWidgets_EXPORT vtkKWColorTransferFunctionEditor : public vtkKWParameterValueHermiteFunctionEditor
{
public:
  static vtkKWColorTransferFunctionEditor* New();
  vtkTypeRevisionMacro(vtkKWColorTransferFunctionEditor,vtkKWParameterValueHermiteFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the function
  // Note that the whole parameter range is automatically reset to the
  // function range.
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);
  virtual void SetColorTransferFunction(vtkColorTransferFunction*);

  // Description:
  // Set/Get a point color. Those methodes do not trigger any commands/events.
  // Return 1 on success, 0 otherwise (if point does not exist for example)
  virtual int GetPointColorAsRGB(int id, double rgb[3]);
  virtual int GetPointColorAsHSV(int id, double hsv[3]);
  virtual int SetPointColorAsRGB(int id, const double rgb[3]);
  virtual int SetPointColorAsRGB(int id, double r, double g, double b);
  virtual int SetPointColorAsHSV(int id, const double hsv[3]);
  virtual int SetPointColorAsHSV(int id, double h, double s, double v);

  // Description:
  // Set/Get the color ramp visibility.
  vtkBooleanMacro(ColorRampVisibility, int);
  virtual void SetColorRampVisibility(int);
  vtkGetMacro(ColorRampVisibility, int);

  // Description:
  // Get/Set a specific function to display in the color ramp. If not
  // specified, the ColorTransferFunction will be used.
  vtkGetObjectMacro(ColorRampTransferFunction, vtkColorTransferFunction);
  virtual void SetColorRampTransferFunction(vtkColorTransferFunction*);

  // Description:
  // Set/Get the color ramp height (in pixels).
  virtual void SetColorRampHeight(int);
  vtkGetMacro(ColorRampHeight, int);

  // Description:
  // Display the color ramp at the default position (under the canvas), or 
  // in the canvas itself.
  // The ColorRampVisibility parameter still has to be On for the ramp to be
  // displayed.
  //BTX
  enum
  {
    ColorRampPositionDefault = 10,
    ColorRampPositionCanvas
  };
  //ETX
  virtual void SetColorRampPosition(int);
  vtkGetMacro(ColorRampPosition, int);
  virtual void SetColorRampPositionToDefault()
    { this->SetColorRampPosition(
      vtkKWColorTransferFunctionEditor::ColorRampPositionDefault); };
  virtual void SetColorRampPositionToCanvas()
    { this->SetColorRampPosition(
      vtkKWColorTransferFunctionEditor::ColorRampPositionCanvas); };

  // Description:
  // Set/Get the color ramp outline style.
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
  virtual void SetColorRampOutlineStyleToNone()
    { this->SetColorRampOutlineStyle(
      vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleNone); };
  virtual void SetColorRampOutlineStyleToSolid()
    { this->SetColorRampOutlineStyle(
      vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSolid); };
  virtual void SetColorRampOutlineStyleToSunken()
    { this->SetColorRampOutlineStyle(
      vtkKWColorTransferFunctionEditor::ColorRampOutlineStyleSunken); };

  // Description:
  // Set/Get the color space option menu visibility.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  virtual void SetColorSpaceOptionMenuVisibility(int);
  vtkBooleanMacro(ColorSpaceOptionMenuVisibility, int);
  vtkGetMacro(ColorSpaceOptionMenuVisibility, int);

  // Description:
  // Set/Get the value entries UI visibility.
  // Not shown if superclass PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ValueEntriesVisibility, int);
  virtual void SetValueEntriesVisibility(int);
  vtkGetMacro(ValueEntriesVisibility, int);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

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
  virtual unsigned long GetFunctionMTime();
  virtual int GetFunctionPointParameter(int id, double *parameter);
  virtual int GetFunctionPointDimensionality();

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ColorSpaceCallback();
  virtual void ValueEntriesCallback(const char *value);
  virtual void DoubleClickOnPointCallback(int x, int y);

protected:
  vtkKWColorTransferFunctionEditor();
  ~vtkKWColorTransferFunctionEditor();

  // Description:
  // Proxy to the function. 
  // Those are low-level manipulators, they do not check if points can
  // be added/removed/locked, it is up to the higer-level methods to do it.
  // IMPLEMENT those functions in the subclasses.
  // See public: section too.
  virtual int GetFunctionPointValues(int id, double *values);
  virtual int SetFunctionPointValues(int id, const double *values);
  virtual int InterpolateFunctionPointValues(double parameter, double *values);
  virtual int AddFunctionPoint(
    double parameter, const double *values, int *id);
  virtual int SetFunctionPoint(int id, double parameter, const double *values);
  virtual int RemoveFunctionPoint(int id);
  virtual int GetFunctionPointMidPoint(int id, double *pos);
  virtual int SetFunctionPointMidPoint(int id, double pos);
  virtual int GetFunctionPointSharpness(int id, double *sharpness);
  virtual int SetFunctionPointSharpness(int id, double sharpness);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  MoveFunctionPointInColorSpace(
    int id, double parameter, const double *values, int colorspace);

  virtual void UpdatePointEntries(int id);

  vtkColorTransferFunction *ColorTransferFunction;
  vtkColorTransferFunction *ColorRampTransferFunction;

  int ValueEntriesVisibility;
  int ColorSpaceOptionMenuVisibility;
  int ColorRampVisibility;
  int ColorRampHeight;
  int ColorRampPosition;
  int ColorRampOutlineStyle;
  unsigned long LastRedrawColorRampTime;

  // GUI

  vtkKWMenuButton   *ColorSpaceOptionMenu;
  vtkKWEntryWithLabel *ValueEntries[3];
  vtkKWLabel        *ColorRamp;

  // Description:
  // Redraw
  virtual void Redraw();
  virtual void RedrawSizeDependentElements();
  virtual void RedrawPanOnlyDependentElements();
  virtual void RedrawFunctionDependentElements();
  virtual void RedrawSinglePointDependentElements(int id);

  // Description:
  // Redraw the histogram
  //BTX
  virtual void UpdateHistogramImageDescriptor(vtkKWHistogram::ImageDescriptor*);
  //ETX

  // Description:
  // Pack the widget
  virtual void Pack();
  virtual void PackPointEntries();

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
  virtual void CreateColorSpaceOptionMenu();
  virtual void CreateColorRamp();
  virtual void CreateValueEntries();
  virtual int IsTopLeftFrameUsed();
  virtual int IsPointEntriesFrameUsed();

private:
  vtkKWColorTransferFunctionEditor(const vtkKWColorTransferFunctionEditor&); // Not implemented
  void operator=(const vtkKWColorTransferFunctionEditor&); // Not implemented
};

#endif

