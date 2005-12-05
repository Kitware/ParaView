/*=========================================================================

  Module:    vtkKWThumbWheel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWThumbWheel - a thumbwheel widget
// .SECTION Description
// A widget that repsentes a thumbwheel widget with options for 
// a label string and a text entry box.

#ifndef __vtkKWThumbWheel_h
#define __vtkKWThumbWheel_h

#include "vtkKWCompositeWidget.h"

class vtkKWLabel;
class vtkKWEntry;
class vtkKWPushButton;
class vtkKWTopLevel;

class KWWIDGETS_EXPORT vtkKWThumbWheel : public vtkKWCompositeWidget
{
public:
  static vtkKWThumbWheel* New();
  vtkTypeRevisionMacro(vtkKWThumbWheel,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the value of the thumbwheel.
  virtual void SetValue(double v);
  vtkGetMacro(Value, double);

  // Description:
  // Set/Get the minimum value. The current value will be clamped only if
  // ClampMinimumValue is true.
  vtkSetMacro(MinimumValue, double);
  vtkGetMacro(MinimumValue, double);
  vtkSetMacro(ClampMinimumValue, int);
  vtkGetMacro(ClampMinimumValue, int);
  vtkBooleanMacro(ClampMinimumValue, int);  

  // Description:
  // Set/Get the maximum value. The current value will be clamped only if
  // ClampMaximumValue is true.
  vtkSetMacro(MaximumValue, double);
  vtkGetMacro(MaximumValue, double);
  vtkSetMacro(ClampMaximumValue, int);
  vtkGetMacro(ClampMaximumValue, int);
  vtkBooleanMacro(ClampMaximumValue, int);  

  // Description:
  // Set the range.
  virtual void SetRange(double min, double max)
    { this->SetMinimumValue(min); this->SetMaximumValue(max); };
  virtual void SetRange(const double *range) 
    { this->SetRange(range[0], range[1]); };

  // Description:
  // Set/Get the resolution of the thumbwheel. Moving the thumbwheel will
  // increase/decrease the value by an amount proportional to this resolution.
  virtual void SetResolution(double r);
  vtkGetMacro(Resolution, double);
  
  // Description:
  // Set the interaction modes (mode 0 is left button, 1 is middle, 
  // 2 is right). 
  // Note: set it before setting the balloon help string.
  //BTX
  enum
  {
    InteractionModeNone = 0,
    InteractionModeLinearMotion,
    InteractionModeNonLinearMotion,
    InteractionModeToggleCenterIndicator
  };
  //ETX
  virtual void SetInteractionMode(int mode, int v);
  virtual int GetInteractionMode(int mode);
  virtual void SetInteractionModeToNone(int mode) 
    { this->SetInteractionMode(
      mode, vtkKWThumbWheel::InteractionModeNone); };
  virtual void SetInteractionModeToLinear(int mode) 
    { this->SetInteractionMode(
      mode, vtkKWThumbWheel::InteractionModeLinearMotion); };
  virtual void SetInteractionModeToNonLinear(int mode) 
    { this->SetInteractionMode(
      mode, vtkKWThumbWheel::InteractionModeNonLinearMotion); };
  virtual void SetInteractionModeToToggleCenterIndicator(int mode) 
    { this->SetInteractionMode(
      mode, vtkKWThumbWheel::InteractionModeToggleCenterIndicator); };
  virtual char *GetInteractionModeAsString(int mode);

  // Description:
  // Set/Get the % of the thumbwheel's current width that must be "travelled"
  // by the mouse so that the value is increased/decreased by one resolution 
  // unit (Resolution ivar). Linear mode only.
  // Example: if the threshold is 0.1, the current width is 100 pixels and
  // the resolution is 2, then the mouse must be moved 10 pixels to "the right"
  // to add 2 to the current value.
  vtkSetMacro(LinearThreshold, double);
  vtkGetMacro(LinearThreshold, double);

  // Description:
  // Set/Get the maximum multiplier in non-linear mode. This bounds the 
  // scaling factor applied to the resolution when the thumbwheel is reaching
  // its maximum left or right position.
  vtkSetMacro(NonLinearMaximumMultiplier, double);
  vtkGetMacro(NonLinearMaximumMultiplier, double);

  // Description:
  // Set/Get the width and height of the thumbwheel. Can't be smaller than 5x5.
  virtual void SetThumbWheelWidth(int v);
  vtkGetMacro(ThumbWheelWidth, int);
  virtual void SetThumbWheelHeight(int v);
  vtkGetMacro(ThumbWheelHeight, int);
  virtual void SetThumbWheelSize(int w, int h) 
    { this->SetThumbWheelWidth(w); this->SetThumbWheelHeight(h); };
  virtual void SetLength(int v) { this->SetThumbWheelWidth(v); };

  // Description:
  // Enable/Disable automatic thumbwheel resizing. Turn it off if you want
  // a specific thumbwheel size, otherwise it will resize when its parent
  // widget expands. Note that the ThumbWheelWidth and ThumbWheelHeight ivars
  // are  updated accordingly automatically.
  virtual void SetResizeThumbWheel(int flag);
  vtkGetMacro(ResizeThumbWheel, int);
  vtkBooleanMacro(ResizeThumbWheel, int);

  // Description:
  // Display/Hide a thumbwheel position indicator when the user performs a 
  // motion. This is just a vertical colored bar following the mouse position.
  // Set/Get the indicator color.
  vtkSetMacro(DisplayThumbWheelPositionIndicator, int);
  vtkGetMacro(DisplayThumbWheelPositionIndicator, int);
  vtkBooleanMacro(DisplayThumbWheelPositionIndicator, int);  
  vtkSetVector3Macro(ThumbWheelPositionIndicatorColor, double);
  vtkGetVectorMacro(ThumbWheelPositionIndicatorColor, double, 3);

  // Description:
  // Display/Hide a centrer indicator so that the user can easily find the
  // positive and negative part of the range.
  virtual void SetDisplayThumbWheelCenterIndicator(int flag);
  vtkGetMacro(DisplayThumbWheelCenterIndicator, int);
  vtkBooleanMacro(DisplayThumbWheelCenterIndicator, int);  
  virtual void ToggleDisplayThumbWheelCenterIndicator();

  // Description:
  // Set/Get the average size (in pixels) of the notches on the visible part
  // of the thumbwheel. Can be a decimal value, since it's only used to compute
  // the number of notches to display depending on the current thumbwheel size.
  virtual void SetSizeOfNotches(double v);
  vtkGetMacro(SizeOfNotches, double);

  // Description:
  // Display/Hide an entry field (optional).
  virtual void SetDisplayEntry(int flag);
  vtkGetMacro(DisplayEntry, int);
  vtkBooleanMacro(DisplayEntry, int);  
  vtkGetObjectMacro(Entry, vtkKWEntry);

  // Description:
  // Display/Hide/Set a label (optional).
  virtual void SetDisplayLabel(int flag);
  vtkGetMacro(DisplayLabel, int);
  vtkBooleanMacro(DisplayLabel, int);  
  virtual vtkKWLabel* GetLabel();

  // Description:
  // Set/Get the position of the label and/or entry (on top, or on the side).
  virtual void SetDisplayEntryAndLabelOnTop(int flag);
  vtkGetMacro(DisplayEntryAndLabelOnTop, int);
  vtkBooleanMacro(DisplayEntryAndLabelOnTop, int);  

  // Description:
  // Set/Get the popup mode.
  // WARNING: this mode must be set *before* Create() is called.
  vtkSetMacro(PopupMode, int);
  vtkGetMacro(PopupMode, int);
  vtkBooleanMacro(PopupMode, int);  
  vtkGetObjectMacro(PopupPushButton, vtkKWPushButton);

  // Description:
  // Set/Get the entry expansion flag. This flag is only used if PopupMode 
  // is On. In that case, the default behaviour is to provide a widget as 
  // compact as possible, i.e. the Entry won't be expanded if the widget grows.
  // Set ExpandEntry to On to override this behaviour.
  virtual void SetExpandEntry(int flag);
  vtkGetMacro(ExpandEntry, int);
  vtkBooleanMacro(ExpandEntry, int);  

  // Description:
  // Specifies commands to associate with the widget. 
  // 'Command' is invoked when the widget value is changed.
  // 'StartCommand' is invoked at the beginning of an interaction with
  // the widget.
  // 'EndCommand' is invoked at the end of an interaction with the widget.
  // 'EntryCommand' is invoked when the widget value is changed using
  // the text entry.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - the current value: double
  virtual void SetCommand(vtkObject *object, const char *method);
  virtual void SetStartCommand(vtkObject *object, const char *method);
  virtual void SetEndCommand(vtkObject *object, const char *method);
  virtual void SetEntryCommand(vtkObject *object, const char *method);

  // Description:
  // Setting this string enables balloon help for this widget.
  // Override to pass down to children for cleaner behavior.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Bind/Unbind all components so that values can be changed, but
  // no command will be called.
  void Bind();
  void UnBind();

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
  virtual void ResizeThumbWheelCallback();
  virtual void DisplayPopupCallback();
  virtual void WithdrawPopupCallback();
  virtual void EntryValueCallback(const char*);
  virtual void StartLinearMotionCallback();
  virtual void PerformLinearMotionCallback();
  virtual void StartNonLinearMotionCallback();
  virtual void PerformNonLinearMotionCallback();
  virtual void StopMotionCallback();

protected:
  vtkKWThumbWheel();
  ~vtkKWThumbWheel();

  double      Value;
  double      MinimumValue;
  int         ClampMinimumValue;
  double      MaximumValue;
  int         ClampMaximumValue;
  double      Resolution;
  double      NonLinearMaximumMultiplier;
  double      LinearThreshold;

  int         ThumbWheelWidth;
  int         ThumbWheelHeight;
  double      SizeOfNotches;
  double      ThumbWheelPositionIndicatorColor[3];

  int         ResizeThumbWheel;
  int         DisplayLabel;
  int         DisplayEntry;
  int         DisplayEntryAndLabelOnTop;
  int         DisplayThumbWheelPositionIndicator;
  int         DisplayThumbWheelCenterIndicator;
  int         PopupMode;
  int         ExpandEntry;

  char        *Command;
  char        *StartCommand;
  char        *EndCommand;
  char        *EntryCommand;

  virtual void InvokeThumbWheelCommand(const char *command, double value);
  virtual void InvokeCommand(double value);
  virtual void InvokeStartCommand(double value);
  virtual void InvokeEndCommand(double value);
  virtual void InvokeEntryCommand(double value);

  double      ThumbWheelShift;

  int         InteractionModes[3];

  vtkKWLabel    *ThumbWheel;
  vtkKWEntry    *Entry;
  vtkKWLabel    *Label;
  vtkKWTopLevel *TopLevel;
  vtkKWPushButton *PopupPushButton;

  void CreateEntry();
  void CreateLabel();
  void UpdateThumbWheelImage(double pos = -1.0);
  void PackWidget();
  double GetMousePositionInThumbWheel();

  //BTX

  int State;
  enum WidgetState
  {
    Idle,
    InMotion
  };
  
  class LinearMotionState
  {
  public:
    double Value;
    double ThumbWheelShift;
    double MousePosition;
    int InPerform;
  };

  class NonLinearMotionState
  {
  public:
    double Value;
    double Increment;
    int InPerform;
  };
  //ETX

  LinearMotionState StartLinearMotionState;
  NonLinearMotionState StartNonLinearMotionState;

  int InInvokeCommand;

  void RefreshValue();

private:
  vtkKWThumbWheel(const vtkKWThumbWheel&); // Not implemented
  void operator=(const vtkKWThumbWheel&); // Not implemented
};

#endif

