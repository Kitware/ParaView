/*=========================================================================

  Module:    vtkKWSpinBox.h,v

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWSpinBox - SpinBox
// .SECTION Description
// A widget with up and down arrow controls and direct text editing.
// Typically used with integer fields that users increment by 1 (or
// decrement) by clicking on the arrows.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWSpinBox_h
#define __vtkKWSpinBox_h

#include "vtkKWCoreWidget.h"

class KWWidgets_EXPORT vtkKWSpinBox : public vtkKWCoreWidget
{
public:
  static vtkKWSpinBox* New();
  vtkTypeRevisionMacro(vtkKWSpinBox,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the range. Default to [0, 10]
  virtual void SetRange(double from, double to);

  // Description:
  // Set the increment value. Default to 1.
  virtual void SetIncrement(double increment);
  virtual double GetIncrement();

  // Description:
  // Set/Get the current value.
  virtual void SetValue(double value);
  virtual double GetValue();

  // Description:
  // Set/Get the string used to format the value.
  // Specifies an alternate format to use when setting the string value when
  // using the range. This must be a format specifier of the
  // form %<pad>.<pad>f, as it will format a floating-point number.
  virtual void SetValueFormat(const char *format);
  virtual const char* GetValueFormat();

  // Description:
  // Set/Get the wrap. If on, values at edges of range wrap around to the
  // other side of the range when clicking on the up/down arrows.
  virtual void SetWrap(int wrap);
  virtual int GetWrap();
  vtkBooleanMacro(Wrap, int);

  // Description:
  // Restrict the value to a given type (integer, double, or no restriction).
  // Note: checks against RestrictValue are performed before ValidationCommand.
  //BTX
  enum
  {
    RestrictNone = 0,
    RestrictInteger,
    RestrictDouble
  };
  //ETX
  vtkGetMacro(RestrictValue, int);
  virtual void SetRestrictValue(int);
  virtual void SetRestrictValueToInteger();
  virtual void SetRestrictValueToDouble();
  virtual void SetRestrictValueToNone();

  // Description:
  // Specifies a command to associate with this step. This command can
  // be used to validate the contents of the widget.
  // Note: checks against RestrictValue are performed before ValidationCommand.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // This command should return 1 if the contents is valid, 0 otherwise.
  // The following parameters are also passed to the command:
  // - current value: const char*
  virtual void SetValidationCommand(vtkObject *object, const char *method);
  virtual int InvokeValidationCommand(const char *value);

  // Description:
  // Set/Get the width of the spinbox in number of characters.
  virtual void SetWidth(int);
  virtual int GetWidth();

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget.
  virtual void GetForegroundColor(double *r, double *g, double *b);
  virtual double* GetForegroundColor();
  virtual void SetForegroundColor(double r, double g, double b);
  virtual void SetForegroundColor(double rgb[3])
    { this->SetForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the background color of the widget when it is disabled.
  virtual void GetDisabledBackgroundColor(double *r, double *g, double *b);
  virtual double* GetDisabledBackgroundColor();
  virtual void SetDisabledBackgroundColor(double r, double g, double b);
  virtual void SetDisabledBackgroundColor(double rgb[3])
    { this->SetDisabledBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the foreground color of the widget when it is disabled.
  virtual void GetDisabledForegroundColor(double *r, double *g, double *b);
  virtual double* GetDisabledForegroundColor();
  virtual void SetDisabledForegroundColor(double r, double g, double b);
  virtual void SetDisabledForegroundColor(double rgb[3])
    { this->SetDisabledForegroundColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the background color of the widget when it is read-only.
  virtual void GetReadOnlyBackgroundColor(double *r, double *g, double *b);
  virtual double* GetReadOnlyBackgroundColor();
  virtual void SetReadOnlyBackgroundColor(double r, double g, double b);
  virtual void SetReadOnlyBackgroundColor(double rgb[3])
    { this->SetReadOnlyBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the background color of the spin-buttons.
  virtual void GetButtonBackgroundColor(double *r, double *g, double *b);
  virtual double* GetButtonBackgroundColor();
  virtual void SetButtonBackgroundColor(double r, double g, double b);
  virtual void SetButtonBackgroundColor(double rgb[3])
    { this->SetButtonBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/Get the active background color of the widget. An element
  // (a widget or portion of a widget) is active if the mouse cursor is
  // positioned over the element and pressing a mouse button will cause some
  // action to occur.
  virtual void GetActiveBackgroundColor(double *r, double *g, double *b);
  virtual double* GetActiveBackgroundColor();
  virtual void SetActiveBackgroundColor(double r, double g, double b);
  virtual void SetActiveBackgroundColor(double rgb[3])
    { this->SetActiveBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the border width, a non-negative value indicating the width of
  // the 3-D border to draw around the outside of the widget (if such a border
  // is being drawn; the Relief option typically determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised();
  virtual void SetReliefToSunken();
  virtual void SetReliefToFlat();
  virtual void SetReliefToRidge();
  virtual void SetReliefToSolid();
  virtual void SetReliefToGroove();

  // Description:
  // Specifies the font to use when drawing text inside the widget. 
  // You can use predefined font names (e.g. 'system'), or you can specify
  // a set of font attributes with a platform-independent name, for example,
  // 'times 12 bold'. In this example, the font is specified with a three
  // element list: the first element is the font family, the second is the
  // size, the third is a list of style parameters (normal, bold, roman, 
  // italic, underline, overstrike). Example: 'times 12 {bold italic}'.
  // The Times, Courier and Helvetica font families are guaranteed to exist
  // and will be matched to the corresponding (closest) font on your system.
  // If you are familiar with the X font names specification, you can also
  // describe the font that way (say, '*times-medium-r-*-*-12*').
  virtual void SetFont(const char *font);
  virtual const char* GetFont();

  // Description:
  // Specifies whether or not a selection in the widget should also be the X
  // selection. If the selection is exported, then selecting in the widget
  // deselects the current X selection, selecting outside the widget deselects
  // any widget selection, and the widget will respond to selection retrieval
  // requests when it has a selection.  
  virtual void SetExportSelection(int);
  virtual int GetExportSelection();
  vtkBooleanMacro(ExportSelection, int);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the return key is pressed, or the focus is lost,
  // as specified by the CommandTrigger variable. It is also invoked when
  // the spinbuttons are pressed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the current value: int (if Increment is integer); double otherwise
  //   Note: the 'int' signature is for convenience, so that the command can
  //   be set to a callback accepting 'int'. In doubt, implement the callback
  //   using a 'double' signature that will accept both 'int' and 'double'.
  virtual void SetCommand(vtkObject *object, const char *method);
  virtual void InvokeCommand(double value);

  // Description:
  // Specify when Command should be invoked. Default to losing focus and
  // return key.
  //BTX
  enum
  {
    TriggerOnFocusOut  = 1,
    TriggerOnReturnKey = 2,
    TriggerOnAnyChange = 4
  };
  //ETX
  vtkGetMacro(CommandTrigger, int);
  virtual void SetCommandTrigger(int);
  virtual void SetCommandTriggerToReturnKeyAndFocusOut();
  virtual void SetCommandTriggerToAnyChange();

  // Description:
  // Events. The SpinBoxValueChangedEvent is triggered when the widget value
  // is changed. It is similar in concept to the 'Command' callback but can be
  // used by multiple listeners/observers at a time.
  // Important: since there is no way to robustly find out when the user
  // is done inputing characters in the text entry part of the spinbox, the 
  // SpinBoxValueChangedEvent event is also generated when <Return> is pressed,
  // or the spinbox widget is losing focus (i.e. the user clicked outside the
  // text field).
  // The following parameters are also passed as client data:
  // - the current value: double
  //BTX
  enum
  {
    SpinBoxValueChangedEvent = 10000
  };
  //ETX

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
  virtual void ValueCallback();
  virtual int ValidationCallback(const char *value);
  virtual void TracedVariableChangedCallback(
    const char *, const char *, const char *);

protected:
  vtkKWSpinBox();
  ~vtkKWSpinBox();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int RestrictValue;
  int CommandTrigger;

  char *Command;
  char *ValidationCommand;

  // Description:
  // Configure.
  virtual void Configure();
  virtual void ConfigureValidation();

private:
  vtkKWSpinBox(const vtkKWSpinBox&); // Not implemented
  void operator=(const vtkKWSpinBox&); // Not implemented
};

#endif
