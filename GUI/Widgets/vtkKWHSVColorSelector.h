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

#include "vtkKWCompositeWidget.h"

class vtkKWCanvas;
class vtkKWLabel;

class KWWidgets_EXPORT vtkKWHSVColorSelector : public vtkKWCompositeWidget
{
public:
  static vtkKWHSVColorSelector* New();
  vtkTypeRevisionMacro(vtkKWHSVColorSelector,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Select/Deselect a color (in HSV space)
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
  // Specifies commands to associate with the widget. 
  // 'SelectionChangedCommand' is invoked when the selected color has
  // changed (i.e. at the end of the user interaction).
  // 'SelectionChangingCommand' is invoked when the selected color is
  // changing (i.e. during the user interaction).
  // The need for a '...ChangedCommand' and '...ChangingCommand' can be
  // explained as follows: the former can be used to be notified about any
  // changes made to this widget *after* the corresponding user interaction has
  // been performed (say, after releasing the mouse button that was dragging
  // a slider, or after clicking on a checkbutton). The later can be set
  // *additionally* to be notified about the intermediate changes that
  // occur *during* the corresponding user interaction (say, *while* dragging
  // a slider). While setting '...ChangedCommand' is enough to be notified
  // about any changes, setting '...ChangingCommand' is an application-specific
  // choice that is likely to depend on how fast you want (or can) answer to
  // rapid changes occuring during a user interaction, if any.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - selected HSV color: double, double, double
  // Note that if InvokeCommandsWithRGB is true, the selected color is passed
  // as RGB instead of HSV.
  virtual void SetSelectionChangedCommand(
    vtkObject *object, const char *method);
  virtual void SetSelectionChangingCommand(
    vtkObject *object, const char *method);

  // Description:
  // Set/Get if the commands should be invoked with RGB parameters instead
  // of the current HSV value.
  vtkSetMacro(InvokeCommandsWithRGB, int);
  vtkGetMacro(InvokeCommandsWithRGB, int);
  vtkBooleanMacro(InvokeCommandsWithRGB, int);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);

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

  // Description:
  // Callbacks. Internal, do not use.
  virtual void HueSatPickCallback(int x, int y);
  virtual void HueSatMoveCallback(int x, int y);
  virtual void HueSatReleaseCallback();
  virtual void ValuePickCallback(int x, int y);
  virtual void ValueMoveCallback(int x, int y);
  virtual void ValueReleaseCallback();

protected:
  vtkKWHSVColorSelector();
  ~vtkKWHSVColorSelector();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

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

  int InvokeCommandsWithRGB;
  virtual void InvokeCommandWithColor(
    const char *command, double h, double s, double v);
  virtual void InvokeSelectionChangedCommand(double h, double s, double v);
  virtual void InvokeSelectionChangingCommand(double h, double s, double v);

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
  // Look for a tag in a canvas. 
  virtual int CanvasHasTag(const char *canvas, const char *tag);

private:
  vtkKWHSVColorSelector(const vtkKWHSVColorSelector&); // Not implemented
  void operator=(const vtkKWHSVColorSelector&); // Not implemented
};

#endif

