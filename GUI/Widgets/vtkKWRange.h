/*=========================================================================

  Module:    vtkKWRange.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWRange - a range widget
// .SECTION Description
// A widget that represents a range within a bigger range.
// Note: As a subclass of vtkKWWidgetWithLabel, it inherits a label and methods
// to set its position and visibility. Note that the default label position 
// implemented in this class is on the top of the range if the range 
// direction is horizontal, on the left if is is vertical. Specific positions
// listed in vtkKWWidgetWithLabel are supported as well.
// .SECTION See Also
// vtkKWWidgetWithLabel

#ifndef __vtkKWRange_h
#define __vtkKWRange_h

#include "vtkKWWidgetWithLabel.h"

class vtkKWCanvas;
class vtkKWEntry;
class vtkKWFrame;
class vtkKWPushButtonSet;

class KWWidgets_EXPORT vtkKWRange : public vtkKWWidgetWithLabel
{
public:
  static vtkKWRange* New();
  vtkTypeRevisionMacro(vtkKWRange,vtkKWWidgetWithLabel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set/Get the whole range.
  vtkGetVector2Macro(WholeRange, double);
  virtual void SetWholeRange(double r0, double r1);
  virtual void SetWholeRange(const double range[2]) 
    { this->SetWholeRange(range[0], range[1]); };

  // Description:
  // Set/Get the current (sub-)range.
  vtkGetVector2Macro(Range, double);
  virtual void SetRange(double r0, double r1);
  virtual void SetRange(const double range[2]) 
    { this->SetRange(range[0], range[1]); };

  // Description:
  // Set/Get the current (sub-)range as relative positions in the whole range.
  virtual void GetRelativeRange(double &r0, double &r1);
  virtual void GetRelativeRange(double range[2])
    { this->GetRelativeRange(range[0], range[1]); };
  virtual void SetRelativeRange(double r0, double r1);
  virtual void SetRelativeRange(const double range[2])
    { this->SetRelativeRange(range[0], range[1]); };
  
  // Description:
  // Set/Get the resolution of the slider.
  // The whole range and sub range are not snapped to this resolution.
  // Both ranges can be set to any floating point number. 
  // Think of the sliders and the resolution as a way to set the bounds of
  // the sub range interactively using nice clean steps (power of 10 for 
  // example).
  // The entries associated to the sub range can be used to set the bounds to 
  // anything within the whole range, despite the resolution, allowing the user
  // to enter precise values that could not be reached given the resolution.
  // Of course, given a whole range of 1 to 64, if the resolution is set to 3
  // the slider will only snap to values ranging from 3 to 63 (within the 
  // whole range constraint), but the entries can be used to set accurate
  // values out of the resolution (i.e., 1, 2... 64).
  virtual void SetResolution(double r);
  vtkGetMacro(Resolution, double);

  // Description:
  // Adjust the resolution automatically (to a power of 10 in this implem)
  virtual void SetAdjustResolution(int);
  vtkBooleanMacro(AdjustResolution, int);
  vtkGetMacro(AdjustResolution, int);
  
  // Description:
  // Set/Get the orientation.
  //BTX
  enum 
  {
    OrientationHorizontal = 0,
    OrientationVertical   = 1
  };
  //ETX
  virtual void SetOrientation(int);
  vtkGetMacro(Orientation, int);
  virtual void SetOrientationToHorizontal()
    { this->SetOrientation(vtkKWRange::OrientationHorizontal); };
  virtual void SetOrientationToVertical() 
    { this->SetOrientation(vtkKWRange::OrientationVertical); };

  // Description:
  // Set/Get the order of the sliders (inverted means that the first slider
  // will be associated to Range[1], the last to Range[0])
  virtual void SetInverted(int);
  vtkBooleanMacro(Inverted, int);
  vtkGetMacro(Inverted, int);

  // Description:
  // Set/Get the desired narrow dimension of the widget. For horizontal widget
  // this is the widget height, for vertical this is the width.
  // In the current implementation, this controls the sliders narrow dim.
  virtual void SetThickness(int);
  vtkGetMacro(Thickness, int);
  
  // Description:
  // Set/Get the desired narrow dimension of the internal widget as a fraction
  // of the thickness of the widget (see Thickness). 
  // In the current implementation, this controls the range bar narrow dim.
  virtual void SetInternalThickness(double);
  vtkGetMacro(InternalThickness, double);
  
  // Description:
  // Set/Get the long dimension of the widget. For horizontal widget
  // this is the widget width, for vertical this is the height.
  // Set it to zero (default) to ignore it and let the widget
  // resize.
  virtual void SetRequestedLength(int);
  vtkGetMacro(RequestedLength, int);
  
  // Description:
  // Set/Get the slider size.
  virtual void SetSliderSize(int);
  vtkGetMacro(SliderSize, int);
  
  // Description:
  // Set/Get if a slider can push another slider when bumping into it
  vtkSetMacro(SliderCanPush, int);
  vtkBooleanMacro(SliderCanPush, int);
  vtkGetMacro(SliderCanPush, int);

  // Description:
  // Set/Get the (sub) range scale color. 
  // Defaults to -1, -1, -1: a shade of the widget background color will
  // be used at runtime.
  vtkGetVector3Macro(RangeColor, double);
  virtual void SetRangeColor(double r, double g, double b);
  virtual void SetRangeColor(double rgb[3])
    { this->SetRangeColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the (sub) range scale interaction color. Used when interaction
  // is performed using the sliders.
  // IF set to -1, -1, -1: a shade of the widget background color will
  // be used at runtime.
  vtkGetVector3Macro(RangeInteractionColor, double);
  virtual void SetRangeInteractionColor(double r, double g, double b);
  virtual void SetRangeInteractionColor(double rgb[3])
    { this->SetRangeInteractionColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the entries visibility.
  virtual void SetEntriesVisibility(int);
  vtkBooleanMacro(EntriesVisibility, int);
  vtkGetMacro(EntriesVisibility, int);

  // Description:
  // Get the entries object.
  virtual vtkKWEntry* GetEntry1()
    { return this->Entries[0]; };
  virtual vtkKWEntry* GetEntry2()
    { return this->Entries[1]; };

  // Description:
  // Set/Get the entries width (in chars).
  virtual void SetEntriesWidth(int width);
  vtkGetMacro(EntriesWidth, int);

  // Description:
  // Set/Get the position of the entries (Default is top if the range 
  // direction is horizontal, left if it is vertical).
  // Note that you can also set the label position using the superclass
  // methods (vtkKWWidgetWithLabel).
  //BTX
  enum
  {
    EntryPositionDefault = 0,
    EntryPositionTop,
    EntryPositionBottom,
    EntryPositionLeft,
    EntryPositionRight
  };
  //ETX
  virtual void SetEntry1Position(int);
  vtkGetMacro(Entry1Position, int);
  virtual void SetEntry1PositionToDefault()
    { this->SetEntry1Position(vtkKWRange::EntryPositionDefault); };
  virtual void SetEntry1PositionToTop()
    { this->SetEntry1Position(vtkKWRange::EntryPositionTop); };
  virtual void SetEntry1PositionToBottom()
    { this->SetEntry1Position(vtkKWRange::EntryPositionBottom); };
  virtual void SetEntry1PositionToLeft()
    { this->SetEntry1Position(vtkKWRange::EntryPositionLeft); };
  virtual void SetEntry1PositionToRight()
    { this->SetEntry1Position(vtkKWRange::EntryPositionRight); };
  virtual void SetEntry2Position(int);
  vtkGetMacro(Entry2Position, int);
  virtual void SetEntry2PositionToDefault()
    { this->SetEntry2Position(vtkKWRange::EntryPositionDefault); };
  virtual void SetEntry2PositionToTop()
    { this->SetEntry2Position(vtkKWRange::EntryPositionTop); };
  virtual void SetEntry2PositionToBottom()
    { this->SetEntry2Position(vtkKWRange::EntryPositionBottom); };
  virtual void SetEntry2PositionToLeft()
    { this->SetEntry2Position(vtkKWRange::EntryPositionLeft); };
  virtual void SetEntry2PositionToRight()
    { this->SetEntry2Position(vtkKWRange::EntryPositionRight); };

  // Description:
  // Specifies commands to associate with the widget. 
  // 'Command' is invoked when the widget value is changing (i.e. during
  // user interaction).
  // 'StartCommand' is invoked at the beginning of a user interaction with
  // the widget (when a mouse button is pressed over the widget for example).
  // 'EndCommand' is invoked at the end of the user interaction with the 
  // widget (when the mouse button is released for example).
  // 'EntriesCommand' is invoked when the widget value is changed using
  // the text entries.
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
  // - the current range: int, int (if Resolution is integer); double, double
  //   otherwise.
  //   Note: the 'int' signature is for convenience, so that the command can
  //   be set to a callback accepting 'int'. In doubt, implement the callback
  //   using a 'double' signature that will accept both 'int' and 'double'.
  virtual void SetCommand(vtkObject *object, const char *method);
  virtual void SetStartCommand(vtkObject *object, const char *method);
  virtual void SetEndCommand(vtkObject *object, const char *method);
  virtual void SetEntriesCommand(vtkObject *object, const char *method);

  // Description:
  // Set/Get whether the above commands should be called or not.
  // This allow you to disable the commands while you are setting the range
  // value for example.
  vtkSetMacro(DisableCommands, int);
  vtkGetMacro(DisableCommands, int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Events. The RangeValueChangingEvent is triggered when the widget value
  // is changed (i.e., during user interaction on the widget's slider),
  // the RangeValueStartChangingEvent is invoked at the beginning of an 
  // interaction with the widget, the RangeValueChangedEvent is invoked at the
  // end of an interaction with the widget (or when the value is changed
  // using the entries widget). They are similar in concept as
  // the 'Command', 'StartCommand', 'EndCommand' and 'EntriesCommand' callbacks
  // but can be used by multiple listeners/observers at a time.
  // The following parameters are also passed as client data:
  // - the current range: double, double.
  //BTX
  enum
  {
    RangeValueChangingEvent = 10000,
    RangeValueChangedEvent,
    RangeValueStartChangingEvent
  };
  //ETX

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);

  // Description:
  // Access to the canvas
  vtkGetObjectMacro(Canvas, vtkKWCanvas);

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
  vtkSetMacro(ClampRange, int);
  vtkGetMacro(ClampRange, int);
  vtkBooleanMacro(ClampRange, int);

  // Description:
  // Callbacks. Internal, do not use.
  //BTX
  enum
  {
    SliderIndex0 = 0,
    SliderIndex1 = 1
  };
  //ETX
  virtual void ConfigureCallback();
  virtual void MaximizeRangeCallback();
  virtual void EnlargeRangeCallback();
  virtual void ShrinkRangeCallback();
  virtual void EntriesUpdateCallback(int i);
  virtual void StartInteractionCallback(int x, int y);
  virtual void EndInteractionCallback();
  virtual void SliderMotionCallback(int slider_idx, int x, int y);
  virtual void RangeMotionCallback(int x, int y);

protected:
  vtkKWRange();
  ~vtkKWRange();

  double WholeRange[2];
  double Range[2];
  double WholeRangeAdjusted[2];
  double RangeAdjusted[2];
  double Resolution;
  int   AdjustResolution;
  int   Inverted;
  int   Thickness;
  double InternalThickness;
  int   RequestedLength;
  int   Orientation;
  int   DisableCommands;
  int   SliderSize;
  double RangeColor[3];
  double RangeInteractionColor[3];
  int   EntriesVisibility;
  int   Entry1Position;
  int   Entry2Position;
  int   EntriesWidth;
  int   SliderCanPush;

  int   InInteraction;
  int   StartInteractionPos;
  double StartInteractionRange[2];

  int ClampRange;

  char  *Command;
  char  *StartCommand;
  char  *EndCommand;
  char  *EntriesCommand;

  virtual void InvokeRangeCommand(const char *command, double r0, double r1);
  virtual void InvokeCommand(double r0, double r1);
  virtual void InvokeStartCommand(double r0, double r1);
  virtual void InvokeEndCommand(double r0, double r1);
  virtual void InvokeEntriesCommand(double r0, double r1);

  vtkKWFrame         *CanvasFrame;
  vtkKWCanvas        *Canvas;
  vtkKWEntry         *Entries[2];

  virtual void CreateEntries();
  virtual void UpdateEntriesValue(double range[2]);
  virtual void ConstrainResolution();

  // Description:
  // Bind/Unbind all components.
  virtual void Bind();
  virtual void UnBind();

  // Description:
  // Make sure all elements are constrained correctly
  virtual void ConstrainRangeToResolution(double range[2], int adjust = 1);
  virtual void ConstrainRangeToWholeRange(
    double range[2], double whole_range[2], double *old_range_hint = 0);
  virtual void ConstrainWholeRange();
  virtual void ConstrainRange(double *old_range_hint = 0);
  virtual void ConstrainRanges();

  // Description:
  // Pack the widget
  virtual void Pack();

  // Description:
  // Get element colors (and shades)
  //BTX
  enum
  {
    DarkShadowColor,
    LightShadowColor,
    BackgroundColor,
    HighlightColor
  };
  //ETX
  virtual void GetWholeRangeColor(int type, double &r, double &g, double &b);
  virtual void GetRangeColor(int type, double &r, double &g, double &b);
  virtual void GetSliderColor(int type, double &r, double &g, double &b);

  // Description:
  // Redraw elements
  virtual void RedrawCanvas();
  virtual void RedrawWholeRange();
  virtual void RedrawRange();
  virtual void RedrawSliders();
  virtual void RedrawSlider(int x, int slider_idx);
  virtual void UpdateRangeColors();
  virtual void UpdateColors();

  // Description:
  // Look for a tag
  virtual int HasTag(const char *tag, const char *suffix = 0);

  // Description:
  // Get the current sliders center positions
  virtual void GetSlidersPositions(int pos[2]);

private:
  vtkKWRange(const vtkKWRange&); // Not implemented
  void operator=(const vtkKWRange&); // Not implemented
};

#endif

