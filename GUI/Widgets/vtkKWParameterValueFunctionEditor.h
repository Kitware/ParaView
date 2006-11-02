/*=========================================================================

  Module:    vtkKWParameterValueFunctionEditor.h,v

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
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWWidgetWithLabel

#ifndef __vtkKWParameterValueFunctionEditor_h
#define __vtkKWParameterValueFunctionEditor_h

#include "vtkKWParameterValueFunctionInterface.h"

//BTX
#include "vtkKWHistogram.h" // I need this one
//ETX

class vtkCallbackCommand;
class vtkKWCanvas;
class vtkKWFrame;
class vtkKWIcon;
class vtkKWLabel;
class vtkKWEntryWithLabel;
class vtkKWRange;
class vtkKWMenuButton;

//BTX
class ostrstream;
//ETX

class KWWidgets_EXPORT vtkKWParameterValueFunctionEditor : public vtkKWParameterValueFunctionInterface
{
public:
  vtkTypeRevisionMacro(vtkKWParameterValueFunctionEditor,vtkKWParameterValueFunctionInterface);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the whole parameter range.
  // Note that the visible parameter range is changed automatically to maintain
  // the same relative visible range within the whole range.
  virtual double* GetWholeParameterRange();
  virtual void SetWholeParameterRange(double r0, double r1);
  virtual void GetWholeParameterRange(double &r0, double &r1);
  virtual void GetWholeParameterRange(double range[2]);
  virtual void SetWholeParameterRange(double range[2]);

  // Description:
  // Set the whole parameter range to the function parameter range. 
  // Note that for safety reasons it will maintain the same relative visible
  // parameter range.
  virtual void SetWholeParameterRangeToFunctionRange();

  // Description:
  // Set/Get the visible parameter range in the editor.
  // This is the portion of the whole parameter range that is currently
  // visible (zoomed).
  virtual double* GetVisibleParameterRange();
  virtual void SetVisibleParameterRange(double r0, double r1);
  virtual void GetVisibleParameterRange(double &r0, double &r1);
  virtual void GetVisibleParameterRange(double range[2]);
  virtual void SetVisibleParameterRange(double range[2]);

  // Description:
  // Set the visible parameter range to the whole parameter range
  virtual void SetVisibleParameterRangeToWholeParameterRange();

  // Description:
  // Set/Get the visible parameter range in the editor as relative positions
  // in the whole parameter range.
  virtual void SetRelativeVisibleParameterRange(double r0, double r1);
  virtual void GetRelativeVisibleParameterRange(double &r0, double &r1);
  virtual void GetRelativeVisibleParameterRange(double range[2]);
  virtual void SetRelativeVisibleParameterRange(double range[2]);

  // Description:
  // Set/Get the whole value range.
  // Note that the visible value range is changed automatically to maintain
  // the same relative visible range within the whole range.
  virtual double* GetWholeValueRange();
  virtual void SetWholeValueRange(double r0, double r1);
  virtual void GetWholeValueRange(double &r0, double &r1);
  virtual void GetWholeValueRange(double range[2]);
  virtual void SetWholeValueRange(double range[2]);

  // Description:
  // Set/Get the visible value range.
  // This is the portion of the whole value range that is currently
  // visible (zoomed).
  virtual double* GetVisibleValueRange();
  virtual void SetVisibleValueRange(double r0, double r1);
  virtual void GetVisibleValueRange(double &r0, double &r1);
  virtual void GetVisibleValueRange(double range[2]);
  virtual void SetVisibleValueRange(double range[2]);

  // Description:
  // Set/Get the visible value range in the editor as relative positions
  // in the whole value range.
  virtual void SetRelativeVisibleValueRange(double r0, double r1);
  virtual void GetRelativeVisibleValueRange(double &r0, double &r1);
  virtual void GetRelativeVisibleValueRange(double range[2]);
  virtual void SetRelativeVisibleValueRange(double range[2]);

  // Description:
  // If supported, set the label position in regards to the rest of
  // the composite widget (override the super).
  // As a subclass of vtkKWWidgetWithLabel, this class inherits a label and
  // methods to set its position and visibility. Note that the default label 
  // position implemented in this class is on the same line as all other UI
  // elements like entries, or range parameters. Only a subset of the specific
  // positions listed in vtkKWWidgetWithLabel is supported: on Top
  // (the label is placed on its own line), or the Left of the whole editor, 
  // on the same line as the canvas. 
  virtual void SetLabelPosition(int);

  // Description:
  // Set/Get the parameter range UI visibility (the slider).
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ParameterRangeVisibility, int);
  virtual void SetParameterRangeVisibility(int);
  vtkGetMacro(ParameterRangeVisibility, int);

  // Description:
  // Set the position of the parameter range UI.
  //BTX
  enum 
  {
    ParameterRangePositionTop = 0,
    ParameterRangePositionBottom
  };
  //ETX
  virtual void SetParameterRangePosition(int);
  vtkGetMacro(ParameterRangePosition, int);
  virtual void SetParameterRangePositionToTop();
  virtual void SetParameterRangePositionToBottom();

  // Description:
  // Set/Get the value range UI visibility (the slider).
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ValueRangeVisibility, int);
  virtual void SetValueRangeVisibility(int);
  vtkGetMacro(ValueRangeVisibility, int);

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
  // the DisplayedWholeParameterRange.
  // The GetDisplayedVisibleParameterRange is a convenience function that
  // will return the visible parameter range mapped inside that displayed
  // parameter range.
  // The MapParameterToDisplayedParameter is a convenience function that
  // will map a parameter to the displayed parameter range.
  // The MapDisplayedParameterToParameter is a convenience function that
  // will map a displayed parameter back to the parameter range.
  // The GetFunctionPointDisplayedParameter is a convenience function that
  // will map the parameter of a point 'id' to the displayed parameter range.
  // If both ends of that range are the same, it is not used and all the
  // functions return the same parameter.
  vtkGetVector2Macro(DisplayedWholeParameterRange, double);
  virtual void SetDisplayedWholeParameterRange(double r0, double r1);
  virtual void SetDisplayedWholeParameterRange(double range[2]);
  virtual void GetDisplayedVisibleParameterRange(double &r0, double &r1);
  virtual void GetDisplayedVisibleParameterRange(double range[2]);
  virtual void MapParameterToDisplayedParameter(double p, double *displayed_p);
  virtual void MapDisplayedParameterToParameter(double displayed_p, double *p);
  virtual int GetFunctionPointDisplayedParameter(int id, double *displayed_p);
  
  // Description:
  // Set the position of points in the value range. 
  // Default is PointPositionValue, i.e. if the point value is
  // mono-dimensional, its vertical position in the canvas will be computed
  // from its value relative to the whole value range. If PositionCenter 
  // or if the point value is multi-dimensional, the point is centered
  // vertically.
  //BTX
  enum 
  {
    PointPositionValue = 0,
    PointPositionTop,
    PointPositionBottom,
    PointPositionCenter
  };
  //ETX
  virtual void SetPointPositionInValueRange(int);
  vtkGetMacro(PointPositionInValueRange, int);
  virtual void SetPointPositionInValueRangeToValue();
  virtual void SetPointPositionInValueRangeToTop();
  virtual void SetPointPositionInValueRangeToBottom();
  virtual void SetPointPositionInValueRangeToCenter();

  // Description:
  // Set/Get the parameter range label UI visibility.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ParameterRangeLabelVisibility, int);
  virtual void SetParameterRangeLabelVisibility(int);
  vtkGetMacro(ParameterRangeLabelVisibility, int);

  // Description:
  // Set/Get the value range label UI visibility.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ValueRangeLabelVisibility, int);
  virtual void SetValueRangeLabelVisibility(int);
  vtkGetMacro(ValueRangeLabelVisibility, int);

  // Description:
  // Display the range label at the default position (on the same line as all
  // other elements), or on top on its own line.
  // The ParameterRangeLabelVisibility or ValueRangeLabelVisibility 
  // parameter still has to be On for the label to be displayed.
  //BTX
  enum
  {
    RangeLabelPositionDefault = 10,
    RangeLabelPositionTop
  };
  //ETX
  virtual void SetRangeLabelPosition(int);
  vtkGetMacro(RangeLabelPosition, int);
  virtual void SetRangeLabelPositionToDefault();
  virtual void SetRangeLabelPositionToTop();

  // Description:
  // Display the points entries (i.e. the parameter entry, 
  // and any other entries the subclass will introduce) at
  // the default position (on the same line as all other elements), or on
  // the right of the canvas.
  //BTX
  enum
  {
    PointEntriesPositionDefault = 10,
    PointEntriesPositionRight
  };
  //ETX
  virtual void SetPointEntriesPosition(int);
  vtkGetMacro(PointEntriesPosition, int);
  virtual void SetPointEntriesPositionToDefault();
  virtual void SetPointEntriesPositionToRight();

  // Description:
  // Set/Get the point entries UI visibility.
  // This will hide all text entries for this class, i.e. the parameter
  // entry and all values entries (say, RGB, or opacitry, or sharpness, etc).
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(PointEntriesVisibility, int);
  virtual void SetPointEntriesVisibility(int);
  vtkGetMacro(PointEntriesVisibility, int);

  // Description:
  // Set/Get the parameter entry UI visibility.
  // Not shown if PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ParameterEntryVisibility, int);
  virtual void SetParameterEntryVisibility(int);
  vtkGetMacro(ParameterEntryVisibility, int);

  // Description:
  // Set/Get the parameter entry printf format. If not NULL, it is
  // applied to the displayed parameter value before assigning it to
  // the parameter entry.
  virtual void SetParameterEntryFormat(const char *);
  vtkGetStringMacro(ParameterEntryFormat);

  // Description:
  // Access the parameter entry.
  virtual vtkKWEntryWithLabel* GetParameterEntry();

  // Description:
  // Set/Get the user frame UI visibility.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(UserFrameVisibility, int);
  virtual void SetUserFrameVisibility(int);
  vtkGetMacro(UserFrameVisibility, int);

  // Description:
  // Access the user frame
  // If you need to add elements to the user-frame, make sure you first set 
  // UserFrameVisibility to On and call Create().
  vtkGetObjectMacro(UserFrame, vtkKWFrame);

  // Description:
  // Set/Get the requested canvas width/height in pixels (i.e. the drawable 
  // region). If ExpandCanvasWidth is On, the canvas will expand automatically
  // to accomodate its parent: in that case, use GetCurrentCanvasWidth and
  // GetCurrentCanvasHeight to retrieve the current width/height. 
  // This mechanism does not behave as expected sometimes, in that case set 
  // ExpandCanvasWidth to Off and CanvasWidth to the proper value
  virtual void SetCanvasHeight(int);
  virtual void SetCanvasWidth(int);
  virtual int GetCanvasHeight();
  virtual int GetCanvasWidth();
  vtkBooleanMacro(ExpandCanvasWidth, int);
  virtual void SetExpandCanvasWidth(int);
  vtkGetMacro(ExpandCanvasWidth, int);
  vtkGetMacro(CurrentCanvasHeight, int);
  vtkGetMacro(CurrentCanvasWidth, int);
  
  // Description:
  // Set/Get the canvas visibility, i.e. the whole area where the function
  // line, points, canvas outline, background and histogram are displayed
  vtkBooleanMacro(CanvasVisibility, int);
  virtual void SetCanvasVisibility(int);
  vtkGetMacro(CanvasVisibility, int);

  // Description:
  // Set/Get the function line visibility 
  // (i.e, if set to Off, only the points are displayed).
  vtkBooleanMacro(FunctionLineVisibility, int);
  virtual void SetFunctionLineVisibility(int);
  vtkGetMacro(FunctionLineVisibility, int);

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
  virtual void SetFunctionLineStyleToSolid();
  virtual void SetFunctionLineStyleToDash();

  // Description:
  // Set/Get the canvas outline visibility
  vtkBooleanMacro(CanvasOutlineVisibility, int);
  virtual void SetCanvasOutlineVisibility(int);
  vtkGetMacro(CanvasOutlineVisibility, int);

  // Description:
  // Set the canvas outline style.
  //BTX
  enum 
  {
    CanvasOutlineStyleLeftSide        = 1,
    CanvasOutlineStyleRightSide       = 2,
    CanvasOutlineStyleHorizontalSides = 3,
    CanvasOutlineStyleTopSide         = 4,
    CanvasOutlineStyleBottomSide      = 8,
    CanvasOutlineStyleVerticalSides   = 12,
    CanvasOutlineStyleAllSides        = 15
  };
  //ETX
  vtkBooleanMacro(CanvasOutlineStyle, int);
  virtual void SetCanvasOutlineStyle(int);
  vtkGetMacro(CanvasOutlineStyle, int);
  
  // Description:
  // Set/Get the canvas background visibility
  vtkBooleanMacro(CanvasBackgroundVisibility, int);
  virtual void SetCanvasBackgroundVisibility(int);
  vtkGetMacro(CanvasBackgroundVisibility, int);
  
  // Description:
  // Set/Get the parameter cursor visibility. This is a vertical line
  // spanning the whole value range, located at a specific position in
  // the parameter range. Set the position using ParameterCursorPosition.
  vtkBooleanMacro(ParameterCursorVisibility, int);
  virtual void SetParameterCursorVisibility(int);
  vtkGetMacro(ParameterCursorVisibility, int);

  // Description:
  // Set/Get the parameter cursor position (inside the parameter range)
  virtual void SetParameterCursorPosition(double);
  vtkGetMacro(ParameterCursorPosition, double);
  
  // Description:
  // Set/Get the cursor color. 
  vtkGetVector3Macro(ParameterCursorColor, double);
  virtual void SetParameterCursorColor(double r, double g, double b);
  virtual void SetParameterCursorColor(double rgb[3]);

  // Description:
  // Set the parameter cursor interaction style.
  //BTX
  enum 
  {
    ParameterCursorInteractionStyleNone                     = 0,
    ParameterCursorInteractionStyleDragWithLeftButton       = 1,
    ParameterCursorInteractionStyleSetWithRighButton        = 2,
    ParameterCursorInteractionStyleSetWithControlLeftButton = 4,
    ParameterCursorInteractionStyleAll                      = 7
  };
  //ETX
  vtkBooleanMacro(ParameterCursorInteractionStyle, int);
  virtual void SetParameterCursorInteractionStyle(int);
  vtkGetMacro(ParameterCursorInteractionStyle, int);

  // Description:
  // Set/Get the parameter ticks visibility
  vtkBooleanMacro(ParameterTicksVisibility, int);
  virtual void SetParameterTicksVisibility(int);
  vtkGetMacro(ParameterTicksVisibility, int);

  // Description:
  // Set/Get the number of parameters ticks.
  virtual void SetNumberOfParameterTicks(int);
  vtkGetMacro(NumberOfParameterTicks, int);

  // Description:
  // Set/Get the parameter ticks printf format. Set to NULL to actually
  // hide the label.
  virtual void SetParameterTicksFormat(const char *);
  vtkGetStringMacro(ParameterTicksFormat);

  // Description:
  // Set/Get the value ticks visibility
  vtkBooleanMacro(ValueTicksVisibility, int);
  virtual void SetValueTicksVisibility(int);
  vtkGetMacro(ValueTicksVisibility, int);

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
  // Set/Get if the points of the function are locked in the parameter
  // space (they can not be removed or can only be moved in the value space).
  virtual void SetLockPointsParameter(int);
  vtkBooleanMacro(LockPointsParameter, int);
  vtkGetMacro(LockPointsParameter, int);

  // Description:
  // Set/Get if the end-points of the function are locked in the parameter
  // space (they can not be removed or can only be moved in the value space).
  // Superseded by LockPointsParameter
  virtual void SetLockEndPointsParameter(int);
  vtkBooleanMacro(LockEndPointsParameter, int);
  vtkGetMacro(LockEndPointsParameter, int);

  // Description:
  // Set/Get if the points of the function are locked in the value
  // space (they can not be removed or can only be moved in the parameter 
  // space).
  virtual void SetLockPointsValue(int);
  vtkBooleanMacro(LockPointsValue, int);
  vtkGetMacro(LockPointsValue, int);

  // Description:
  // Set/Get if points can be added and removed.
  vtkSetMacro(DisableAddAndRemove, int);
  vtkBooleanMacro(DisableAddAndRemove, int);
  vtkGetMacro(DisableAddAndRemove, int);

  // Description:
  // Convenience method to set both LockPointsParameter, LockPointsValue
  // and DisableAddAndRemove to On or Off
  virtual void SetReadOnly(int);
  vtkBooleanMacro(ReadOnly, int);

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
  // Set/Get the point radius (in pixels) horizontally and vertically.
  virtual void SetPointRadius(int);
  virtual void SetPointRadiusX(int);
  vtkGetMacro(PointRadiusX, int);
  virtual void SetPointRadiusY(int);
  vtkGetMacro(PointRadiusY, int);

  // Description:
  // Set/Get the selected point radius as a fraction
  // of the point radius (see PointRadiusX and PointRadiusY). 
  virtual void SetSelectedPointRadius(double);
  vtkGetMacro(SelectedPointRadius, double);

  // Description:
  // Set/Get the label to display in the selected point instead of its
  // index (if PointIndexVisibility or SetPointIndexVisibility are set
  // to ON). Set to NULL to go back to defaults.
  virtual void SetSelectedPointText(const char *);
  vtkGetStringMacro(SelectedPointText);
  virtual void SetSelectedPointTextToInt(int);

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
  virtual void SetPointStyleToDisc();
  virtual void SetPointStyleToCursorDown();
  virtual void SetPointStyleToCursorUp();
  virtual void SetPointStyleToCursorLeft();
  virtual void SetPointStyleToCursorRight();
  virtual void SetPointStyleToRectangle();
  virtual void SetPointStyleToDefault();
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
    PointMarginAllSides        = 15
  };
  //ETX
  vtkBooleanMacro(PointMarginToCanvas, int);
  virtual void SetPointMarginToCanvas(int);
  vtkGetMacro(PointMarginToCanvas, int);
  virtual void SetPointMarginToCanvasToNone();
  virtual void SetPointMarginToCanvasToLeftSide();
  virtual void SetPointMarginToCanvasToRightSide();
  virtual void SetPointMarginToCanvasToHorizontalSides();
  virtual void SetPointMarginToCanvasToTopSide();
  virtual void SetPointMarginToCanvasToBottomSide();
  virtual void SetPointMarginToCanvasToVerticalSides();
  virtual void SetPointMarginToCanvasToAllSides();

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
  virtual int AddPointAtCanvasCoordinates(int x, int y, int *id);
  virtual int AddPointAtParameter(double parameter, int *id);

  // Description:
  // Merge all the points from another function editor.
  // Return the number of points merged.
  virtual int MergePointsFromEditor(vtkKWParameterValueFunctionEditor *editor);

  // Description:
  // Set/Get the background color of the main frame, where the function
  // is drawn. Note that the frame can be smaller than the widget itself
  // depending on the margin requested to draw the points entirely (see
  // PointMarginToCanvas ivar). Use SetBackgroundColor to set the
  // canvas color (i.e., the whole area outside the margin)
  vtkGetVector3Macro(FrameBackgroundColor, double);
  virtual void SetFrameBackgroundColor(double r, double g, double b);
  virtual void SetFrameBackgroundColor(double rgb[3]);
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3]);

  // Description:
  // Set/Get the point color. 
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(PointColor, double);
  virtual void SetPointColor(double r, double g, double b);
  virtual void SetPointColor(double rgb[3]);
  
  // Description:
  // Set/Get the selected point color, as well as its color when the user.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(SelectedPointColor, double);
  virtual void SetSelectedPointColor(double r, double g, double b);
  virtual void SetSelectedPointColor(double rgb[3]);

  // Description:
  // Set/Get the selected point color when the user is actually interacting
  // with it. Set any components to a negative value to use the default
  // SelectedPointColor.
  vtkGetVector3Macro(SelectedPointColorInInteraction, double);
  virtual void SetSelectedPointColorInInteraction(
    double r, double g, double b);
  virtual void SetSelectedPointColorInInteraction(double rgb[3]);

  // Description:
  // Set the way the points are colored, either filled, or outlined.
  //BTX
  enum 
  {
    PointColorStyleFill = 0,
    PointColorStyleOutline
  };
  //ETX
  virtual void SetPointColorStyle(int);
  vtkGetMacro(PointColorStyle, int);
  virtual void SetPointColorStyleToFill();
  virtual void SetPointColorStyleToOutline();

  // Description:
  // Set/Get the point text color.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(PointTextColor, double);
  virtual void SetPointTextColor(double r, double g, double b);
  virtual void SetPointTextColor(double rgb[3]);

  // Description:
  // Set/Get the selected point text color.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(SelectedPointTextColor, double);
  virtual void SetSelectedPointTextColor(double r, double g, double b);
  virtual void SetSelectedPointTextColor(double rgb[3]);

  // Description:
  // Set a hint: some colors should be function of the value
  // (might not be supported/implemented in subclasses).
  vtkBooleanMacro(ComputePointColorFromValue, int);
  virtual void SetComputePointColorFromValue(int);
  vtkGetMacro(ComputePointColorFromValue, int);
  
  // Description:
  // Set/Get the point visibility in the canvas.
  // This actually hides both the point and the index inside.
  // If set to on, the index can still be hidden using PointIndexVisibility
  // and SelectedPointIndexVisibility. Guidelines are not affected.
  vtkBooleanMacro(PointVisibility, int);
  virtual void SetPointVisibility(int);
  vtkGetMacro(PointVisibility, int);

  // Description:
  // Set/Get the point index visibility for each point in the canvas.
  vtkBooleanMacro(PointIndexVisibility, int);
  virtual void SetPointIndexVisibility(int);
  vtkGetMacro(PointIndexVisibility, int);

  // Description:
  // Set/Get the selected point index visibility in the canvas.
  vtkBooleanMacro(SelectedPointIndexVisibility, int);
  virtual void SetSelectedPointIndexVisibility(int);
  vtkGetMacro(SelectedPointIndexVisibility, int);

  // Description:
  // Set/Get the point guideline visibility in the canvas 
  // (for ex: a vertical line at each point).
  vtkBooleanMacro(PointGuidelineVisibility, int);
  virtual void SetPointGuidelineVisibility(int);
  vtkGetMacro(PointGuidelineVisibility, int);

  // Description:
  // Set/Get the line style for the guideline.
  // See FunctionLineStyle for enumeration of style values.
  virtual void SetPointGuidelineStyle(int);
  vtkGetMacro(PointGuidelineStyle, int);

  // Description:
  // Set/Get the histogram and secondary histogram over the parameter range.
  // The primary histogram is drawn in a bar/area style, the secondary
  // one is drawn as dots on top of the primary.
  vtkGetObjectMacro(Histogram, vtkKWHistogram);
  virtual void SetHistogram(vtkKWHistogram*);
  vtkGetObjectMacro(SecondaryHistogram, vtkKWHistogram);
  virtual void SetSecondaryHistogram(vtkKWHistogram*);

  // Description:
  // Convenience method that will hide all elements but the histogram.
  // Various elements (the main label, the range and value sliders, the point
  // margins, etc.) will not be visible anymore, but can be restored at
  // any point. Note that ExpandCanvasWidth will be set to Off for 
  // convenience, and that the whole range will be set to the maximum range
  // of either the primary or secondary histogram, for convenience again (you
  // should therefore call SetHistogram and SetSecondaryHistogram before 
  // calling this method).
  virtual void DisplayHistogramOnly();

  // Description:
  // Set/Get the histogram and secondary histogram color. 
  // Overriden by ComputeHistogramColorFromValue if supported.
  vtkGetVector3Macro(HistogramColor, double);
  virtual void SetHistogramColor(double r, double g, double b);
  virtual void SetHistogramColor(double rgb[3]);
  vtkGetVector3Macro(SecondaryHistogramColor, double);
  virtual void SetSecondaryHistogramColor(double r, double g, double b);
  virtual void SetSecondaryHistogramColor(double rgb[3]);
  
  // Description:
  // Set a hint: histogram and secondary histogram colors should be function
  // of the value (might not be supported/implemented in subclasses).
  vtkBooleanMacro(ComputeHistogramColorFromValue, int);
  virtual void SetComputeHistogramColorFromValue(int);
  vtkGetMacro(ComputeHistogramColorFromValue, int);

  // Description:
  // Set/Get the histogram and secondary histogram style
  //BTX
  enum 
  {
    HistogramStyleBars = 0,
    HistogramStyleDots,
    HistogramStylePolyLine
  };
  //ETX
  vtkGetMacro(HistogramStyle, int);
  virtual void SetHistogramStyle(int);
  virtual void SetHistogramStyleToBars();
  virtual void SetHistogramStyleToDots();
  virtual void SetHistogramStyleToPolyLine();
  vtkGetMacro(SecondaryHistogramStyle, int);
  virtual void SetSecondaryHistogramStyle(int);
  virtual void SetSecondaryHistogramStyleToBars();
  virtual void SetSecondaryHistogramStyleToDots();
  virtual void SetSecondaryHistogramStyleToPolyLine();

  // Description:
  // Set/Get the line width of the histograms when drawn in polyline mode.
  virtual void SetHistogramPolyLineWidth(int);
  vtkGetMacro(HistogramPolyLineWidth, int);
  
  // Description:
  // Set/Get the histogram log mode button visibility.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  virtual void SetHistogramLogModeOptionMenuVisibility(int);
  vtkBooleanMacro(HistogramLogModeOptionMenuVisibility, int);
  vtkGetMacro(HistogramLogModeOptionMenuVisibility, int);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the histogram log mode is changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - new histogram log mode: int
  virtual void SetHistogramLogModeChangedCommand(
    vtkObject *object,const char *method);

  // Description:
  // Set/Get if the mouse cursor is changed automatically to provide
  // more feedback regarding the interaction (defaults to On).
  vtkBooleanMacro(ChangeMouseCursor, int);
  vtkGetMacro(ChangeMouseCursor, int);
  vtkSetMacro(ChangeMouseCursor, int);

  // Description:
  // Specifies function-related commands to associate with the widget.
  // 'FunctionStartChanging' is called when the function is starting to 
  // changing (as the result of a user starting an interaction, like selecting
  // a point to move it). 
  // 'FunctionChanging' is called when the function is changing (as the result
  // of a user interaction in progress, like moving a point). 
  // 'FunctionChanged' is called when the function has changed (as the result
  // of a user interaction that is now over, like a point added/(re)moved). 
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
  virtual void SetFunctionStartChangingCommand(
    vtkObject *object, const char *method);
  virtual void SetFunctionChangedCommand(
    vtkObject *object, const char *method);
  virtual void SetFunctionChangingCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies point-related commands to associate with the widget.
  // 'PointAddedCommand' is called after a point was added.
  // 'PointChangingCommand' is called when a point is changing (as the
  // result of a user interaction that is in progress, like moving a point).
  // 'PointChangedCommand' is called when a point is has changed (as the
  // result of a user interaction that is now over, like a point moved).
  // 'DoubleClickOnPointCommand' is called when double-clicking on a point.
  // 'PointRemovedCommand' is called after a point was removed.
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
  // Those commands provide more granularity than function-wide commands like
  // 'FunctionChangedCommand' and 'FunctionChangingCommand'. In most situations
  // though (or in doubt), you should probably use the function-wide commands.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the commands:
  // - index of the point that is being/was modified: int
  // 'PointRemovedCommand' is passed additional parameters:
  // - value of the parameter of the point that was removed (i.e. its position
  // in the parameter domain): double
  virtual void SetPointAddedCommand(
    vtkObject *object,const char *method);
  virtual void SetPointChangingCommand(
    vtkObject *object, const char *method);
  virtual void SetPointChangedCommand(
    vtkObject *object, const char *method);
  virtual void SetDoubleClickOnPointCommand(
    vtkObject *object,const char *method);
  virtual void SetPointRemovedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies selection-related commands to associate with the widget.
  // 'SelectionChanged' is called whenever the selection was changed or
  // cleared.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetSelectionChangedCommand(
    vtkObject *object,const char *method);

  // Description:
  // Specifies range-related commands to associate with the widget.
  // 'VisibleRangeChangingCommand' is called whenever the visible range 
  // (in parameter or value domain) is changing (i.e. during user interaction).
  // 'VisibleRangeChangedCommand' is called whenever the visible range 
  // (in parameter or value domain) has changed (i.e. at the end of the
  // user interaction).
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
  virtual void SetVisibleRangeChangedCommand(
    vtkObject *object, const char *method);
  virtual void SetVisibleRangeChangingCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies cursor-related commands to associate with the widget.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // 'ParameterCursorMovingCommand' is called whenever the parameter cursor
  // is moving (i.e. during user interaction).
  // 'ParameterCursorMovedCommand' is called whenever the parameter cursor
  // was moved (i.e., at the end of the user interaction).
  // The following parameters are also passed to the commands:
  // - current parameter cursor position: double
  virtual void SetParameterCursorMovingCommand(
    vtkObject *object, const char *method);
  virtual void SetParameterCursorMovedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Set/Get whether the above commands should be called or not.
  // This allow you to disable the commands while you are setting the range
  // value for example. Events are still invoked.
  vtkSetMacro(DisableCommands, int);
  vtkGetMacro(DisableCommands, int);
  vtkBooleanMacro(DisableCommands, int);

  // Description:
  // Events. Even though it is highly recommended to use the commands
  // framework defined above to specify the callback methods you want to be 
  // invoked when specific event occur, you can also use the observer
  // framework and listen to the corresponding events/
  // Note that they are passed the same parameters as the commands, if any.
  // If more than one numerical parameter is passed, they are all stored
  // in the calldata as an array of double.
  //BTX
  enum
  {
    FunctionChangedEvent = 10000,
    FunctionStartChangingEvent,
    FunctionChangingEvent,
    PointAddedEvent,
    PointChangedEvent,
    PointChangingEvent,
    PointRemovedEvent,
    SelectionChangedEvent,
    VisibleParameterRangeChangedEvent,
    VisibleParameterRangeChangingEvent,
    VisibleRangeChangedEvent,
    VisibleRangeChangingEvent,
    ParameterCursorMovedEvent,
    ParameterCursorMovingEvent,
    DoubleClickOnPointEvent
  };
  //ETX

  // Description:
  // Synchronize the visible parameter range between two editors A and B.
  // Each time the visible range of A is changed, the same visible range
  // is assigned to the synchronized editor B, and vice-versa.
  // Note that a call A->(B) is the same as a call B->(A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizeVisibleParameterRange(
    vtkKWParameterValueFunctionEditor *b);
  virtual int DoNotSynchronizeVisibleParameterRange(
    vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize points between two editors A and B.
  // First make sure both editors have the same points in the
  // parameter space (by calling MergePointsFromEditor on each other).
  // Then each time a point in A is added, moved or removed through 
  // user interaction, the same point in B is altered and vice-versa.
  // Note that a call A->(B) is the same as a call B->(A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizePoints(vtkKWParameterValueFunctionEditor *b);
  virtual int DoNotSynchronizePoints(vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize single selection between two editors A and B.
  // Each time a point is selected in A, the selection is cleared in B, 
  // and vice-versa.
  // Note that a call A->(B) is the same as a call B->(A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizeSingleSelection(
    vtkKWParameterValueFunctionEditor *b);
  virtual int DoNotSynchronizeSingleSelection(
    vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize same selection between two editors A and B.
  // Each time a point is selected in A, the same point is selected in B, 
  // and vice-versa.
  // Note that a call A->(B) is the same as a call B->(A), 
  // i.e. this is a double-link, only one call is needed to set the sync.
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizeSameSelection(
    vtkKWParameterValueFunctionEditor *b);
  virtual int DoNotSynchronizeSameSelection(
    vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);

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
  // Some constants
  //BTX
  static const char *FunctionTag;
  static const char *SelectedTag;
  static const char *PointTag;
  static const char *PointGuidelineTag;
  static const char *PointTextTag;
  static const char *LineTag;
  static const char *HistogramTag;
  static const char *SecondaryHistogramTag;
  static const char *FrameForegroundTag;
  static const char *FrameBackgroundTag;
  static const char *ParameterCursorTag;
  static const char *ParameterTicksTag;
  static const char *ValueTicksTag;
  //ETX

  // Description:
  // Is point locked, protected, removable ?
  virtual int FunctionPointCanBeAdded();
  virtual int FunctionPointCanBeRemoved(int id);
  virtual int FunctionPointParameterIsLocked(int id);
  virtual int FunctionPointValueIsLocked(int id);
  virtual int FunctionPointCanBeMovedToParameter(int id, double parameter);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  MoveFunctionPoint(int id,double parameter,const double *values);

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ConfigureCallback();
  virtual void VisibleParameterRangeChangingCallback(double, double);
  virtual void VisibleParameterRangeChangedCallback(double, double);
  virtual void VisibleValueRangeChangingCallback(double, double);
  virtual void VisibleValueRangeChangedCallback(double, double);
  virtual void StartInteractionCallback(int x, int y);
  virtual void MovePointCallback(int x, int y, int shift);
  virtual void EndInteractionCallback(int x, int y);
  virtual void ParameterCursorStartInteractionCallback(int x);
  virtual void ParameterCursorEndInteractionCallback();
  virtual void ParameterCursorMoveCallback(int x);
  virtual void ParameterEntryCallback(const char*);
  virtual void HistogramLogModeCallback(int mode);
  virtual void DoubleClickOnPointCallback(int x, int y);

protected:
  vtkKWParameterValueFunctionEditor();
  ~vtkKWParameterValueFunctionEditor();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // Description:
  // Return 1 if the function line joining point 'id1' and point 'id2'
  // is visible given the current visible parameter and value range . 
  // This implementation assuming that if the line is actually made of
  // segments sampled between the two end-points, the segments are still
  // bound by the box which diagonal is the line between id1 and id2. If
  // this is not the case, you can still override that small function in
  // subclasses.
  virtual int FunctionLineIsInVisibleRangeBetweenPoints(int id1, int id2);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  GetFunctionPointColorInCanvas(int id, double rgb[3]);
  virtual int  GetFunctionPointTextColorInCanvas(int id, double rgb[3]);
  virtual int  GetFunctionPointCanvasCoordinates(int id, int *x, int *y);
  virtual int  GetFunctionPointCanvasCoordinatesAtParameter(
    double parameter, int *x, int *y);
  virtual int  AddFunctionPointAtCanvasCoordinates(int x, int y, int *id);
  virtual int  AddFunctionPointAtParameter(double parameter, int *id);
  virtual int  MoveFunctionPointToCanvasCoordinates(int id,int x,int y);
  virtual int  MoveFunctionPointToParameter(int id,double parameter,int i=0);
  virtual int  EqualFunctionPointValues(
    const double *values1, const double *values2);
  virtual int  FindFunctionPointAtCanvasCoordinates(
    int x, int y, int *id, int *c_x, int *c_y);

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

  // Description:
  // Merge the point 'editor_id' from another function editor 'editor' into
  // the instance. This only happens if no other point already exists at the 
  // same parameter location, thus resulting in the creation of a new point.
  // Return 1 if a point was added (and set its id in 'new_id'), 0 otherwise
  virtual int MergePointFromEditor(
    vtkKWParameterValueFunctionEditor *editor, int editor_id, int *new_id);

  // Description:
  // Copy the point 'id' parameter and values from another function editor
  // 'editor' into the point 'id' in the instance. Both points have to exist
  // in both editors.
  // Return 1 if copy succeeded, 0 otherwise
  virtual int CopyPointFromEditor(
    vtkKWParameterValueFunctionEditor *editor, int id);

  int   ParameterRangeVisibility;
  int   ValueRangeVisibility;
  int   PointPositionInValueRange;
  int   ParameterRangePosition;
  int   PointColorStyle;
  int   CurrentCanvasHeight;
  int   CurrentCanvasWidth;
  int   RequestedCanvasHeight;
  int   RequestedCanvasWidth;
  int   ExpandCanvasWidth;
  int   LockPointsParameter;
  int   LockEndPointsParameter;
  int   LockPointsValue;
  int   RescaleBetweenEndPoints;
  int   DisableAddAndRemove;
  int   DisableRedraw;
  int   PointRadiusX;
  int   PointRadiusY;
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
  int   CanvasOutlineVisibility;
  int   CanvasOutlineStyle;
  int   ParameterCursorInteractionStyle;
  int   CanvasBackgroundVisibility;
  int   ParameterCursorVisibility;
  int   FunctionLineVisibility;
  int   CanvasVisibility;
  int   PointVisibility;
  int   PointIndexVisibility;
  int   PointGuidelineVisibility;
  int   SelectedPointIndexVisibility;
  int   ParameterRangeLabelVisibility;
  int   ValueRangeLabelVisibility;
  int   RangeLabelPosition;
  int   PointEntriesPosition;
  int   ParameterEntryVisibility;
  int   PointEntriesVisibility;
  int   UserFrameVisibility;
  int   ParameterTicksVisibility;
  int   ValueTicksVisibility;
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
  char* SelectedPointText;

  double FrameBackgroundColor[3];
  double ParameterCursorColor[3];
  double PointColor[3];
  double SelectedPointColor[3];
  double SelectedPointColorInInteraction[3];
  double PointTextColor[3];
  double SelectedPointTextColor[3];
  int    ComputePointColorFromValue;
  int    InUserInteraction;

  // Commands

  char  *PointAddedCommand;
  char  *PointChangingCommand;
  char  *PointChangedCommand;
  char  *PointRemovedCommand;
  char  *SelectionChangedCommand;
  char  *FunctionChangedCommand;
  char  *FunctionChangingCommand;
  char  *FunctionStartChangingCommand;
  char  *VisibleRangeChangedCommand;
  char  *VisibleRangeChangingCommand;
  char  *ParameterCursorMovingCommand;
  char  *ParameterCursorMovedCommand;
  char  *DoubleClickOnPointCommand;

  virtual void InvokeObjectMethodCommand(const char *command);
  virtual void InvokeHistogramLogModeChangedCommand(int mode);
  virtual void InvokePointCommand(
    const char *command, int id, const char *extra = 0);

  virtual void InvokePointAddedCommand(int id);
  virtual void InvokePointChangingCommand(int id);
  virtual void InvokePointChangedCommand(int id);
  virtual void InvokePointRemovedCommand(int id, double parameter);
  virtual void InvokeDoubleClickOnPointCommand(int id);
  virtual void InvokeSelectionChangedCommand();
  virtual void InvokeFunctionChangedCommand();
  virtual void InvokeFunctionChangingCommand();
  virtual void InvokeFunctionStartChangingCommand();
  virtual void InvokeVisibleRangeChangedCommand();
  virtual void InvokeVisibleRangeChangingCommand();
  virtual void InvokeParameterCursorMovingCommand(double pos);
  virtual void InvokeParameterCursorMovedCommand(double pos);

  // GUI

  vtkKWCanvas         *Canvas;
  vtkKWRange          *ParameterRange;
  vtkKWRange          *ValueRange;
  vtkKWFrame          *TopLeftContainer;
  vtkKWFrame          *TopLeftFrame;
  vtkKWFrame          *UserFrame;
  vtkKWFrame          *PointEntriesFrame;
  vtkKWLabel          *RangeLabel;
  vtkKWEntryWithLabel *ParameterEntry;
  vtkKWCanvas         *ValueTicksCanvas;
  vtkKWCanvas         *ParameterTicksCanvas;
  vtkKWCanvas         *GuidelineValueCanvas;

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
  int           HistogramLogModeOptionMenuVisibility;
  char          *HistogramLogModeChangedCommand;
  int           HistogramPolyLineWidth;

  vtkKWMenuButton  *HistogramLogModeOptionMenu;
  virtual void CreateHistogramLogModeOptionMenu();
  virtual void UpdateHistogramLogModeOptionMenu();

  // Description:
  // Bind/Unbind all widgets.
  virtual void Bind();
  virtual void UnBind();

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateLabel();
  virtual void CreateParameterRange();
  virtual void CreateValueRange();
  virtual void CreateRangeLabel();
  virtual void CreatePointEntriesFrame();
  virtual void CreateParameterEntry();
  virtual void CreateTopLeftContainer();
  virtual void CreateTopLeftFrame();
  virtual void CreateUserFrame();
  virtual void CreateValueTicksCanvas();
  virtual void CreateParameterTicksCanvas();
  virtual void CreateGuidelineValueCanvas();
  virtual int IsTopLeftFrameUsed();
  virtual int IsPointEntriesFrameUsed();
  virtual int IsGuidelineValueCanvasUsed();

  // Description:
  // Pack the widget
  virtual void Pack();
  virtual void PackPointEntries();

  // Description:
  // Get the center of a given canvas item (using its item id)
  virtual void GetCanvasItemCenter(int item_id, int *x, int *y);

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
  // RedrawFunctionDependentElements: the function has changed (as triggered
  // if GetRedrawFunctionTime(), a monotonically increasing value, has changed.
  // in this implementation, it just calls GetFunctionMTime(), but can be
  // overriden in subclasses to take into account other objects modification
  // time)
  virtual unsigned long GetRedrawFunctionTime();
  virtual void Redraw();
  virtual void RedrawSizeDependentElements();
  virtual void RedrawPanOnlyDependentElements();
  virtual void RedrawFunctionDependentElements();
  virtual void RedrawSinglePointDependentElements(int id);

  // Description:
  // Redraw the whole function or a specific point, or 
  // the line between two points
  //BTX
  virtual void RedrawFunction();
  virtual void RedrawPoint(int id, ostrstream *tk_cmd = 0);
  virtual void RedrawLine(int id1, int id2, ostrstream *tk_cmd = 0);
  virtual void GetLineCoordinates(int id1, int id2, ostrstream *tk_cmd);
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
  int           LastRedrawFunctionSize;
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
  int           LastSelectionCanvasCoordinateX;
  int           LastSelectionCanvasCoordinateY;
  int           LastConstrainedMove;

  // Description:
  // Update the range label according to the current visible parameter and
  // value ranges
  virtual void UpdateRangeLabel();

  // Description:
  // Update the parameter entry according to the parameter of a point
  virtual void UpdateParameterEntry(int id);

  // Description:
  // Look for a tag in the Canvas. 
  // Return the number of elements matching tag+suffix, in default
  // canvas or specified one.
  virtual int CanvasHasTag(
    const char *tag, int *suffix = 0, vtkKWCanvas *canv = NULL);

  // Description:
  // Remove everything with a given tag.
  virtual void CanvasRemoveTag(const char *tag, const char *canv_name = NULL);
  virtual void CanvasRemoveTag(
    const char *prefix, int id, const char *canv_name = NULL);

  // Description:
  // Check if a given tag if of a given type
  virtual int CanvasCheckTagType(const char *prefix, int id, const char *type);

  // Description:
  // Find item with given tag closest to canvas coordinates x, y and a given
  // halo distance. 
  // Return 1 if found, 0 otherwise. 
  int FindClosestItemWithTagAtCanvasCoordinates(
    int x, int y, int halo, const char *tag, int *c_x, int *c_y, char *found);

  // Synchronization callbacks

  vtkCallbackCommand *SynchronizeCallbackCommand;
  vtkCallbackCommand *SynchronizeCallbackCommand2;

  virtual int AddObserversList(int nb_events, int *events, vtkCommand *cmd);
  virtual int RemoveObserversList(int nb_events, int *events, vtkCommand *cmd);

  virtual void ProcessSynchronizationEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  static void ProcessSynchronizationEventsFunction(
    vtkObject *object, unsigned long event, void *clientdata, void *calldata);

  virtual void ProcessSynchronizationEvents2(
    vtkObject *caller, unsigned long event, void *calldata);
  static void ProcessSynchronizationEventsFunction2(
    vtkObject *object, unsigned long event, void *clientdata, void *calldata);

private:

  vtkKWParameterValueFunctionEditor(const vtkKWParameterValueFunctionEditor&); // Not implemented
  void operator=(const vtkKWParameterValueFunctionEditor&); // Not implemented
};

#endif

