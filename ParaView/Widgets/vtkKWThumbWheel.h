/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkKWThumbWheel - a thumbwheel widget
// .SECTION Description
// A widget that repsentes a thumbwheel widget with options for 
// a label string and a text entry box.

#ifndef __vtkKWThumbWheel_h
#define __vtkKWThumbWheel_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWLabel;
class vtkKWEntry;
class vtkKWPushButton;

#define VTK_KW_TW_MODE_NONE                    0
#define VTK_KW_TW_MODE_LINEAR_MOTION           1
#define VTK_KW_TW_MODE_NONLINEAR_MOTION        2
#define VTK_KW_TW_MODE_TOGGLE_CENTER_INDICATOR 3

class VTK_EXPORT vtkKWThumbWheel : public vtkKWWidget
{
public:
  static vtkKWThumbWheel* New();
  vtkTypeRevisionMacro(vtkKWThumbWheel,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the value of the thumbwheel.
  virtual void SetValue(float v);
  vtkGetMacro(Value, float);

  // Description:
  // Set/Get the minimum value. The current value will be clamped only if
  // ClampMinimumValue is true.
  vtkSetMacro(MinimumValue, float);
  vtkGetMacro(MinimumValue, float);
  vtkSetMacro(ClampMinimumValue, int);
  vtkGetMacro(ClampMinimumValue, int);
  vtkBooleanMacro(ClampMinimumValue, int);  

  // Description:
  // Set/Get the maximum value. The current value will be clamped only if
  // ClampMaximumValue is true.
  vtkSetMacro(MaximumValue, float);
  vtkGetMacro(MaximumValue, float);
  vtkSetMacro(ClampMaximumValue, int);
  vtkGetMacro(ClampMaximumValue, int);
  vtkBooleanMacro(ClampMaximumValue, int);  

  // Description:
  // Set/Get the resolution of the thumbwheel. Moving the thumbwheel will
  // increase/decrease the value by an amount proportional to this resolution.
  virtual void SetResolution(float r);
  vtkGetMacro(Resolution, float);
  
  // Description:
  // Set the interaction modes (mode 0 is left button, 1 is middle, 2 is right). 
  // Note: set it before setting the balloon help string.
  virtual void SetInteractionMode(int mode, int v);
  virtual int GetInteractionMode(int mode);
  virtual void SetInteractionModeToNone(int mode) 
    { this->SetInteractionMode(mode, VTK_KW_TW_MODE_NONE); };
  virtual void SetInteractionModeToLinear(int mode) 
    { this->SetInteractionMode(mode, VTK_KW_TW_MODE_LINEAR_MOTION); };
  virtual void SetInteractionModeToNonLinear(int mode) 
    { this->SetInteractionMode(mode, VTK_KW_TW_MODE_NONLINEAR_MOTION); };
  virtual void SetInteractionModeToToggleCenterIndicator(int mode) 
    { this->SetInteractionMode(mode, VTK_KW_TW_MODE_TOGGLE_CENTER_INDICATOR); };
  virtual char *GetInteractionModeAsString(int mode);

  // Description:
  // Set/Get the % of the thumbwheel's current width that must be "travelled"
  // by the mouse so that the value is increased/decreased by one resolution 
  // unit (Resolution ivar). Linear mode only.
  // Example: if the threshold is 0.1, the current width is 100 pixels and
  // the resolution is 2, then the mouse must be moved 10 pixels to "the right"
  // to add 2 to the current value.
  vtkSetMacro(LinearThreshold, float);
  vtkGetMacro(LinearThreshold, float);

  // Description:
  // Set/Get the maximum multiplier in non-linear mode. This bounds the 
  // scaling factor applied to the resolution when the thumbwheel is reaching
  // its maximum left or right position.
  vtkSetMacro(NonLinearMaximumMultiplier, float);
  vtkGetMacro(NonLinearMaximumMultiplier, float);

  // Description:
  // Set/Get the width and height of the thumbwheel. Can't be smaller than 5x5.
  virtual void SetThumbWheelWidth(int v);
  vtkGetMacro(ThumbWheelWidth, int);
  virtual void SetThumbWheelHeight(int v);
  vtkGetMacro(ThumbWheelHeight, int);
  virtual void SetThumbWheelSize(int w, int h) 
    { this->SetThumbWheelWidth(w); this->SetThumbWheelHeight(h); };

  // Description:
  // Enable/Disable automatic thumbwheel resizing. Turn it off if you want
  // a specific thumbwheel size, otherwise it will resize when its parent
  // widget expands. Note that the ThumbWheelWidth and ThumbWheelHeight ivars
  // are  updated accordingly automatically.
  virtual void SetResizeThumbWheel(int flag);
  vtkGetMacro(ResizeThumbWheel, int);
  vtkBooleanMacro(ResizeThumbWheel, int);
  void ResizeThumbWheelCallback();

  // Description:
  // Display/Hide a thumbwheel position indicator when the user performs a 
  // motion. This is just a vertical colored bar following the mouse position.
  // Set/Get the indicator color.
  vtkSetMacro(DisplayThumbWheelPositionIndicator, int);
  vtkGetMacro(DisplayThumbWheelPositionIndicator, int);
  vtkBooleanMacro(DisplayThumbWheelPositionIndicator, int);  
  vtkSetVector3Macro(ThumbWheelPositionIndicatorColor, float);
  vtkGetVectorMacro(ThumbWheelPositionIndicatorColor, float, 3);

  // Description:
  // Display/Hide a centrer indicator so that the user can easily find the
  // positive and negative part of the range.
  virtual void SetDisplayThumbWheelCenterIndicator(int flag);
  vtkGetMacro(DisplayThumbWheelCenterIndicator, int);
  vtkBooleanMacro(DisplayThumbWheelCenterIndicator, int);  
  virtual void ToggleDisplayThumbWheelCenterIndicator();

  // Description:
  // Set/Get the average size (in pixels) of the notches on the visible part of 
  // the thumbwheel. Can be a decimal value, since it's only used to compute
  // the number of notches to display depending on the current thumbwheel size.
  virtual void SetSizeOfNotches(float v);
  vtkGetMacro(SizeOfNotches, float);

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
  virtual void SetLabel(const char *label);
  vtkGetObjectMacro(Label, vtkKWLabel);

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
  void DisplayPopupCallback();
  void WithdrawPopupCallback();
  vtkGetObjectMacro(PopupPushButton, vtkKWPushButton);

  // Description:
  // Set/Get the entry expansion flag. This flag is only used if PopupMode 
  // is On. In that case, the default behaviour is to provide a widget as compact
  // as possible, i.e. the Entry won't be expanded if the widget grows. Set
  // ExpandEntry to On to override this behaviour.
  virtual void SetExpandEntry(int flag);
  vtkGetMacro(ExpandEntry, int);
  vtkBooleanMacro(ExpandEntry, int);  

  // Description:
  // Set the callback functions on objects.  The first argument is
  // the KWObject that will have the method called on it.  The second is the
  // name of the method to be called and any arguments in string form.
  // The call is done via Tcl wrappers for the KWObject.
  virtual void SetCommand(vtkKWObject* Object, const char *arg);
  virtual void SetStartCommand(vtkKWObject* Object, const char *arg);
  virtual void SetEndCommand(vtkKWObject* Object, const char *arg);
  virtual void SetEntryCommand (vtkKWObject* Object, const char *arg);
  virtual void InvokeCommand();
  virtual void InvokeStartCommand();
  virtual void InvokeEndCommand();
  virtual void InvokeEntryCommand();

  // Description:
  // Setting this string enables balloon help for this widget.
  // Override to pass down to children for cleaner behavior.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Bind/Unbind all components so that values can be changed, but
  // no command will be called.
  void Bind();
  void UnBind();

  // Description:
  // Methods that gets invoked when the value has changed
  // or motion is started/end
  virtual void EntryCallback();
  virtual void StartLinearMotionCallback();
  virtual void PerformLinearMotionCallback();
  virtual void StartNonLinearMotionCallback();
  virtual void PerformNonLinearMotionCallback();
  virtual void StopMotionCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWThumbWheel();
  ~vtkKWThumbWheel();

  float       Value;
  float       MinimumValue;
  int         ClampMinimumValue;
  float       MaximumValue;
  int         ClampMaximumValue;
  float       Resolution;
  float       NonLinearMaximumMultiplier;
  float       LinearThreshold;

  int         ThumbWheelWidth;
  int         ThumbWheelHeight;
  float       SizeOfNotches;
  float       ThumbWheelPositionIndicatorColor[3];

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

  float       ThumbWheelShift;

  int         InteractionModes[3];

  vtkKWWidget *ThumbWheel;
  vtkKWEntry  *Entry;
  vtkKWLabel  *Label;
  vtkKWWidget *TopLevel;
  vtkKWPushButton *PopupPushButton;

  void CreateEntry();
  void CreateLabel();
  void UpdateThumbWheelImage(float pos = -1.0);
  void PackWidget();
  void UpdateEntryResolution();
  float GetMousePositionInThumbWheel();

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
    float Value;
    float ThumbWheelShift;
    float MousePosition;
    int InPerform;
  };

  class NonLinearMotionState
  {
  public:
    float Value;
    float Increment;
    int InPerform;
  };
  //ETX

  LinearMotionState StartLinearMotionState;
  NonLinearMotionState StartNonLinearMotionState;

  int InInvokeCommand;

private:
  vtkKWThumbWheel(const vtkKWThumbWheel&); // Not implemented
  void operator=(const vtkKWThumbWheel&); // Not implemented
};

#endif

