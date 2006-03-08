/*=========================================================================

  Module:    vtkKWPiecewiseFunctionEditor.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWPiecewiseFunctionEditor - a piecewise function editor
// .SECTION Description
// A widget that allows the user to edit a piecewise function.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWPiecewiseFunctionEditor_h
#define __vtkKWPiecewiseFunctionEditor_h

#include "vtkKWParameterValueHermiteFunctionEditor.h"

class vtkKWCheckButton;
class vtkPiecewiseFunction;
class vtkKWScaleWithEntry;
class vtkColorTransferFunction;

class KWWidgets_EXPORT vtkKWPiecewiseFunctionEditor : public vtkKWParameterValueHermiteFunctionEditor
{
public:
  static vtkKWPiecewiseFunctionEditor* New();
  vtkTypeRevisionMacro(vtkKWPiecewiseFunctionEditor,vtkKWParameterValueHermiteFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the function
  // Note that the whole parameter range is automatically reset to the
  // function range.
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction*);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Get/Set an optional color transfer function to be used to retrieve
  // the color of each point. It is likely you may want to turn
  // ComputePointColorFromValue to On too.
  vtkGetObjectMacro(PointColorTransferFunction, vtkColorTransferFunction);
  virtual void SetPointColorTransferFunction(vtkColorTransferFunction*);

  // Description:
  // Set/Get the window/level mode. In that mode:
  // - the end-points parameter are locked (similar to LockEndPointsParameter)
  // - no point can be added or removed (similar to DisableAddAndRemove)
  // - the first and second point have the same value (they move together)
  // - the last and last-1 point have the same value (they move together) 
  // - the midpoint and sharpness are set to (0.5 / 0.0) and can not be edited
  virtual void SetWindowLevelMode(int);
  vtkBooleanMacro(WindowLevelMode, int);
  vtkGetMacro(WindowLevelMode, int);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the window/level *mode* is changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - new histogram log mode: int
  virtual void SetWindowLevelModeChangedCommand(
    vtkObject *object,const char *method);

  // Description:
  // Set/Get the window/level mode button visibility.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  virtual void SetWindowLevelModeButtonVisibility(int);
  vtkBooleanMacro(WindowLevelModeButtonVisibility, int);
  vtkGetMacro(WindowLevelModeButtonVisibility, int);

  // Description:
  // Set/Get the window/level lock mode. In that mode, provided that
  // WindowLevelMode is On:
  // - the last and last-1 points values are locked (expected to be the same)
  vtkSetMacro(WindowLevelModeLockEndPointValue, int);
  vtkBooleanMacro(WindowLevelModeLockEndPointValue, int);
  vtkGetMacro(WindowLevelModeLockEndPointValue, int);

  // Description:
  // Set/Get the window/level.
  // This method will invoke FunctionChangedCommand. Use 
  // SetInteractiveWindowLevel to invoke FunctionChangingCommand instead.
  virtual void SetWindowLevel(double window, double level);
  virtual void SetInteractiveWindowLevel(double window, double level);
  vtkGetMacro(Window, double);
  vtkGetMacro(Level, double);

  // Description:
  // Set/Get the value entry UI visibility.
  // Not shown if superclass PointEntriesVisibility is set to Off
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ValueEntryVisibility, int);
  virtual void SetValueEntryVisibility(int);
  vtkGetMacro(ValueEntryVisibility, int);

  // Description:
  // Access the entry
  // If you need to customize this object, make sure you first set 
  // ValueEntryVisibility to On and call Create().
  vtkGetObjectMacro(ValueEntry, vtkKWEntryWithLabel);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Proxy to the function. 
  // See protected: section too.
  virtual int HasFunction();
  virtual int GetFunctionSize();
  virtual unsigned long GetFunctionMTime();
  virtual int GetFunctionPointParameter(int id, double *parameter);
  virtual int GetFunctionPointDimensionality();

  // Description:
  // Is point locked, protected, removable ?
  virtual int FunctionPointCanBeAdded();
  virtual int FunctionPointCanBeRemoved(int id);
  virtual int FunctionPointParameterIsLocked(int id);
  virtual int FunctionPointValueIsLocked(int id);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  MoveFunctionPoint(int id,double parameter,const double *values);

  // Description:
  // Callbacks. Internal, do not use.
  virtual void ValueEntryCallback(const char *value);
  virtual void WindowLevelModeCallback(int state);

protected:
  vtkKWPiecewiseFunctionEditor();
  ~vtkKWPiecewiseFunctionEditor();

  // Description:
  // Proxy to the function. 
  // Those are low-level manipulators, they do not check if points can
  // be added/removed/locked, it is up to the higer-level methods to do it.
  // IMPLEMENT those functions in the subclasses.
  // See public: section too.
  virtual int GetFunctionPointValues(int id, double *values);
  virtual int SetFunctionPointValues(int id, const double *values);
  virtual int InterpolateFunctionPointValues(double parameter, double *values);
  virtual int AddFunctionPoint(double parameter, const double *values,int *id);
  virtual int SetFunctionPoint(int id, double parameter, const double *values);
  virtual int RemoveFunctionPoint(int id);
  virtual int FunctionLineIsSampledBetweenPoints(int id1, int id2);
  virtual int GetFunctionPointMidPoint(int id, double *pos);
  virtual int SetFunctionPointMidPoint(int id, double pos);
  virtual int GetFunctionPointSharpness(int id, double *sharpness);
  virtual int SetFunctionPointSharpness(int id, double sharpness);
  virtual int FunctionPointMidPointIsLocked(int id);
  virtual int FunctionPointSharpnessIsLocked(int id);

  virtual int GetMidPointVisibility();

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  GetFunctionPointColorInCanvas(int id, double rgb[3]);

  virtual void UpdatePointEntries(int id);

  //BTX
  virtual void GetLineCoordinates(int id1, int id2, ostrstream *tk_cmd);
  //ETX

  vtkPiecewiseFunction *PiecewiseFunction;
  vtkColorTransferFunction *PointColorTransferFunction;

  int WindowLevelMode;
  int ValueEntryVisibility;
  int WindowLevelModeButtonVisibility;
  int WindowLevelModeLockEndPointValue;

  double Window;
  double Level;

  virtual void UpdatePointsFromWindowLevel(int interactive = 0);
  virtual void UpdateWindowLevelFromPoints();

  virtual void InvokeWindowLevelModeChangedCommand(int mode);
  virtual void InvokeFunctionChangedCommand();
  virtual void InvokeFunctionChangingCommand();

  // Description:
  // Pack the widget
  virtual void Pack();
  virtual void PackPointEntries();

  // Commands

  char  *WindowLevelModeChangedCommand;

  // GUI

  vtkKWEntryWithLabel *ValueEntry;
  vtkKWCheckButton    *WindowLevelModeCheckButton;

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateWindowLevelModeCheckButton();
  virtual void CreateValueEntry();
  virtual int IsTopLeftFrameUsed();
  virtual int IsPointEntriesFrameUsed();

  virtual unsigned long GetRedrawFunctionTime();

private:
  vtkKWPiecewiseFunctionEditor(const vtkKWPiecewiseFunctionEditor&); // Not implemented
  void operator=(const vtkKWPiecewiseFunctionEditor&); // Not implemented
};

#endif

