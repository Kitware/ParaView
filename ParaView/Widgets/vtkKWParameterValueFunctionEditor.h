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
// .NAME vtkKWParameterValueFunctionEditor - a parameter/value function editor
// .SECTION Description
// A widget that allows the user to edit a parameter/value function.

#ifndef __vtkKWParameterValueFunctionEditor_h
#define __vtkKWParameterValueFunctionEditor_h

#include "vtkKWLabeledWidget.h"

class vtkKWFrame;
class vtkKWIcon;
class vtkKWLabel;
class vtkKWLabeledLabel;
class vtkKWRange;

//BTX
class ostrstream;
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWParameterValueFunctionEditor : public vtkKWLabeledWidget
{
public:
  vtkTypeRevisionMacro(vtkKWParameterValueFunctionEditor,vtkKWLabeledWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char *args = 0);

  // Description:
  // Set/Get the whole parameter range.
  virtual float* GetWholeParameterRange();
  virtual void SetWholeParameterRange(float r0, float r1);
  virtual void GetWholeParameterRange(float &r0, float &r1)
    { r0 = this->GetWholeParameterRange()[0]; 
    r1 = this->GetWholeParameterRange()[1]; }
  virtual void GetWholeParameterRange(float range[2])
    { this->GetWholeParameterRange(range[0], range[1]); };
  virtual void SetWholeParameterRange(float range[2]) 
    { this->SetWholeParameterRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible parameter range in the editor.
  virtual float* GetVisibleParameterRange();
  virtual void SetVisibleParameterRange(float r0, float r1);
  virtual void GetVisibleParameterRange(float &r0, float &r1)
    { r0 = this->GetVisibleParameterRange()[0]; 
    r1 = this->GetVisibleParameterRange()[1]; }
  virtual void GetVisibleParameterRange(float range[2])
    { this->GetVisibleParameterRange(range[0], range[1]); };
  virtual void SetVisibleParameterRange(float range[2]) 
    { this->SetVisibleParameterRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible parameter range in the editor as relative positions
  // in the whole parameter range.
  virtual void SetRelativeVisibleParameterRange(float r0, float r1);
  virtual void GetRelativeVisibleParameterRange(float &r0, float &r1);
  virtual void GetRelativeVisibleParameterRange(float range[2])
    { this->GetRelativeVisibleParameterRange(range[0], range[1]); };
  virtual void SetRelativeVisibleParameterRange(float range[2]) 
    { this->SetRelativeVisibleParameterRange(range[0], range[1]); };

  // Description:
  // Set/Get the whole value range.
  virtual float* GetWholeValueRange();
  virtual void SetWholeValueRange(float r0, float r1);
  virtual void GetWholeValueRange(float &r0, float &r1)
    { r0 = this->GetWholeValueRange()[0]; 
    r1 = this->GetWholeValueRange()[1]; }
  virtual void GetWholeValueRange(float range[2])
    { this->GetWholeValueRange(range[0], range[1]); };
  virtual void SetWholeValueRange(float range[2]) 
    { this->SetWholeValueRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible value range.
  virtual float* GetVisibleValueRange();
  virtual void SetVisibleValueRange(float r0, float r1);
  virtual void GetVisibleValueRange(float &r0, float &r1)
    { r0 = this->GetVisibleValueRange()[0]; 
    r1 = this->GetVisibleValueRange()[1]; }
  virtual void GetVisibleValueRange(float range[2])
    { this->GetVisibleValueRange(range[0], range[1]); };
  virtual void SetVisibleValueRange(float range[2]) 
    { this->SetVisibleValueRange(range[0], range[1]); };

  // Description:
  // Set/Get the visible value range in the editor as relative positions
  // in the whole value range.
  virtual void SetRelativeVisibleValueRange(float r0, float r1);
  virtual void GetRelativeVisibleValueRange(float &r0, float &r1);
  virtual void GetRelativeVisibleValueRange(float range[2])
    { this->GetRelativeVisibleValueRange(range[0], range[1]); };
  virtual void SetRelativeVisibleValueRange(float range[2]) 
    { this->SetRelativeVisibleValueRange(range[0], range[1]); };

  // Description:
  // Set/Get the canvas height in pixels (i.e. the drawable region)
  // Get the canvas width (it can not be set, as it will expand automatically)
  virtual void SetCanvasHeight(int);
  vtkGetMacro(CanvasHeight, float);
  vtkGetMacro(CanvasWidth, float);
  
  // Description:
  // Set/Get if the end-points of the function are locked (they can not
  // be removed or can only be moved in the value space).
  vtkSetMacro(LockEndPoints, int);
  vtkBooleanMacro(LockEndPoints, int);
  vtkGetMacro(LockEndPoints, int);

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
  virtual void SetSelectedPointRadius(float);
  vtkGetMacro(SelectedPointRadius, float);

  // Description:
  // Select/Deselect a point, get the selected point (-1 if none selected)
  virtual void SelectPoint(int id);
  vtkGetMacro(SelectedPoint, int);
  virtual void ClearSelection();

  // Description:
  // Merge all the points from another function editor.
  // Return the number of points merged.
  virtual int MergePointsFromEditor(
    vtkKWParameterValueFunctionEditor *editor);

  // Description:
  // Synchronize points with a given editor. 
  // First it will make sure both editors have the same points in the
  // parameter space (by calling MergePointsFromEditor on each other).
  // Then each time a point is added, moved or removed through 
  // user interaction, the same point is altered in the synchronized editor,
  // and vice-versa.
  // Multiple editors can be synchronized (note that if you synchronize A
  // with B, you do not need to synchronize B with A, this is a double-link).
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizePointsWithEditor(
    vtkKWParameterValueFunctionEditor *editor);
  virtual int DoNotSynchronizePointsWithEditor(
    vtkKWParameterValueFunctionEditor *editor);

  // Description:
  // Synchronize the visible parameter range with a given editor. 
  // Each time the current visible range is changed, the same visible range
  // is assigned to the synchronized editor, and vice-versa.
  // Multiple editors can be synchronized (note that if you synchronize A
  // with B, you do not need to synchronize B with A, this is a double-link).
  // Return 1 on success, 0 otherwise.
  virtual int SynchronizeVisibleParameterRangeWithEditor(
    vtkKWParameterValueFunctionEditor *editor);
  virtual int DoNotSynchronizeVisibleParameterRangeWithEditor(
    vtkKWParameterValueFunctionEditor *editor);

  // Description:
  // Set/Get the point color. 
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(PointColor, float);
  virtual void SetPointColor(float r, float g, float b);
  virtual void SetPointColor(float rgb[3])
    { this->SetPointColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the selected point color.
  // Overriden by ComputePointColorFromValue if supported.
  vtkGetVector3Macro(SelectedPointColor, float);
  virtual void SetSelectedPointColor(float r, float g, float b);
  virtual void SetSelectedPointColor(float rgb[3])
    { this->SetSelectedPointColor(rgb[0], rgb[1], rgb[2]); };

  // Description:
  // Set all points color to be a function of the value (might not be
  // supported/implemented in subclasses). Override PointColor.
  vtkBooleanMacro(ComputePointColorFromValue, int);
  virtual void SetComputePointColorFromValue(int);
  vtkGetMacro(ComputePointColorFromValue, int);
  
  // Description:
  // Set commands.
  // Point... commands are passed the index of the point that is/was modified.
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
  virtual void InvokePointRemovedCommand(int id);
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
  // Set the string that enables balloon help for this widget.
  // Override to pass down to children.
  virtual void SetBalloonHelpString(const char *str);
  virtual void SetBalloonHelpJustification(int j);

  // Description:
  // Callbacks
  virtual void ConfigureCallback();
  virtual void VisibleParameterRangeChangingCallback();
  virtual void VisibleParameterRangeChangedCallback();
  virtual void VisibleValueRangeChangingCallback();
  virtual void VisibleValueRangeChangedCallback();
  virtual void StartInteractionCallback(int x, int y);
  virtual void MovePointCallback(int x, int y, int shift);
  virtual void EndInteractionCallback(int x, int y);

  // Description:
  // Access to the ranges.
  vtkGetObjectMacro(ParameterRange, vtkKWRange);
  vtkGetObjectMacro(ValueRange, vtkKWRange);

  // Description:
  // Access to the canvas and internal elements
  vtkGetObjectMacro(Canvas, vtkKWWidget);
  vtkGetObjectMacro(TitleFrame, vtkKWFrame);
  vtkGetObjectMacro(InfoFrame, vtkKWFrame);
  vtkGetObjectMacro(InfoLabel, vtkKWLabeledLabel);

  // Description:
  // Update the whole UI depending on the value of the Ivars
  virtual void Update();

protected:
  vtkKWParameterValueFunctionEditor();
  ~vtkKWParameterValueFunctionEditor();

  int   CanvasHeight;
  int   CanvasWidth;
  int   LockEndPoints;
  int   DisableAddAndRemove;
  int   PointRadius;
  float SelectedPointRadius;
  int   DisableCommands;
  int   SelectedPoint;

  float PointColor[3];
  float SelectedPointColor[3];
  int   ComputePointColorFromValue;

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
  virtual void InvokePointCommand(const char *command, int id);
  virtual void SetObjectMethodCommand(char **command, 
                                      vtkKWObject *object, const char *method);

  // GUI

  vtkKWWidget       *Canvas;
  vtkKWRange        *ParameterRange;
  vtkKWRange        *ValueRange;
  vtkKWFrame        *TitleFrame;
  vtkKWFrame        *InfoFrame;
  vtkKWLabeledLabel *InfoLabel;

  //BTX
  enum 
  {
    ICON_AXES = 0,
    ICON_MOVE,
    ICON_MOVE_H,
    ICON_MOVE_V,
    ICON_TRASHCAN
  };
  vtkKWImageLabel   **Icons;
  //ETX

  // Description:
  // Bind/Unbind all components.
  virtual void Bind();
  virtual void UnBind();

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

  // Description:
  // Redraw or update canvas elements
  virtual void RedrawCanvas();
  virtual void RedrawCanvasElements();
  //BTX
  virtual void RedrawCanvasPoint(int id, ostrstream *tk_cmd = 0);
  //ETX

  unsigned long LastRedrawCanvasElementsTime;
  double        LastRelativeVisibleParameterRange;
  double        LastRelativeVisibleValueRange;

  int           LastSelectCanvasCoordinates[2];
  int           LastConstrainedMove;
  //BTX
  enum
  {
    CONSTRAINED_MOVE_FREE,
    CONSTRAINED_MOVE_H,
    CONSTRAINED_MOVE_V
  };
  //ETX

  // Description:
  // Update the info label according to the current visible parameter and
  // value ranges
  virtual void UpdateInfoLabelWithRange();

  // Description:
  // Convenience method to look for a tag in the Canvas. 
  // Return the number of elements matching tag+suffix.
  virtual int CanvasHasTag(const char *tag, int *suffix = 0);

  // Description:
  // Is point locked, protected ?
  virtual int FunctionPointIsRemovable(int id);
  virtual int FunctionPointParameterIsLocked(int id);
  virtual int FunctionPointValueIsLocked(int id);

  // Description:
  // Proxy to the function. 
  // Only those functions need to be implemented in the subclasses.
  virtual int  HasFunction() = 0;
  virtual int  GetFunctionSize() = 0;
  virtual int  GetFunctionPointColor(int id, float rgb[3]);
  virtual int  GetFunctionPointParameter(int id, float &parameter) = 0;
  virtual int  GetFunctionPointCanvasCoordinates(int id, int &x, int &y) = 0;
  virtual int  AddFunctionPointAtCanvasCoordinates(int x, int y, int &id) = 0;
  virtual int  AddFunctionPointAtParameter(float parameter, int &id) = 0;
  virtual int  MoveFunctionPointToCanvasCoordinates(int id,int x,int y) = 0;
  virtual int  RemoveFunctionPoint(int id) = 0;
  virtual void UpdateInfoLabelWithFunctionPoint(int id) = 0;
  virtual unsigned long GetFunctionMTime() = 0;

  // Synchronized editors

  //BTX

  class SynchronizedEditorSlot
  {
  public:
    enum
    {
      SYNC_VISIBLE_PARAMETER_RANGE = 1,
      SYNC_POINTS                  = 2
    };
    int Options;
    vtkKWParameterValueFunctionEditor *Editor;
  };

  typedef vtkLinkedList<SynchronizedEditorSlot*> 
          SynchronizedEditorsContainer;
  typedef vtkLinkedListIterator<SynchronizedEditorSlot*> 
          SynchronizedEditorsContainerIterator;
  SynchronizedEditorsContainer *SynchronizedEditors;

  virtual void DeleteAllSynchronizedEditors();
  SynchronizedEditorSlot* GetSynchronizedEditorSlot(
    vtkKWParameterValueFunctionEditor *editor);
  virtual int AddSynchronizedEditor(
    vtkKWParameterValueFunctionEditor *editor);
  virtual int RemoveSynchronizedEditor(
    vtkKWParameterValueFunctionEditor *editor);
  virtual int AddSynchronizedEditorOption(
    vtkKWParameterValueFunctionEditor *editor, int option);
  virtual int RemoveSynchronizedEditorOption(
    vtkKWParameterValueFunctionEditor *editor, int option);

  virtual void SynchronizePointsWithAllEditors();
  virtual void SynchronizeVisibleParameterRangeWithAllEditors();

  //ETX

  // Description:
  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWParameterValueFunctionEditor(const vtkKWParameterValueFunctionEditor&); // Not implemented
  void operator=(const vtkKWParameterValueFunctionEditor&); // Not implemented
};

#endif

