/*=========================================================================

  Module:    vtkKWParameterValueFunctionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWParameterValueFunctionEditor - a parameter/value function editor
// .SECTION Description
// A widget that allows the user to edit a parameter/value function.
// Keybindings: Delete or x, Home, End, PageUp or p, PageDown or n, 

#ifndef __vtkKWParameterValueFunctionEditor_h
#define __vtkKWParameterValueFunctionEditor_h

#include "vtkKWParameterValueFunctionInterface.h"
//BTX
#include "vtkKWHistogram.h"
//ETX

#define VTK_KW_PVFE_SELECTED_TAG             "selected_tag"
#define VTK_KW_PVFE_POINT_TAG                "point_tag"
#define VTK_KW_PVFE_POINT_GUIDELINE_TAG      "point_guideline_tag"
#define VTK_KW_PVFE_LINE_TAG                 "line_tag"
#define VTK_KW_PVFE_TEXT_TAG                 "text_tag"
#define VTK_KW_PVFE_FUNCTION_TAG             "function_tag"
#define VTK_KW_PVFE_HISTOGRAM_TAG            "histogram_tag"
#define VTK_KW_PVFE_FRAME_FG_TAG             "framefg_tag"
#define VTK_KW_PVFE_FRAME_BG_TAG             "framebg_tag"
#define VTK_KW_PVFE_PARAMETER_CURSOR_TAG     "cursor_tag"
#define VTK_KW_PVFE_PARAMETER_TICKS_TAG      "p_ticks_tag"
#define VTK_KW_PVFE_VALUE_TICKS_TAG          "v_ticks_tag"

class vtkCallbackCommand;
class vtkKWCanvas;
class vtkKWFrame;
class vtkKWIcon;
class vtkKWLabel;
class vtkKWLabeledEntry;
class vtkKWRange;
class vtkKWOptionMenu;

//BTX
class ostrstream;
//ETX

class VTK_EXPORT vtkKWParameterValueFunctionEditor : public vtkKWParameterValueFunctionInterface
{
public:
  vtkTypeRevisionMacro(vtkKWParameterValueFunctionEditor,vtkKWParameterValueFunctionInterface);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the whole parameter range.
  virtual double* GetWholeParameterRange();
  virtual void SetWholeParameterRange(double r0, double r1);
  virtual void GetWholeParameterRange(double &r0, double &r1)
    { r0 = this->GetWholeParameterRange()[0]; 
    r1 = this->GetWholeParameterRange()[1]; }
  virtual void GetWholeParameterRange(double range[2])
    { this->GetWholeParameterRange(range[0], range[1]); };
  virtual void SetWholeParameterRange(double range[2]) 
    { this->SetWholeParameterRange(range[0], range[1]); };
  
  // Description:
  // Set/Get the visible parameter range in the editor.
  // This is the portion of the whole parameter range that is currently
  // visible (zoomed).
  virtual double* GetVisibleParameterRange();
  virtual void SetVisibleParameterRange(double r0, double r1);
  virtual void GetVisibleParameterRange(double &r0, double &r1)
    { r0 = this->GetVisibleParameterRange()[0]; 
    r1 = this->GetVisibleParameterRange()[1]; }
  virtual void GetVisibleParameterRange(double range[2])
    { this->GetVisibleParameterRange(range[0], range[1]); };
  virtual void SetVisibleParameterRange(double range[2]) 
    { this->SetVisibleParameterRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible parameter range in the editor as relative positions
  // in the whole parameter range.
  virtual void SetRelativeVisibleParameterRange(double r0, double r1);
  virtual void GetRelativeVisibleParameterRange(double &r0, double &r1);
  virtual void GetRelativeVisibleParameterRange(double range[2])
    { this->GetRelativeVisibleParameterRange(range[0], range[1]); };
  virtual void SetRelativeVisibleParameterRange(double range[2]) 
    { this->SetRelativeVisibleParameterRange(range[0], range[1]); };

  // Description:
  // Convenience method to set the whole parameter range while maintaining
  // the same relative visible parameter range.
  virtual void SetWholeParameterRangeAndMaintainVisible(double r0, double r1);
  virtual void SetWholeParameterRangeAndMaintainVisible(double range[2]) 
    { this->SetWholeParameterRangeAndMaintainVisible(range[0], range[1]); };

  // Description:
  // Set/Get the whole value range.
  virtual double* GetWholeValueRange();
  virtual void SetWholeValueRange(double r0, double r1);
  virtual void GetWholeValueRange(double &r0, double &r1)
    { r0 = this->GetWholeValueRange()[0]; 
    r1 = this->GetWholeValueRange()[1]; }
  virtual void GetWholeValueRange(double range[2])
    { this->GetWholeValueRange(range[0], range[1]); };
  virtual void SetWholeValueRange(double range[2]) 
    { this->SetWholeValueRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible value range.
  // This is the portion of the whole value range that is currently
  // visible (zoomed).
  virtual double* GetVisibleValueRange();
  virtual void SetVisibleValueRange(double r0, double r1);
  virtual void GetVisibleValueRange(double &r0, double &r1)
    { r0 = this->GetVisibleValueRange()[0]; 
    r1 = this->GetVisibleValueRange()[1]; }
  virtual void GetVisibleValueRange(double range[2])
    { this->GetVisibleValueRange(range[0], range[1]); };
  virtual void SetVisibleValueRange(double range[2]) 
    { this->SetVisibleValueRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible value range in the editor as relative positions
  // in the whole value range.
  virtual void SetRelativeVisibleValueRange(double r0, double r1);
  virtual void GetRelativeVisibleValueRange(double &r0, double &r1);
  virtual void GetRelativeVisibleValueRange(double range[2])
    { this->GetRelativeVisibleValueRange(range[0], range[1]); };
  virtual void SetRelativeVisibleValueRange(double range[2]) 
    { this->SetRelativeVisibleValueRange(range[0], range[1]); };

  // Description:
  // Convenience method to set the whole value range while maintaining
  // the same relative visible value range.
  virtual void SetWholeValueRangeAndMaintainVisible(double r0, double r1);
  virtual void SetWholeValueRangeAndMaintainVisible(double range[2]) 
    { this->SetWholeValueRangeAndMaintainVisible(range[0], range[1]); };

  // Description:
  // Show the label at the default position (on the same line as all
  // other elements), or on top on its own line, or on the left of the
  // whole editor, on the same line as the canvas.
  // The superclass ShowLabel still has to be On for the label to be
  // shown.
  // Use the superclass: 
  //   - SetLabel() to set the label string, 
  //   - GetLabel() to get the widget,
  //   - SetShowLabel(), ShowLabelOn(), ShowLabelOff() to show/hide the label.
  // Note: set ShowLabel to the proper value before calling Create() in order
  // to minimize the footprint of the object.
  virtual void SetShowLabel(int);
  //BTX
  enum
  {
    LabelPositionAtDefault = 10,
    LabelPositionAtTop,
    LabelPositionAtLeft
  };
  //ETX
  virtual void SetLabelPosition(int);
  vtkGetMacro(LabelPosition, int);

  // Description:
  // Show the parameter range UI (the slider).
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowParameterRange, int);
  virtual void SetShowParameterRange(int);
  vtkGetMacro(ShowParameterRange, int);

  // Description:
  // Show the value range UI (the slider).
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowValueRange, int);
  virtual void SetShowValueRange(int);
  vtkGetMacro(ShowValueRange, int);

  // Description:
  // Access to the ranges (the sliders).
  // Note: use those methods to modify the aspect the ranges. Do not modify
  // the value of the ranges themselves, use the API below instead.
  vtkGetObjectMacro(ParameterRange, vtkKWRange);
  vtkGetObjectMacro(ValueRange, vtkKWRange);

  // Description:
  // Set/Get the displayed whole parameter range. As if things were not 
  // complicated enough, this method allows you to set the whole parameter
  // range that will be used instead of the WholeParameterRange for UI
  // elements that display information related to the parameter range
  // (i.e., the parameter range label and the parameter range entry). This
  // allows you to use a function set in a different internal range than 
  // the one you want to display. This works by mapping the relative position
  // of the VisibleParameterRange inside the WholeParameterRange to
  // the DisplayedWholeParameterRange (the GetDisplayedVisibleParameterRange
  // is a convenience function that will return that result).
  // If both ends of that range are the same, it is not used.
  vtkGetVector2Macro(DisplayedWholeParameterRange, double);
  virtual void SetDisplayedWholeParameterRange(double r0, double r1);
  virtual void SetDisplayedWholeParameterRange(double range[2]) 
    { this->SetDisplayedWholeParameterRange(range[0], range[1]); };
  virtual void GetDisplayedVisibleParameterRange(double &r0, double &r1);
  virtual void GetDisplayedVisibleParameterRange(double range[2])
    { this->GetDisplayedVisibleParameterRange(range[0], range[1]); };
  
  // Description:
  // Set the position of points in the value range. 
  // Default is PointPositionValue, i.e. if the point value is
  // mono-dimensional, its vertical position in the canvas will be computed
  // from its value relative to the whole value range. If PositionAtCenter 
  // or if the point value is multi-dimensional, the point is centered
  // vertically.
  //BTX
  enum 
  {
    PointPositionAtValue = 0,
    PointPositionAtTop,
    PointPositionAtBottom,
    PointPositionAtCenter
  };
  //ETX
  virtual void SetPointPositionInValueRange(int);
  vtkGetMacro(PointPositionInValueRange, int);

  // Description:
  // Show the range label UI.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowRangeLabel, int);
  virtual void SetShowRangeLabel(int);
  vtkGetMacro(ShowRangeLabel, int);

  // Description:
  // Show the range label at the default position (on the same line as all
  // other elements), or on top on its own line.
  // The ShowRangeLabel parameter still has to be On for the label to be
  // shown.
  //BTX
  enum
  {
    RangeLabelPositionAtDefault = 10,
    RangeLabelPositionAtTop
  };
  //ETX
  virtual void SetRangeLabelPosition(int);
  vtkGetMacro(RangeLabelPosition, int);

  // Description:
  // Show the parameter entry UI.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowParameterEntry, int);
  virtual void SetShowParameterEntry(int);
  vtkGetMacro(ShowParameterEntry, int);

  // Description:
  // Show the parameter entry at the default position (on the same line as all
  // other elements), or on the right of the canvas.
  // The ShowParameterEntry parameter still has to be On for the entry to be
  // shown.
  //BTX
  enum
  {
    ParameterEntryPositionAtDefault = 10,
    ParameterEntryPositionAtRight
  };
  //ETX
  virtual void SetParameterEntryPosition(int);
  vtkGetMacro(ParameterEntryPosition, int);

  // Description:
  // Set/Get the parameter entry printf format. If not NULL, it is
  // applied to the displayed parameter value before assigning it to
  // the parameter entry.
  virtual void SetParameterEntryFormat(const char *);
  vtkGetStringMacro(ParameterEntryFormat);

  // Description:
  // Access the entry
  // If you need to customize this object, make sure you first set 
  // ShowParameterEntry to On and call Create().
  vtkGetObjectMacro(ParameterEntry, vtkKWLabeledEntry);

  // Description:
  // Show the user frame UI.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowUserFrame, int);
  virtual void SetShowUserFrame(int);
  vtkGetMacro(ShowUserFrame, int);

  // Description:
  // Access the user frame
  // If you need to add elements to the user-frame, make sure you first set 
  // ShowUserFrame to On and call Create().
  vtkGetObjectMacro(UserFrame, vtkKWFrame);

  // Description:
  // Set/Get the canvas width/height in pixels (i.e. the drawable region)
  // If ExpandCanvasWidth is On, the canvas will expand automatically to
  // accomodate its parent. This mechanism does not behave as expected 
  // sometimes, in that case set ExpandCanvasWidth to Off and CanvasWidth to
  // the proper value
  virtual void SetCanvasHeight(int);
  virtual void SetCanvasWidth(int);
  vtkGetMacro(CanvasHeight, int);
  vtkGetMacro(CanvasWidth, int);
  vtkBooleanMacro(ExpandCanvasWidth, int);
  virtual void SetExpandCanvasWidth(int);
  vtkGetMacro(ExpandCanvasWidth, int);
  
  // Description:
  // Show/Hide the function line 
  // (i.e, if set to Off, only the points are shown).
  vtkBooleanMacro(ShowFunctionLine, int);
  virtual void SetShowFunctionLine(int);
  vtkGetMacro(ShowFunctionLine, int);

  // Description:
  // Set/Get the line width for the function
  virtual void SetFunctionLineWidth(int);
  vtkGetMacro(FunctionLineWidth, int);
  
  // Description:
  // Set/Get the line style for the function
  //BTX
  enum 
  {
    LineStyleSolid = 0,
    LineStyleDash
  };
  //ETX
  virtual void SetFunctionLineStyle(int);
  vtkGetMacro(FunctionLineStyle, int);

  // Description:
  // Show/Hide the canvas outline
  vtkBooleanMacro(ShowCanvasOutline, int);
  virtual void SetShowCanvasOutline(int);
  vtkGetMacro(ShowCanvasOutline, int);
  
  // Description:
  // Show/Hide the canvas background
  vtkBooleanMacro(ShowCanvasBackground, int);
  virtual void SetShowCanvasBackground(int);
  vtkGetMacro(ShowCanvasBackground, int);
  
  // Description:
  // Show/Hide the parameter cursor. This is a vertical line spanning the
  // whole value range, located at a specific position in the parameter
  // range. Set the position using ParameterCursorPosition.
  vtkBooleanMacro(ShowParameterCursor, int);
  virtual void SetShowParameterCursor(int);
  vtkGetMacro(ShowParameterCursor, int);

  // Description:
  // Set/Get the parameter cursor position (inside the parameter range)
  virtual void SetParameterCursorPosition(double);
  vtkGetMacro(ParameterCursorPosition, double);
  
  // Description:
  // Set/Get the cursor color. 
  vtkGetVector3Macro(ParameterCursorColor, double);
  virtual void SetParameterCursorColor(double r, double g, double b);
  virtual void SetParameterCursorColor(double rgb[3])
    { this->SetParameterCursorColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Show/Hide the parameter ticks
  vtkBooleanMacro(ShowParameterTicks, int);
  virtual void SetShowParameterTicks(int);
  vtkGetMacro(ShowParameterTicks, int);

  // Description:
  // Set/Get the number of parameters ticks.
  virtual void SetNumberOfParameterTicks(int);
  vtkGetMacro(NumberOfParameterTicks, int);

  // Description:
  // Set/Get the parameter ticks printf format.
  virtual void SetParameterTicksFormat(const char *);
  vtkGetStringMacro(ParameterTicksFormat);

  // Description:
  // Show/Hide the value ticks
  vtkBooleanMacro(ShowValueTicks, int);
  virtual void SetShowValueTicks(int);
  vtkGetMacro(ShowValueTicks, int);

  // Description:
  // Set/Get the number of value ticks.
  virtual void SetNumberOfValueTicks(int);
  vtkGetMacro(NumberOfValueTicks, int);

  // Description:
  // Set/Get the width of the value ticks canvas
  virtual void SetValueTicksCanvasWidth(int);
  vtkGetMacro(ValueTicksCanvasWidth, int);

  // Description:
  // Set/Get the value ticks printf format.
  virtual void SetValueTicksFormat(const char *);
  vtkGetStringMacro(ValueTicksFormat);

  // Description:
  // Compute the value ticks using the histogram occurence values
  vtkBooleanMacro(ComputeValueTicksFromHistogram, int);
  virtual void SetComputeValueTicksFromHistogram(int);
  vtkGetMacro(ComputeValueTicksFromHistogram, int);

  // Description:
  // Set/Get the ticks length (in pixels).
  virtual void SetTicksLength(int);
  vtkGetMacro(TicksLength, int);
  
  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Set/Get if the end-points of the function are locked in the parameter
  // space (they can not be removed or can only be moved in the value space).
  vtkSetMacro(LockEndPointsParameter, int);
  vtkBooleanMacro(LockEndPointsParameter, int);
  vtkGetMacro(LockEndPointsParameter, int);

  // Description:
  // Set/Get if moving the end-points of the function will automatically
  // rescale/move all the points in between to keep the relative distance 
  // between points the same in the parameter domain.
  // Note that for convenience reasons, the end-points become
  // immune to deletion.
  vtkSetMacro(RescaleBetweenEndPoints, int);
  vtkBooleanMacro(RescaleBetweenEndPoints, int);
  vtkGetMacro(RescaleBetweenEndPoints, int);

  // Description:
  // Set/Get if points can be added and removed.
  vtkSetMacro(DisableAddAndRemove, int);
  vtkBooleanMacro(DisableAddAndRemove, int);
  vtkGetMacro(DisableAddAndRemove, int);

  // Description:
  // Set/Get the point radius (in pixels).
  virtual void SetPointRadius(int);
  vtkGetMacro(PointRadius, int);

  // Description:
  // Set/Get the selected point radius as a fraction
  // of the point radius (see PointRadius). 
  virtual void SetSelectedPointRadius(double);
  vtkGetMacro(SelectedPointRadius, double);

  // Description:
  // Set/Get the point style for the function points, or specifically
  // for the first or last point (if set to Default, the first or last
  // point will use the same style as the other points, or Disc if that
  // style is set to Default too)
  //BTX
  enum 
  {
    PointStyleDisc = 0,
    PointStyleCursorDown,
    PointStyleCursorUp,
    PointStyleCursorLeft,
    PointStyleCursorRight,
    PointStyleRectangle,
    PointStyleDefault
  };
  //ETX
  virtual void SetPointStyle(int);
  vtkGetMacro(PointStyle, int);
  virtual void SetFirstPointStyle(int);
  vtkGetMacro(FirstPointStyle, int);
  virtual void SetLastPointStyle(int);
  vtkGetMacro(LastPointStyle, int);

  // Description:
  // Set/Get the outline width for the points
  virtual void SetPointOutlineWidth(int);
  vtkGetMacro(PointOutlineWidth, int);

  // Description:
  // Set margin for the canvas to display the points entirely.
  // If set to None, the canvas parameter range will match the 
  // VisibleParameterRange (as a side effect, points on the border of the
  // range will be clipped, only half of them will be displayed, making 
  // selection a bit more difficult). If not, the canvas will also provide 
  // room for each point to be displayed entirely, vertically or horizontally,
  // or both.
  //BTX
  enum 
  {
    PointMarginNone            = 0,
    PointMarginLeftSide        = 1,
    PointMarginRightSide       = 2,
    PointMarginHorizontalSides = 3,
    PointMarginTopSide         = 4,
    PointMarginBottomSide      = 8,
    PointMarginVerticalSides   = 12,
    PointMarginAllSides        = 15,
  };
  //ETX
  vtkBooleanMacro(PointMarginToCanvas, int);
  virtual void SetPointMarginToCanvas(int);
  vtkGetMacro(PointMarginToCanvas, int);

  // Description:
  // Select/Deselect a point, get the selected point (-1 if none selected)
  vtkGetMacro(SelectedPoint, int);
  virtual void SelectPoint(int id);
  virtual void ClearSelection();
  virtual int  HasSelection();
  virtual void SelectNextPoint();
  virtual void SelectPreviousPoint();
  virtual void SelectFirstPoint();
  virtual void SelectLastPoint();

  // Description:
  // Remove a point
  virtual int RemoveSelectedPoint();
  virtual int RemovePoint(int id);
  virtual int RemovePointAtParameter(double parameter);

  // Description:
  // Add a point
  virtual int AddPointAtCanvasCoordinates(int x, int y, int &id);
  virtual int AddPointAtParameter(double parameter, int &id);

  // Description:
  // Merge all the points from another function editor.
  // Return the number of points merged.
  virtual int MergePointsFromEditor(
    vtkKWParameterValueFunctionEditor *editor);

  // Description:
  // Set/Get the background color of the main frame, where the function
  // is drawn. Note that the frame can be smaller than the widget itself
  // depending on the margin requested to draw the points entirely (see
  // PointMarginToCanvas ivar). Use SetBackgroundColor to set the
  // canvas color (i.e., the whole area outside the margin)
  vtkGetVector3Macro(FrameBackgroundColor, double);
  virtual void SetFrameBackgroundColor(double r, double g, double b);
  virtual void SetFrameBackgroundColor(double rgb[3])
    { this->SetFrameBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  virtual void SetBackgroundColor(int r, int g, int b);
  virtual void SetBackgroundColor(double r, double g, double b)
    { this->Superclass::SetBackgroundColor(r, g, b); }
  
  // Description:
  // Set/Get the point color. 
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(PointColor, double);
  virtual void SetPointColor(double r, double g, double b);
  virtual void SetPointColor(double rgb[3])
    { this->SetPointColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the selected point color.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(SelectedPointColor, double);
  virtual void SetSelectedPointColor(double r, double g, double b);
  virtual void SetSelectedPointColor(double rgb[3])
    { this->SetSelectedPointColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the point text color.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(PointTextColor, double);
  virtual void SetPointTextColor(double r, double g, double b);
  virtual void SetPointTextColor(double rgb[3])
    { this->SetPointTextColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the selected point text color.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(SelectedPointTextColor, double);
  virtual void SetSelectedPointTextColor(double r, double g, double b);
  virtual void SetSelectedPointTextColor(double rgb[3])
    { this->SetSelectedPointTextColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set a hint: some colors should be function of the value
  // (might not be supported/implemented in subclasses).
  vtkBooleanMacro(ComputePointColorFromValue, int);
  virtual void SetComputePointColorFromValue(int);
  vtkGetMacro(ComputePointColorFromValue, int);
  
  // Description:
  // Show the point index in the canvas.
  vtkBooleanMacro(ShowPointIndex, int);
  virtual void SetShowPointIndex(int);
  vtkGetMacro(ShowPointIndex, int);

  // Description:
  // Show the point guideline in the canvas 
  // (for ex: a vertical line at each point).
  vtkBooleanMacro(ShowPointGuideline, int);
  virtual void SetShowPointGuideline(int);
  vtkGetMacro(ShowPointGuideline, int);

  // Description:
  // Set/Get the line style for the guideline.
  // See FunctionLineStyle for enumeration of style values.
  virtual void SetPointGuidelineStyle(int);
  vtkGetMacro(PointGuidelineStyle, int);

  // Description:
  // Show the selected point index in the canvas.
  vtkBooleanMacro(ShowSelectedPointIndex, int);
  virtual void SetShowSelectedPointIndex(int);
  vtkGetMacro(ShowSelectedPointIndex, int);

  // Description:
  // Set/Get the histogram and secondary histogram over the parameter range.
  // The primary histogram is drawn in a bar/area style, the secondary
  // one is drawn as dots on top of the primary.
  //BTX
  vtkGetObjectMacro(Histogram, vtkKWHistogram);
  virtual void SetHistogram(vtkKWHistogram*);
  vtkGetObjectMacro(SecondaryHistogram, vtkKWHistogram);
  virtual void SetSecondaryHistogram(vtkKWHistogram*);
  //ETX

  // Description:
  // Set/Get the histogram and secondary histogram color. 
  // Overriden by ComputeHistogramColorFromValue if supported.
  vtkGetVector3Macro(HistogramColor, double);
  virtual void SetHistogramColor(double r, double g, double b);
  virtual void SetHistogramColor(double rgb[3])
    { this->SetHistogramColor(rgb[0], rgb[1], rgb[2]); };
  vtkGetVector3Macro(SecondaryHistogramColor, double);
  virtual void SetSecondaryHistogramColor(double r, double g, double b);
  virtual void SetSecondaryHistogramColor(double rgb[3])
    { this->SetSecondaryHistogramColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set a hint: histogram and secondary histogram colors should be function
  // of the value (might not be supported/implemented in subclasses).
  vtkBooleanMacro(ComputeHistogramColorFromValue, int);
  virtual void SetComputeHistogramColorFromValue(int);
  vtkGetMacro(ComputeHistogramColorFromValue, int);

  // Description:
  // Set/Get the histogram and secondary histogram style
  // (see vtkKWHistogram::ImageDescriptor styles).
  virtual void SetHistogramStyle(int);
  vtkGetMacro(HistogramStyle, int);
  virtual void SetSecondaryHistogramStyle(int);
  vtkGetMacro(SecondaryHistogramStyle, int);

  // Description:
  // Show/Hide the histogram log mode button.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  virtual void SetShowHistogramLogModeOptionMenu(int);
  vtkBooleanMacro(ShowHistogramLogModeOptionMenu, int);
  vtkGetMacro(ShowHistogramLogModeOptionMenu, int);
  virtual void SetHistogramLogModeChangedCommand(
    vtkKWObject* object,const char *method);
  virtual void InvokeHistogramLogModeChangedCommand();

  // Description:
  // Set/Get if the mouse cursor is changed automatically to provide
  // more feedback regarding the interaction (defaults to On).
  vtkBooleanMacro(ChangeMouseCursor, int);
  vtkGetMacro(ChangeMouseCursor, int);
  vtkSetMacro(ChangeMouseCursor, int);

  // Description:
  // Set commands.
  // Point... commands are passed the index of the point that is/was modified.
  // PointRemovedCommand take an additional arg which is the value of 
  // the parameter of the point that was removed.
  // SelectionChanged is called on selection/deselection.
  // FunctionChanged is called when the function was changed (as the
  // result of an interaction which is now over, like point added/(re)moved). 
  // FunctionChanging is called when the function is changing (as the
  // result of an interaction in progress, like moving a point). 
  virtual void SetPointAddedCommand(
    vtkKWObject* object,const char *method);
  virtual void SetPointMovingCommand(
    vtkKWObject* object, const char *method);
  virtual void SetPointMovedCommand(
    vtkKWObject* object, const char *method);
  virtual void SetPointRemovedCommand(
    vtkKWObject* object, const char *method);
  virtual void SetSelectionChangedCommand(
    vtkKWObject* object,const char *method);
  virtual void SetFunctionChangedCommand(
    vtkKWObject* object, const char *method);
  virtual void SetFunctionChangingCommand(
    vtkKWObject* object, const char *method);
  virtual void SetVisibleRangeChangedCommand(
    vtkKWObject* object, const char *method);
  virtual void SetVisibleRangeChangingCommand(
    vtkKWObject* object, const char *method);
  virtual void InvokePointAddedCommand(int id);
  virtual void InvokePointMovingCommand(int id);
  virtual void InvokePointMovedCommand(int id);
  virtual void InvokePointRemovedCommand(int id, double parameter);
  virtual void InvokeSelectionChangedCommand();
  virtual void InvokeFunctionChangedCommand();
  virtual void InvokeFunctionChangingCommand();
  virtual void InvokeVisibleRangeChangedCommand();
  virtual void InvokeVisibleRangeChangingCommand();

  // Description:
  // Set/get whether the above commands should be called or not.
  // This allow you to disable the commands while you are setting the range
  // value for example.
  vtkSetMacro(DisableCommands, int);
  vtkGetMacro(DisableCommands, int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Synchronize the visible parameter range between two editors A and B.
  // Each time the visible range of A is changed, the same visible range
  // is assigned to the synchronized editor B, and vice-versa.
  // Note that a call with (A, B) is the same as a call with (B, A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  static int SynchronizeVisibleParameterRange(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);
  static int DoNotSynchronizeVisibleParameterRange(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize points between two editors A and B.
  // First make sure both editors have the same points in the
  // parameter space (by calling MergePointsFromEditor on each other).
  // Then each time a point in A is added, moved or removed through 
  // user interaction, the same point in B is altered and vice-versa.
  // Note that a call with (A, B) is the same as a call with (B, A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  static int SynchronizePoints(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);
  static int DoNotSynchronizePoints(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize single selection between two editors A and B.
  // Each time a point is selected in A, the selection is cleared in B, 
  // and vice-versa.
  // Note that a call with (A, B) is the same as a call with (B, A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  static int SynchronizeSingleSelection(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);
  static int DoNotSynchronizeSingleSelection(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize same selection between two editors A and B.
  // Each time a point is selected in A, the same point is selected in B, 
  // and vice-versa.
  // Note that a call with (A, B) is the same as a call with (B, A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  static int SynchronizeSameSelection(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);
  static int DoNotSynchronizeSameSelection(
    vtkKWParameterValueFunctionEditor *a,vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Callbacks
  virtual void ConfigureCallback();
  virtual void CanvasEnterCallback();
  virtual void VisibleParameterRangeChangingCallback();
  virtual void VisibleParameterRangeChangedCallback();
  virtual void VisibleValueRangeChangingCallback();
  virtual void VisibleValueRangeChangedCallback();
  virtual void StartInteractionCallback(int x, int y);
  virtual void MovePointCallback(int x, int y, int shift);
  virtual void EndInteractionCallback(int x, int y);
  virtual void ParameterEntryCallback();
  virtual void HistogramLogModeCallback(int mode);

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
  vtkKWParameterValueFunctionEditor();
  ~vtkKWParameterValueFunctionEditor();

  // Description:
  // Is point locked, protected, removable ?
  virtual int FunctionPointCanBeAdded();
  virtual int FunctionPointCanBeRemoved(int id);
  virtual int FunctionPointParameterIsLocked(int id);
  virtual int FunctionPointValueIsLocked(int id);
  virtual int FunctionPointCanBeMovedToParameter(int id, double parameter);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  GetFunctionPointColorInCanvas(int id, double rgb[3]);
  virtual int  GetFunctionPointTextColorInCanvas(int id, double rgb[3]);
  virtual int  GetFunctionPointCanvasCoordinates(int id, int &x, int &y);
  virtual int  AddFunctionPointAtCanvasCoordinates(int x, int y, int &id);
  virtual int  AddFunctionPointAtParameter(double parameter, int &id);
  virtual int  MoveFunctionPointToCanvasCoordinates(int id,int x,int y);
  virtual int  MoveFunctionPointToParameter(int id,double parameter,int i=0);
  virtual int  MoveFunctionPoint(int id,double parameter,const double *values);
  virtual int  EqualFunctionPointValues(const double *values1, const double *values2);

  virtual void UpdatePointEntries(int id);

  // Description:
  // Rescale/move all the points in between the end-points to keep the
  // relative distance the same in the parameter domain. 
  // The id and old parameter position of the
  // point that has been moved (the first or the last) are passed.
  // Note that nothing is redrawn for efficiency reason.
  virtual void  RescaleFunctionBetweenEndPoints(int id, double old_parameter);

  // Description:
  // Internal method to disable all redraws.
  vtkSetMacro(DisableRedraw, int);
  vtkBooleanMacro(DisableRedraw, int);
  vtkGetMacro(DisableRedraw, int);

  int   ShowParameterRange;
  int   ShowValueRange;
  int   PointPositionInValueRange;
  int   CanvasHeight;
  int   CanvasWidth;
  int   ExpandCanvasWidth;
  int   LockEndPointsParameter;
  int   RescaleBetweenEndPoints;
  int   DisableAddAndRemove;
  int   DisableRedraw;
  int   PointRadius;
  double SelectedPointRadius;
  int   FunctionLineWidth;
  int   FunctionLineStyle;
  int   PointGuidelineStyle;
  int   PointOutlineWidth;
  int   PointStyle;
  int   FirstPointStyle;
  int   LastPointStyle;
  int   DisableCommands;
  int   SelectedPoint;
  int   ShowCanvasOutline;
  int   ShowCanvasBackground;
  int   ShowParameterCursor;
  int   ShowFunctionLine;
  int   ShowPointIndex;
  int   ShowPointGuideline;
  int   ShowSelectedPointIndex;
  int   LabelPosition;
  int   ShowRangeLabel;
  int   RangeLabelPosition;
  int   ParameterEntryPosition;
  int   ShowParameterEntry;
  int   ShowUserFrame;
  int   ShowParameterTicks;
  int   ShowValueTicks;
  int   ComputeValueTicksFromHistogram;
  int   PointMarginToCanvas;
  int   TicksLength;
  int   NumberOfParameterTicks;
  int   NumberOfValueTicks;
  int   ValueTicksCanvasWidth;
  int   ChangeMouseCursor;
  char* ValueTicksFormat;
  char* ParameterTicksFormat;
  char* ParameterEntryFormat;
  double ParameterCursorPosition;

  double FrameBackgroundColor[3];
  double ParameterCursorColor[3];
  double PointColor[3];
  double SelectedPointColor[3];
  double PointTextColor[3];
  double SelectedPointTextColor[3];
  int    ComputePointColorFromValue;

  // Commands

  char  *PointAddedCommand;
  char  *PointMovingCommand;
  char  *PointMovedCommand;
  char  *PointRemovedCommand;
  char  *SelectionChangedCommand;
  char  *FunctionChangedCommand;
  char  *FunctionChangingCommand;
  char  *VisibleRangeChangedCommand;
  char  *VisibleRangeChangingCommand;

  virtual void InvokeCommand(const char *command);
  virtual void InvokePointCommand(
    const char *command, int id, const char *extra = 0);

  // GUI

  vtkKWCanvas       *Canvas;
  vtkKWRange        *ParameterRange;
  vtkKWRange        *ValueRange;
  vtkKWFrame        *TopLeftContainer;
  vtkKWFrame        *TopLeftFrame;
  vtkKWFrame        *UserFrame;
  vtkKWFrame        *TopRightFrame;
  vtkKWLabel        *RangeLabel;
  vtkKWLabeledEntry *ParameterEntry;
  vtkKWCanvas       *ValueTicksCanvas;
  vtkKWCanvas       *ParameterTicksCanvas;

  // Histogram

  vtkKWHistogram    *Histogram;
  vtkKWHistogram    *SecondaryHistogram;
  //BTX
  vtkKWHistogram::ImageDescriptor *HistogramImageDescriptor;
  vtkKWHistogram::ImageDescriptor *SecondaryHistogramImageDescriptor;
  //ETX
  double        HistogramColor[3];
  double        SecondaryHistogramColor[3];
  int           ComputeHistogramColorFromValue;
  int           HistogramStyle;
  int           SecondaryHistogramStyle;
  unsigned long LastHistogramBuildTime;
  int           ShowHistogramLogModeOptionMenu;
  char          *HistogramLogModeChangedCommand;

  vtkKWOptionMenu  *HistogramLogModeOptionMenu;
  virtual void CreateHistogramLogModeOptionMenu(vtkKWApplication *app);
  virtual void UpdateHistogramLogModeOptionMenu();

  // Description:
  // Bind/Unbind all widgets.
  virtual void Bind();
  virtual void UnBind();

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateLabel(vtkKWApplication *app);
  virtual void CreateParameterRange(vtkKWApplication *app);
  virtual void CreateValueRange(vtkKWApplication *app);
  virtual void CreateRangeLabel(vtkKWApplication *app);
  virtual void CreateTopRightFrame(vtkKWApplication *app);
  virtual void CreateParameterEntry(vtkKWApplication *app);
  virtual void CreateTopLeftContainer(vtkKWApplication *app);
  virtual void CreateTopLeftFrame(vtkKWApplication *app);
  virtual void CreateUserFrame(vtkKWApplication *app);
  virtual void CreateValueTicksCanvas(vtkKWApplication *app);
  virtual void CreateParameterTicksCanvas(vtkKWApplication *app);
  virtual int IsTopLeftFrameUsed();
  virtual int IsTopRightFrameUsed();

  // Description:
  // Pack the widget
  virtual void Pack();

  // Description:
  // Get the center of a given canvas item (using its item id)
  virtual void GetCanvasItemCenter(int item_id, int &x, int &y);

  // Description:
  // Get the scaling factors used to translate parameter/value to x/y canvas
  // coordinates
  virtual void GetCanvasScalingFactors(double factors[2]);
  virtual void GetCanvasMargin(
    int *margin_left, int *margin_right, int *margin_top, int *margin_bottom);
  virtual void GetCanvasScrollRegion(double *x, double *y, double *x2, double *y2);
  virtual void GetCanvasHorizontalSlidingBounds(
    double p_v_range_ext[2], int bounds[2], int margins[2]);

  // Description:
  // Redraw. Will actually call, if necessary:
  // RedrawSizeDependentElements: the size of the canvas or the extent of its
  //                              ranges have changed
  // RedrawPanDependentElements:  the visible ranges are panned while their
  //                              extents are unchanged
  // RedrawFunctionDependentElements: the function has changed
  virtual void Redraw();
  virtual void RedrawSizeDependentElements();
  virtual void RedrawPanOnlyDependentElements();
  virtual void RedrawFunctionDependentElements();

  // Description:
  // Redraw the whole function or a specific point
  //BTX
  virtual void RedrawFunction();
  virtual void RedrawPoint(int id, ostrstream *tk_cmd = 0);
  //ETX

  // Description:
  // Redraw the visible range frame
  virtual void RedrawRangeFrame();

  // Description:
  // Redraw the visible range ticks
  virtual void RedrawRangeTicks();

  // Description:
  // Redraw the parameter cursor
  virtual void RedrawParameterCursor();

  // Description:
  // Redraw the histogram
  virtual void RedrawHistogram();
  //BTX 
  virtual void UpdateHistogramImageDescriptor(vtkKWHistogram::ImageDescriptor*);
  //ETX

  //BTX
  // Simple class designed to hold previous ranges and optimize
  // the way the editor is refreshed

  class Ranges
  {
  public:
    double WholeParameterRange[2];
    double VisibleParameterRange[2];
    double WholeValueRange[2];
    double VisibleValueRange[2];

    Ranges();
    void GetRangesFrom(vtkKWParameterValueFunctionEditor *);
    int HasSameWholeRangesComparedTo(Ranges*);
    int NeedResizeComparedTo(Ranges*);
    int NeedPanOnlyComparedTo(Ranges*);
  };
  Ranges        LastRanges;
  unsigned long LastRedrawFunctionTime;
  //ETX

  double DisplayedWholeParameterRange[2];

  //BTX
  enum
  {
    ConstrainedMoveFree,
    ConstrainedMoveHorizontal,
    ConstrainedMoveVertical
  };
  //ETX
  int           LastSelectCanvasCoordinates[2];
  int           LastConstrainedMove;

  // Description:
  // Update the range label according to the current visible parameter and
  // value ranges
  virtual void UpdateRangeLabel();

  // Description:
  // Update the parameter entry according to the parameter of a point
  virtual void UpdateParameterEntry(int id);

  // Description:
  // Convenience method to look for a tag in the Canvas. 
  // Return the number of elements matching tag+suffix.
  virtual int CanvasHasTag(const char *tag, int *suffix = 0);

  // Description:
  // Convenience method to remove everything with a given tag.
  virtual void CanvasRemoveTag(const char *tag, const char *canv_name = NULL);
  virtual void CanvasRemoveTag(
    const char *prefix, int id, const char *canv_name = NULL);

  // Description:
  // Convenience method to check if a given tag if of a given type
  virtual int CanvasCheckTagType(const char *prefix, int id, const char *type);

  // Synchronization callbacks

  //BTX
  enum
  {
    FunctionChangedEvent = 10000,
    FunctionChangingEvent,
    PointAddedEvent,
    PointMovedEvent,
    PointMovingEvent,
    PointRemovedEvent,
    SelectionChangedEvent,
    VisibleParameterRangeChangedEvent,
    VisibleParameterRangeChangingEvent,
    VisibleRangeChangedEvent,
    VisibleRangeChangingEvent
  };
  //ETX

  vtkCallbackCommand *SynchronizeCallbackCommand;
  vtkCallbackCommand *SynchronizeCallbackCommand2;

  virtual int AddObserversList(int nb_events, int *events, vtkCommand *cmd);
  virtual int RemoveObserversList(int nb_events, int *events, vtkCommand *cmd);

  static void ProcessSynchronizationEvents(
    vtkObject *object, unsigned long event, void *clientdata, void *calldata);
  static void ProcessSynchronizationEvents2(
    vtkObject *object, unsigned long event, void *clientdata, void *calldata);

private:
  vtkKWParameterValueFunctionEditor(const vtkKWParameterValueFunctionEditor&); // Not implemented
  void operator=(const vtkKWParameterValueFunctionEditor&); // Not implemented
};

#endif

