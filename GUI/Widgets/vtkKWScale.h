/*=========================================================================

  Module:    vtkKWScale.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWScale - a scale (slider) widget
// .SECTION Description
// A widget that repsentes a scale (or slider).
// .SECTION See Also
// vtkKWScaleWithEntry

#ifndef __vtkKWScale_h
#define __vtkKWScale_h

#include "vtkKWCoreWidget.h"

class vtkKWEntry;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWTopLevel;

class KWWidgets_EXPORT vtkKWScale : public vtkKWCoreWidget
{
public:
  static vtkKWScale* New();
  vtkTypeRevisionMacro(vtkKWScale,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set the range for this scale.
  virtual void SetRange(double min, double max);
  virtual void SetRange(const double *range) 
    { this->SetRange(range[0], range[1]); };
  vtkGetVector2Macro(Range, double);
  virtual double GetRangeMin() { return this->GetRange()[0]; };
  virtual double GetRangeMax() { return this->GetRange()[1]; };

  // Description:
  // Set/Get the value of the scale.
  virtual void SetValue(double v);
  vtkGetMacro(Value, double);

  // Description:
  // Set/Get the resolution of the slider.
  // The range or the value of the scale are not snapped to this resolution.
  // The range and the value can be any floating point number. 
  // Think of the slider and the resolution as a way to set the value
  // interactively using nice clean steps (power of 10 for example).
  // The entry associated to the scale can be used to set the value to 
  // anything within the range, despite the resolution, allowing the user
  // to enter a precise value that could not be reached given the resolution.
  virtual void SetResolution(double r);
  vtkGetMacro(Resolution, double);
  
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
  // Set/Get the orientation type.
  // For widgets that can lay themselves out with either a horizontal or
  // vertical orientation, such as scales, this option specifies which 
  // orientation should be used. 
  // Valid constants can be found in vtkKWOptions::OrientationType.
  virtual void SetOrientation(int);
  vtkGetMacro(Orientation, int);
  virtual void SetOrientationToHorizontal();
  virtual void SetOrientationToVertical();

  // Description:
  // Set/Get the trough color, i.e. the color to use for the rectangular
  // trough areas in widgets such as scrollbars and scales.
  virtual void GetTroughColor(double *r, double *g, double *b);
  virtual double* GetTroughColor();
  virtual void SetTroughColor(double r, double g, double b);
  virtual void SetTroughColor(double rgb[3])
    { this->SetTroughColor(rgb[0], rgb[1], rgb[2]); };

  // Description
  // Set/Get the narrow dimension of scale. For vertical 
  // scales this is the trough's width; for horizontal scales this is the 
  // trough's height. In pixel.
  virtual void SetWidth(int width);
  virtual int GetWidth();

  // Description
  // Set/Get the desired long dimension of the scale. 
  // For vertical scales this is the scale's height, for horizontal scales
  // it is the scale's width. In pixel.
  virtual void SetLength(int length);
  virtual int GetLength();

  // Description
  // Set/Get the size of the slider, measured in screen units along 
  // the slider's long dimension.
  virtual void SetSliderLength(int length);
  virtual int GetSliderLength();

  // Description:
  // Set/Get the visibility of the value on top of the slider.
  virtual void SetValueVisibility(int);
  virtual int GetValueVisibility();
  vtkBooleanMacro(ValueVisibility, int);

  // Description:
  // Set/Get the tick interval.
  // Determines the spacing between numerical tick marks displayed below or to
  // the left of the slider. If 0, no tick marks will be displayed. 
  virtual void SetTickInterval(double val);
  virtual double GetTickInterval();
  
  // Description
  // Set/Get the string to display as a label for the scale. 
  // For vertical scales the label is displayed just to the right of the top
  // end of the scale. For horizontal scales the label is displayed just above
  // the left end of the scale. If the option is specified as an empty string, 
  // no label is displayed. The position of the label can not be changed. For
  // more elaborated options, check vtkKWScaleWithEntry
  virtual void SetLabelText(const char *);
  virtual const char* GetLabelText();

  // Description:
  // Specifies commands to associate with the widget. 
  // 'Command' is invoked when the widget value is changing (i.e. during
  // user interaction).
  // 'StartCommand' is invoked at the beginning of a user interaction with
  // the widget (when a mouse button is pressed over the widget for example).
  // 'EndCommand' is invoked at the end of the user interaction with the 
  // widget (when the mouse button is released for example).
  // The need for a 'Command', 'StartCommand' and 'EndCommand' can be
  // explained as follows: 'EndCommand' can be used to be notified about any
  // changes made to this widget *after* the corresponding user interaction has
  // been performed (say, after releasing the mouse button that was dragging
  // a slider, or after clicking on a checkbutton). 'Command' can be set
  // *additionally* to be notified about the intermediate changes that
  // occur *during* the corresponding user interaction (say, *while* dragging
  // a slider). While setting 'EndCommand' is enough to be notified about
  // any changes, setting 'Command' is an application-specific choice that
  // is likely to depend on how fast you want (or can) answer to rapid changes
  // occuring during a user interaction, if any. 'StartCommand' is rarely
  // used but provides an opportunity for the application to modify its
  // state and prepare itself for user-interaction; in that case, the
  // 'EndCommand' is usually set in a symmetric fashion to set the application
  // back to its previous state.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the current value: int (if Resolution is integer); double otherwise
  //   Note: the 'int' signature is for convenience, so that the command can
  //   be set to a callback accepting 'int'. In doubt, implement the callback
  //   using a 'double' signature that will accept both 'int' and 'double'.
  virtual void SetCommand(vtkObject *object, const char *method);
  virtual void SetStartCommand(vtkObject *object, const char *method);
  virtual void SetEndCommand(vtkObject *object, const char *method);

  // Description:
  // Events. The ScaleValueChangingEvent is triggered when the widget value
  // is changed (i.e., during user interaction on the widget's slider),
  // the ScaleValueStartChangingEvent is invoked at the beginning of an 
  // interaction with the widget, the ScaleValueChangedEvent is invoked at the
  // end of an interaction with the widget. They are similar in concept as
  // the 'Command', 'StartCommand', and 'EndCommand' callbacks but can be
  // used by multiple listeners/observers at a time.
  // The following parameters are also passed as client data:
  // - the current value: double
  //BTX
  enum
  {
    ScaleValueChangingEvent = 10000,
    ScaleValueChangedEvent,
    ScaleValueStartChangingEvent
  };
  //ETX

  // Description:
  // Set/Get whether the above commands should be called or not.
  // This make it easier to disable the commands while setting the scale
  // value for example.
  vtkSetMacro(DisableCommands, int);
  vtkGetMacro(DisableCommands, int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Turn on/off the automatic clamping of the end values when the 
  // user types a value beyond the range. Default is on.
  vtkSetMacro(ClampValue, int);
  vtkGetMacro(ClampValue, int);
  vtkBooleanMacro(ClampValue, int);

  // Description:
  // Callbacks. Internal, do not use.
  vtkGetMacro(DisableScaleValueCallback, int);
  vtkSetMacro(DisableScaleValueCallback, int);
  vtkBooleanMacro(DisableScaleValueCallback, int);
  virtual void ScaleValueCallback(double num);
  virtual void ButtonPressCallback();
  virtual void ButtonReleaseCallback();

protected:
  vtkKWScale();
  ~vtkKWScale();

  // Description:
  // Bind/Unbind all components so that values can be changed, but
  // no command will be called.
  void Bind();
  void UnBind();

  int DisableScaleValueCallback;
  int ClampValue;

  double       Value;
  double       Resolution;
  double       Range[2];

  int Orientation;

  // Description:
  // Update internal widgets value
  virtual void UpdateRange();
  virtual void UpdateResolution();
  virtual void UpdateValue();
  virtual void UpdateOrientation();

  //BTX
  friend class vtkKWScaleWithEntry;
  //ETX

  int DisableCommands;
  char *Command;
  char *StartCommand;
  char *EndCommand;

  virtual void InvokeScaleCommand(const char *command, double value);
  virtual void InvokeCommand(double value);
  virtual void InvokeStartCommand(double value);
  virtual void InvokeEndCommand(double value);

private:
  vtkKWScale(const vtkKWScale&); // Not implemented
  void operator=(const vtkKWScale&); // Not implemented
};


#endif
