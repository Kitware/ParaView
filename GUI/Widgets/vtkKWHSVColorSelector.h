/*=========================================================================

  Module:    vtkKWHSVColorSelector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWHSVColorSelector - an HSV color selector
// .SECTION Description
// A widget that allows the user choose a HSV color interactively

#ifndef __vtkKWHSVColorSelector_h
#define __vtkKWHSVColorSelector_h

#include "vtkKWWidget.h"

class vtkKWCanvas;
class vtkKWLabel;

class VTK_EXPORT vtkKWHSVColorSelector : public vtkKWWidget
{
public:
  static vtkKWHSVColorSelector* New();
  vtkTypeRevisionMacro(vtkKWHSVColorSelector,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Set/Get the hue/saturation wheel radius in pixels.
  virtual void SetHueSatWheelRadius(int);
  vtkGetMacro(HueSatWheelRadius, int);
  
  // Description:
  // Set/Get the value box width in pixels.
  virtual void SetValueBoxWidth(int);
  vtkGetMacro(ValueBoxWidth, int);
  
  // Description:
  // Set/Get the radius of the selection cursor in the hue/sat wheel in pixels.
  virtual void SetHueSatCursorRadius(int);
  vtkGetMacro(HueSatCursorRadius, int);

  // Description:
  // Set/Get the horizontal outer margin of the selection cursor in the value
  // box in pixels.
  virtual void SetValueCursorMargin(int);
  vtkGetMacro(ValueCursorMargin, int);

  // Description:
  // Select/Deselect a color
  vtkGetVector3Macro(SelectedColor, double);
  virtual void SetSelectedColor(double h, double s, double v);
  virtual void SetSelectedColor(double hsv[3])
    { this->SetSelectedColor(hsv[0], hsv[1], hsv[2]); };
  virtual void ClearSelection();
  virtual int  HasSelection();

  // Description:
  // User can only modify the selection, it can not create a selection (i.e.
  // pick a color) when nothing has been selected yet.
  vtkSetMacro(ModificationOnly, int);
  vtkGetMacro(ModificationOnly, int);
  vtkBooleanMacro(ModificationOnly, int);

  // Description:
  // Hide the Value UI.
  virtual void SetHideValue(int);
  vtkGetMacro(HideValue, int);
  vtkBooleanMacro(HideValue, int);

  // Description:
  // Commands.
  virtual void SetSelectionChangedCommand(
    vtkKWObject* object, const char *method);
  virtual void SetSelectionChangingCommand(
    vtkKWObject* object, const char *method);
  virtual void InvokeSelectionChangedCommand();
  virtual void InvokeSelectionChangingCommand();

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Callbacks
  virtual void HueSatPickCallback(int x, int y);
  virtual void HueSatMoveCallback(int x, int y);
  virtual void HueSatReleaseCallback();
  virtual void ValuePickCallback(int x, int y);
  virtual void ValueMoveCallback(int x, int y);
  virtual void ValueReleaseCallback();

  // Description:
  // Access to the canvas and internal elements
  vtkGetObjectMacro(HueSatWheelCanvas, vtkKWCanvas);
  vtkGetObjectMacro(ValueBoxCanvas, vtkKWCanvas);

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

protected:
  vtkKWHSVColorSelector();
  ~vtkKWHSVColorSelector();

  int    HueSatWheelRadius;
  int    HueSatCursorRadius;
  int    ValueBoxWidth;
  int    ValueCursorMargin;
  int    Selected;
  double SelectedColor[3];
  int    ModificationOnly;
  int    HideValue;
  double PreviouslySelectedColor[3];

  // Commands

  char *SelectionChangedCommand;
  char *SelectionChangingCommand;

  virtual void InvokeCommand(const char *command);

  // GUI

  vtkKWCanvas *HueSatWheelCanvas;
  vtkKWCanvas *ValueBoxCanvas;
  vtkKWLabel  *HueSatLabel;
  vtkKWLabel  *ValueLabel;

  // Description:
  // Bind/Unbind all components.
  virtual void Bind();
  virtual void UnBind();

  // Description:
  // Pack the widget
  virtual void Pack();

  // Description:
  // Redraw or update canvas elements
  virtual void Redraw();
  virtual void RedrawHueSatWheelCanvas();
  virtual void UpdateHueSatWheelImage();
  virtual void UpdateHueSatWheelSelection();
  virtual void RedrawValueBoxCanvas();
  virtual void UpdateValueBoxImage();
  virtual void UpdateValueBoxSelection();

  // Description:
  // Get Hue/Sat given coordinates in Hue/Sat wheel image
  // Return 1 if OK, 0 if coords were out of the wheel (i.e. sat was > 1.0)
  virtual int GetHueSatFromCoordinates(int x, int y, double &hue, double &sat);

  // Description:
  // Get Value given coordinates in Value image
  virtual void GetValueFromCoordinate(int y, double &value);

  // Description:
  // Convenience method to look for a tag in a canvas. 
  virtual int CanvasHasTag(const char *canvas, const char *tag);

private:
  vtkKWHSVColorSelector(const vtkKWHSVColorSelector&); // Not implemented
  void operator=(const vtkKWHSVColorSelector&); // Not implemented
};

#endif

