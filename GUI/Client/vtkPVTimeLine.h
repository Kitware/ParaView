/*=========================================================================

  Module:    vtkPVTimeLine.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTimeLine
// .SECTION Description
// This is the GUI for an vtkSMKeyFrameAnimationCueManipulatorProxy.
// It basically if a timeline. It has two modes of operation, one in which
// it has a proxy associated with it. And another in which it is merely a GUI
// element with no proxy associated with it. The latter can be used 
// the timeline for a group of animatable elements/properties. When no proxy
// is associated with the timeline, it can only have two end points.
#ifndef __vtkPVTimeLine_h
#define __vtkPVTimeLine_h

#include "vtkKWParameterValueFunctionEditor.h"

class vtkPVAnimationCue;
class vtkPVTraceHelper;

class VTK_EXPORT vtkPVTimeLine : public vtkKWParameterValueFunctionEditor
{
public:
  static vtkPVTimeLine* New();
  vtkTypeRevisionMacro(vtkPVTimeLine, vtkKWParameterValueFunctionEditor);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication* app, const char* args);
  
  virtual int HasFunction();
  virtual int GetFunctionSize();

  // Description:
  // Get the parameter bounds of this timeline ie.
  // from what time to what time does this timeline stretch.
  // Returns 1 on success else 0.
  int GetParameterBounds(double* bounds);

  // Description:
  // Remove all key frames.
  void RemoveAll();

  // Description:
  // Methods to move start/end points of the timeline
  // with/without scaling.
  void MoveStartToParameter(double parameter, int enable_scaling);
  void MoveEndToParameter(double parameter, int enable_scaling);
 
  void GetFocus();
  void RemoveFocus();

  // Description:
  // Get/Set the color to be used for the timeline when it has the focus and
  // when it doens't have the focus.
  vtkSetVector3Macro(ActiveColor, double);
  vtkGetVector3Macro(ActiveColor, double);
  vtkSetVector3Macro(InactiveColor, double);
  vtkGetVector3Macro(InactiveColor, double);

  int HasFocus() {return this->Focus; }

  void SetAnimationCue(vtkPVAnimationCue* cue);

  // Description:
  // Set/Get the time position marker for the timeline.
  void SetTimeMarker(double time);
  double GetTimeMarker();

  // Description:
  // Must be called if the keyframes are changed externally. 
  // This will redraw the timeline.
  void ForceUpdate();

  // Description:
  // Most of these callbacks are overridden only to save
  // the trace appropriately.
  virtual void StartInteractionCallback(int x, int y);
  virtual void MovePointCallback(int x, int y, int shift);
  virtual void EndInteractionCallback(int x, int y);
  virtual void ParameterCursorStartInteractionCallback(int x);
  virtual void ParameterCursorEndInteractionCallback();
  virtual void ParameterCursorMoveCallback(int x);

  // Description:
  // Get the trace helper framework.
  vtkGetObjectMacro(TraceHelper, vtkPVTraceHelper);

protected:
  vtkPVTimeLine();
  ~vtkPVTimeLine();

  // To override the manipulation of the start point in certain cases.
  virtual int FunctionPointParameterIsLocked(int id);
  virtual int FunctionPointCanBeMovedToParameter(int id, double parameter);
  //vtkKWParameterValueFunctionInterface methods.
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
  // Overridden to control focus.
  virtual void InvokeSelectionChangedCommand();
  double ActiveColor[3];
  double InactiveColor[3];

  int OldSelection;
  int Focus;
  vtkPVAnimationCue* AnimationCue;
  vtkPVTraceHelper* TraceHelper;

private:
  vtkPVTimeLine(const vtkPVTimeLine&); // Not implemented.
  void operator=(const vtkPVTimeLine&); // Not implemented.
};


#endif

