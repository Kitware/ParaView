/*=========================================================================

  Module:    vtkKWParameterValueHermiteFunctionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWParameterValueHermiteFunctionEditor - a hermite function editor
// .SECTION Description
// This widget implements and defines the interface needed to edit
// an hermitian curve. On top of the superclass API that already
// describes each control point of the curve, several pure virtual functions
// are provided for subclasses to specify the midpoint and sharpness at each
// control point.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWParameterValueHermiteFunctionEditor_h
#define __vtkKWParameterValueHermiteFunctionEditor_h

#include "vtkKWParameterValueFunctionEditor.h"

class vtkKWScaleWithEntry;

class KWWidgets_EXPORT vtkKWParameterValueHermiteFunctionEditor : public vtkKWParameterValueFunctionEditor
{
public:
  vtkTypeRevisionMacro(vtkKWParameterValueHermiteFunctionEditor,vtkKWParameterValueFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the midpoint entry UI visibility.
  // Not shown if superclass PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(MidPointEntryVisibility, int);
  virtual void SetMidPointEntryVisibility(int);
  vtkGetMacro(MidPointEntryVisibility, int);

  // Description:
  // Access the parameter entry.
  virtual vtkKWScaleWithEntry* GetMidPointEntry();

  // Description:
  // Set/Get if the midpoint value should be displayed in the parameter
  // domain instead of the normalized [0.0, 1.0] domain.
  vtkBooleanMacro(DisplayMidPointValueInParameterDomain, int);
  virtual void SetDisplayMidPointValueInParameterDomain(int);
  vtkGetMacro(DisplayMidPointValueInParameterDomain, int);

  // Description:
  // Set/Get the sharpness entry UI visibility.
  // Not shown if superclass PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(SharpnessEntryVisibility, int);
  virtual void SetSharpnessEntryVisibility(int);
  vtkGetMacro(SharpnessEntryVisibility, int);

  // Description:
  // Access the parameter entry.
  virtual vtkKWScaleWithEntry* GetSharpnessEntry();

  // Description:
  // Set/Get the midpoint visibility in the canvas.
  // The style of the midpoint is a rectangle around the midpoint location.
  // Its color is controlled using MidPointColor.
  vtkBooleanMacro(MidPointVisibility, int);
  virtual void SetMidPointVisibility(int);
  vtkGetMacro(MidPointVisibility, int);

  // Description:
  // Set/Get the midpoints color. 
  vtkGetVector3Macro(MidPointColor, double);
  virtual void SetMidPointColor(double r, double g, double b);
  virtual void SetMidPointColor(double rgb[3])
    { this->SetMidPointColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the selected midpoint color.
  vtkGetVector3Macro(SelectedMidPointColor, double);
  virtual void SetSelectedMidPointColor(double r, double g, double b);
  virtual void SetSelectedMidPointColor(double rgb[3])
    { this->SetSelectedMidPointColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set/Get the midpoint guideline visibility in the canvas 
  // (for ex: a vertical line at each midpoint).
  // The style of the midpoint guidelines is the same as the style of the
  // superclass point guideline (PointGuidelineStyle) for consistency.
  vtkBooleanMacro(MidPointGuidelineVisibility, int);
  virtual void SetMidPointGuidelineVisibility(int);
  vtkGetMacro(MidPointGuidelineVisibility, int);

  // Description:
  // Set/Get the midpoint guideline value visibility in the canvas 
  // (i.e., a value on top of the guideline).
  // Note that the value is not displayed if MidPointGuidelineVisibility is
  // set to Off (i.e. if we do not display the guideline itself, why displaying
  // the value ?).
  vtkBooleanMacro(MidPointGuidelineValueVisibility, int);
  virtual void SetMidPointGuidelineValueVisibility(int);
  vtkGetMacro(MidPointGuidelineValueVisibility, int);

  // Description:
  // Set/Get the midpoint guideline value printf format.
  virtual void SetMidPointGuidelineValueFormat(const char *);
  vtkGetStringMacro(MidPointGuidelineValueFormat);

  // Description:
  // Select/Deselect the midpoint between two adjacent points 'id' and
  // 'id + 1'. Retrieve the midpoint selection, clear it, etc.
  // (-1 if none selected)
  vtkGetMacro(SelectedMidPoint, int);
  virtual void SelectMidPoint(int id);
  virtual void ClearMidPointSelection();
  virtual int  HasMidPointSelection();

  // Description:
  // Select a point.
  // Override the superclass so that selecting a point will clear
  // the midpoint selection.
  virtual void SelectPoint(int id);

  // Description:
  // Select next and previous point.
  // Override the superclass so that the mid-points are also selected (i.e.
  // it will iterator over point, mid-point, point, mid-point, etc.)
  virtual void SelectNextPoint();
  virtual void SelectPreviousPoint();

  // Description:
  // Specifies selection-related commands to associate with the widget.
  // 'MidPointSelectionChangedCommand' is called whenever the midpoint
  // selection was changed or cleared.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetMidPointSelectionChangedCommand(
    vtkObject *object,const char *method);

  // Description:
  // Events. Even though it is highly recommended to use the commands
  // framework defined above to specify the callback methods you want to be 
  // invoked when specific event occur, you can also use the observer
  // framework and listen to the corresponding events:
  //BTX
  enum
  {
    MidPointSelectionChangedEvent = 11000
  };
  //ETX

  // Description:
  // Synchronize single selection between two editors A and B.
  // Override the superclass to take the midpoint selection into account
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizeSingleSelection(
    vtkKWParameterValueFunctionEditor *b);
  virtual int DoNotSynchronizeSingleSelection(
    vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Synchronize same selection between two editors A and B.
  // Override the superclass to take the midpoint selection into account
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizeSameSelection(
    vtkKWParameterValueFunctionEditor *b);
  virtual int DoNotSynchronizeSameSelection(
    vtkKWParameterValueFunctionEditor *b);

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Convenience method that will hide all elements but the histogram.
  virtual void DisplayHistogramOnly();

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
  static const char *MidPointTag;
  static const char *MidPointGuidelineTag;
  static const char *MidPointSelectedTag;
  //ETX

  // Description:
  // Callbacks. Internal, do not use.
  virtual void MidPointEntryChangedCallback(double value);
  virtual void MidPointEntryChangingCallback(double value);
  virtual void SharpnessEntryChangedCallback(double value);
  virtual void SharpnessEntryChangingCallback(double value);
  virtual void StartInteractionCallback(int x, int y);
  virtual void MoveMidPointCallback(int x, int y, int button);
  virtual void EndMidPointInteractionCallback(int x, int y);

protected:
  vtkKWParameterValueHermiteFunctionEditor();
  ~vtkKWParameterValueHermiteFunctionEditor();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  // Description:
  // Retrieve the midpoint between two adjacent points 'id' and 'id + 1'.
  // The midpoint is the normalized distance between the two points at which
  // the interpolated value reaches the median value in the value space.
  // Return 1 on success (there is a midpoint at normalized position 'pos'),
  // 0 otherwise.
  // The default implementation here does not provide any midpoint.
  virtual int GetFunctionPointMidPoint(int id, double *pos) = 0;

  // Description:
  // Set the midpoint between two adjacent points 'id' and 'id + 1'.
  // Return 1 on success (the midpoint was successfully set at normalized
  // position 'pos'), 0 otherwise.
  // The default implementation here does not provide any midpoint.
  virtual int SetFunctionPointMidPoint(int id, double pos) = 0;

  // Description:
  // Return 1 if the 'midpoint' of the point 'id' is locked (can/should 
  // not be changed/edited), 0 otherwise.
  virtual int FunctionPointMidPointIsLocked(int id);

  // Description:
  // Retrieve the sharpness of the transition between two adjacent points
  // 'id' and 'id + 1'.
  // Return 1 on success (there is a sharpness defined for this point),
  // 0 otherwise.
  virtual int GetFunctionPointSharpness(int id, double *sharpness) = 0;

  // Description:
  // Set the sharpness of the transition between two adjacent points
  // 'id' and 'id + 1'.
  // Return 1 on success (the sharpness was successfully set), 0 otherwise.
  virtual int SetFunctionPointSharpness(int id, double sharpness) = 0;

  // Description:
  // Return 1 if the 'sharpness' of the point 'id' is locked (can/should 
  // not be changed/edited), 0 otherwise.
  virtual int FunctionPointSharpnessIsLocked(int id);

  // Description:
  // Update mi9dpoint entries
  virtual void UpdateMidPointEntries(int id);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  GetMidPointCanvasCoordinates(int id, int *x, int *y, double *p);
  virtual int  FindMidPointAtCanvasCoordinates(
    int x, int y, int *id, int *c_x, int *c_y);

  int    MidPointEntryVisibility;
  int    DisplayMidPointValueInParameterDomain;
  int    SharpnessEntryVisibility;
  int    MidPointGuidelineVisibility;
  int    MidPointGuidelineValueVisibility;
  double MidPointColor[3];
  double SelectedMidPointColor[3];
  int    SelectedMidPoint;
  int    LastMidPointSelectionCanvasCoordinateX;
  int    LastMidPointSelectionCanvasCoordinateY;
  double LastMidPointSelectionSharpness;

  char* MidPointGuidelineValueFormat;

  // Commands

  char  *MidPointSelectionChangedCommand;

  virtual void InvokeMidPointSelectionChangedCommand();

  // GUI

  vtkKWScaleWithEntry *MidPointEntry;
  vtkKWScaleWithEntry *SharpnessEntry;

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateMidPointEntry();
  virtual void CreateSharpnessEntry();
  virtual int IsPointEntriesFrameUsed();
  virtual int IsGuidelineValueCanvasUsed();

  // Description:
  // Update the midpoint entry according to the midpoint of a point
  virtual void UpdateMidPointEntry(int id);

  // Description:
  // Update the sharpness entry according to the sharpness of a point
  virtual void UpdateSharpnessEntry(int id);

  // Description:
  // Merge the point 'editor_id' from another function editor 'editor' into
  // our instance. Override the super to pass the midpoint and sharpness too
  virtual int MergePointFromEditor(
    vtkKWParameterValueFunctionEditor *editor, int editor_id, int *new_id);

  // Description:
  // Copy the point 'id' parameter and values from another function editor
  // 'editor' into the point 'id' in the instance.
  // Override the super to pass the midpoint and sharpness too
  virtual int CopyPointFromEditor(
    vtkKWParameterValueFunctionEditor *editor, int id);

  // Description:
  // Redraw the whole function or a specific point, or 
  // the line between two points. Overriden to take midpoints into account
  //BTX
  virtual void RedrawFunction();
  virtual void RedrawFunctionDependentElements();
  virtual void RedrawSinglePointDependentElements(int id);
  virtual void RedrawLine(int id1, int id2, ostrstream *tk_cmd = 0);
  //ETX

  // Description:
  // Pack the widget
  virtual void PackPointEntries();

  // Description:
  // Bind/Unbind all widgets.
  virtual void Bind();
  virtual void UnBind();

  // Synchronization callbacks

  virtual void ProcessSynchronizationEvents(
    vtkObject *caller, unsigned long event, void *calldata);
  virtual void ProcessSynchronizationEvents2(
    vtkObject *caller, unsigned long event, void *calldata);

private:

  int    MidPointVisibility;

  vtkKWParameterValueHermiteFunctionEditor(const vtkKWParameterValueHermiteFunctionEditor&); // Not implemented
  void operator=(const vtkKWParameterValueHermiteFunctionEditor&); // Not implemented
};

#endif

