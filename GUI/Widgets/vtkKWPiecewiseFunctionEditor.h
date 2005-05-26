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

#ifndef __vtkKWPiecewiseFunctionEditor_h
#define __vtkKWPiecewiseFunctionEditor_h

#include "vtkKWParameterValueFunctionEditor.h"

class vtkKWCheckButton;
class vtkPiecewiseFunction;

class KWWIDGETS_EXPORT vtkKWPiecewiseFunctionEditor : public vtkKWParameterValueFunctionEditor
{
public:
  static vtkKWPiecewiseFunctionEditor* New();
  vtkTypeRevisionMacro(vtkKWPiecewiseFunctionEditor,vtkKWParameterValueFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the function
//BTX
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction*);
//ETX

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

  // Description:
  // Pack the widget
  virtual void Pack();

  // Description:
  // Set/Get the window/level mode. In that mode:
  // - the end-points parameter are locked (similar to LockEndPointsParameter)
  // - no point can be added or removed (similar to DisableAddAndRemove)
  // - the first and second point have the same value (they move together)
  // - the last and last-1 point have the same value (they move together) 
  virtual void SetWindowLevelMode(int);
  vtkBooleanMacro(WindowLevelMode, int);
  vtkGetMacro(WindowLevelMode, int);

  // Description:
  // Show/Hide the window/level mode button.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  virtual void SetShowWindowLevelModeButton(int);
  vtkBooleanMacro(ShowWindowLevelModeButton, int);
  vtkGetMacro(ShowWindowLevelModeButton, int);

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
  // Set commands.
  virtual void SetWindowLevelModeChangedCommand(
    vtkKWObject* object,const char *method);
  virtual void InvokeWindowLevelModeChangedCommand();
  virtual void InvokeFunctionChangedCommand();
  virtual void InvokeFunctionChangingCommand();

  // Description:
  // Callbacks
  virtual void ValueEntryCallback();
  virtual void WindowLevelModeCallback();

  // Description:
  // Show the value entry UI.
  // Note: set this parameter to the proper value before calling Create() in
  // order to minimize the footprint of the object.
  vtkBooleanMacro(ShowValueEntry, int);
  virtual void SetShowValueEntry(int);
  vtkGetMacro(ShowValueEntry, int);

  // Description:
  // Access the entry
  // If you need to customize this object, make sure you first set 
  // ShowValueEntry to On and call Create().
  vtkGetObjectMacro(ValueEntry, vtkKWEntryLabeled);

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
  // IMPLEMENT those functions in the subclasses.
  // See protected: section too.
  virtual int HasFunction();
  virtual int GetFunctionSize();

protected:
  vtkKWPiecewiseFunctionEditor();
  ~vtkKWPiecewiseFunctionEditor();

  // Description:
  // Is point locked, protected, removable ?
  // Likely to be overriden in subclasses.
  virtual int FunctionPointCanBeAdded();
  virtual int FunctionPointCanBeRemoved(int id);
  virtual int FunctionPointParameterIsLocked(int id);
  virtual int FunctionPointValueIsLocked(int id);

  // Description:
  // Proxy to the function. 
  // Those are low-level manipulators, they do not check if points can
  // be added/removed/locked, it is up to the higer-level methods to do it.
  // IMPLEMENT those functions in the subclasses.
  // See public: section too.
  virtual unsigned long GetFunctionMTime();
  virtual int GetFunctionPointParameter(int id, double *parameter);
  virtual int GetFunctionPointDimensionality();
  virtual int GetFunctionPointValues(int id, double *values);
  virtual int SetFunctionPointValues(int id, const double *values);
  virtual int InterpolateFunctionPointValues(double parameter, double *values);
  virtual int AddFunctionPoint(double parameter, const double *values, int *id);
  virtual int SetFunctionPoint(int id, double parameter, const double *values);
  virtual int RemoveFunctionPoint(int id);

  // Description:
  // Higher-level methods to manipulate the function. 
  virtual int  MoveFunctionPoint(int id,double parameter,const double *values);

  virtual void UpdatePointEntries(int id);

  vtkPiecewiseFunction *PiecewiseFunction;

  int WindowLevelMode;
  int ShowValueEntry;
  int ShowWindowLevelModeButton;
  int WindowLevelModeLockEndPointValue;
  double Window;
  double Level;

  virtual void UpdatePointsFromWindowLevel(int interactive = 0);
  virtual void UpdateWindowLevelFromPoints();

  // Commands

  char  *WindowLevelModeChangedCommand;

  // GUI

  vtkKWEntryLabeled *ValueEntry;
  vtkKWCheckButton  *WindowLevelModeCheckButton;

  // Description:
  // Create some objects on the fly (lazy creation, to allow for a smaller
  // footprint)
  virtual void CreateWindowLevelModeCheckButton(vtkKWApplication *app);
  virtual void CreateValueEntry(vtkKWApplication *app);
  virtual int IsTopLeftFrameUsed();
  virtual int IsTopRightFrameUsed();

private:
  vtkKWPiecewiseFunctionEditor(const vtkKWPiecewiseFunctionEditor&); // Not implemented
  void operator=(const vtkKWPiecewiseFunctionEditor&); // Not implemented
};

#endif

