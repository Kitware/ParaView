/*=========================================================================

  Module:    vtkPVTimeLine.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVTimeLine.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWEvent.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVTraceHelper.h"

vtkStandardNewMacro(vtkPVTimeLine);
vtkCxxRevisionMacro(vtkPVTimeLine, "1.7");

//----------------------------------------------------------------------------
vtkPVTimeLine::vtkPVTimeLine()
{
  this->TraceHelper = vtkPVTraceHelper::New();
  this->TraceHelper->SetObject(this);

  this->OldSelection = 0;
  this->ActiveColor[0] = 0.83;
  this->ActiveColor[1] = 0.83;
  this->ActiveColor[2] = 0.83;
  this->InactiveColor[0] = 0.75;
  this->InactiveColor[1] = 0.75;
  this->InactiveColor[2] = 0.75;
  this->SetFrameBackgroundColor(this->InactiveColor);
  this->Focus = 0;
  this->AnimationCue = 0;

  this->ShowValueRangeOff();
  this->ShowParameterRangeOff();
  this->ShowParameterEntryOff();
  this->ShowLabelOff();
  this->ShowCanvasOutlineOn();
  this->ShowFunctionLineOn();
  this->SetFunctionLineWidth(2);
  this->ShowParameterCursorOn();
  this->SetParameterCursorPosition(0.0);
  this->LockEndPointsParameterOff();
  this->ShowRangeLabelOff();
}

//----------------------------------------------------------------------------
vtkPVTimeLine::~vtkPVTimeLine()
{
  this->SetAnimationCue(0);

  if (this->TraceHelper)
    {
    this->TraceHelper->Delete();
    this->TraceHelper = NULL;
    }
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::HasFunction()
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::SetTimeMarker(double time)
{
  if (time < 0 || time > 1)
    {
    vtkErrorMacro("time must be between 0 and 1");
    return;
    }
  this->SetParameterCursorPosition(time);
}

//----------------------------------------------------------------------------
double vtkPVTimeLine::GetTimeMarker()
{
  return this->GetParameterCursorPosition();
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::ForceUpdate()
{
  this->RedrawFunctionDependentElements();
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::Create(vtkKWApplication* app, const char* args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("vtkPVTimeLine already created");
    return;
    }
  this->Superclass::Create(app, args);
}

//----------------------------------------------------------------------------
unsigned long vtkPVTimeLine::GetFunctionMTime()
{
  return this->AnimationCue->GetKeyFramesMTime();
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::GetFunctionSize()
{
  return this->AnimationCue->GetNumberOfKeyFrames();
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::GetFunctionPointParameter(int id, double *parameter)
{
  if (id < 0 || id >= this->GetFunctionSize()) {return 0;}
  *parameter = this->AnimationCue->GetKeyFrameTime(id);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::GetFunctionPointDimensionality()
{
  // TODO: we are lying about the dimensionality, but this simplifies our job
  // until I understand vtkKWParameterValueFunctionEditor properly.
  return 3;
}

//----------------------------------------------------------------------------
// In our case, the parameter and the values are no different.
int vtkPVTimeLine::GetFunctionPointValues(int id, double * values)
{
  return this->GetFunctionPointParameter(id, values);
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::SetFunctionPointValues(int id, const double* values)
{
  //TODO: when is this called?
  if (id < 0 || id >= this->GetFunctionSize()) {return 0;}
  this->AnimationCue->SetKeyFrameTime(id, values[0]);
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::SetAnimationCue(vtkPVAnimationCue* cue)
{
  this->AnimationCue = cue;
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::InterpolateFunctionPointValues(double parameter,
  double * values)
{
  values[0] = parameter;
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::AddFunctionPoint(double vtkNotUsed(parameter), const double* values,
  int *id)
{
  double keyframe_time = values[0];
  *id = this->AnimationCue->AddNewKeyFrame(keyframe_time);
  if (*id == -1)
    {
    vtkErrorMacro("Failed to add point");
    return 0;
    }
  return 1;
}


//----------------------------------------------------------------------------
int vtkPVTimeLine::SetFunctionPoint(int id, double parameter, 
  const double* vtkNotUsed(values))
{

  if (id < 0 || id >= this->GetFunctionSize()) {return 0;}
  this->AnimationCue->SetKeyFrameTime(id, parameter);
  return 1;
  
  
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::RemoveFunctionPoint(int id)
{
  if (id < 0 || id >=this->GetFunctionSize()) {return 0;}
  return this->AnimationCue->RemoveKeyFrame(id);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::RemoveAll()
{
  int old_disable_redraw = this->GetDisableRedraw();
  this->SetDisableRedraw(1);
  int size = 0;
  while ((size = this->GetFunctionSize()) > 0)
    {
    if (!this->RemovePoint(size-1))
      {
      vtkErrorMacro("Error while removing all points");
      break;
      }
    }
  this->SetDisableRedraw(old_disable_redraw);
  this->RedrawFunctionDependentElements();
}

//----------------------------------------------------------------------------
int vtkPVTimeLine::GetParameterBounds(double * bounds)
{
  if (!this->GetFunctionPointParameter(0, &bounds[0]))
    {
    return 0;
    }
  if (!this->GetFunctionPointParameter(
      this->GetFunctionSize()-1, &bounds[1]))
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::MoveStartToParameter(double parameter, 
  int enable_scaling)
{
  int old_scaling = this->GetRescaleBetweenEndPoints();
  if (enable_scaling)
    {
    this->RescaleBetweenEndPointsOn();
    }
 
  this->MoveFunctionPointToParameter(0, parameter, 0);
  this->SetRescaleBetweenEndPoints(old_scaling);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::MoveEndToParameter(double parameter,
  int enable_scaling)
{
  int old_scaling = this->GetRescaleBetweenEndPoints();
  if (enable_scaling)
    {
    this->RescaleBetweenEndPointsOn();
    }

  this->MoveFunctionPointToParameter(this->GetFunctionSize()-1, parameter, 0);
  this->SetRescaleBetweenEndPoints(old_scaling);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::GetFocus()
{
  if (!this->HasFocus())
    {
    if (!this->HasSelection())
      {
      this->SelectPoint(this->OldSelection);
      }
    this->SetFrameBackgroundColor(this->ActiveColor);
    this->Focus = 1;
    this->InvokeEvent(vtkKWEvent::FocusInEvent);
    }
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::RemoveFocus()
{
  if (this->HasFocus())
    {
    if (this->HasSelection())
      {
      this->OldSelection = this->SelectedPoint;
      this->ClearSelection();
      }
    this->SetFrameBackgroundColor(this->InactiveColor);
    this->Focus = 0;
    this->InvokeEvent(vtkKWEvent::FocusOutEvent);
    }
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::InvokeSelectionChangedCommand()
{
  this->Superclass::InvokeSelectionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::StartInteractionCallback(int x, int y)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) StartInteractionCallback %d %d", 
    this->GetTclName(), x, y);
  vtkPVApplication::SafeDownCast(this->GetApplication())->GetMainWindow()
    ->ShowAnimationPanes();
  if (!this->HasFocus())
    {
    this->GetFocus();
    }
  this->Superclass::StartInteractionCallback(x,y);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::MovePointCallback(int x, int y, int shift)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) MovePointCallback %d %d %d",
    this->GetTclName(), x, y, shift);
  this->Superclass::MovePointCallback(x, y, shift);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::EndInteractionCallback(int x, int y)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) EndInteractionCallback %d %d",
    this->GetTclName(), x, y);
  this->Superclass::EndInteractionCallback(x, y);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::ParameterCursorStartInteractionCallback(int x)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) ParameterCursorStartInteractionCallback %d",
    this->GetTclName(), x);
  this->Superclass::ParameterCursorStartInteractionCallback(x);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::ParameterCursorMoveCallback(int x)
{
  this->GetTraceHelper()->AddEntry("$kw(%s) ParameterCursorMoveCallback %d",
    this->GetTclName(), x);
  this->Superclass::ParameterCursorMoveCallback(x);
}

//----------------------------------------------------------------------------
void vtkPVTimeLine::ParameterCursorEndInteractionCallback()
{
  this->GetTraceHelper()->AddEntry("$kw(%s) ParameterCursorEndInteractionCallback",
    this->GetTclName());
  this->Superclass::ParameterCursorEndInteractionCallback();
}


//----------------------------------------------------------------------------
int vtkPVTimeLine::FunctionPointParameterIsLocked(int id)
{
  if (id == 0 && this->GetFunctionSize() > 1)
    {
    return 1;
    }
  return this->Superclass::FunctionPointParameterIsLocked(id);
}
//----------------------------------------------------------------------------
int vtkPVTimeLine::FunctionPointCanBeMovedToParameter(int id, double parameter)
{
  if (id == 0)
    {
    return 0;
    }
  return this->Superclass::FunctionPointCanBeMovedToParameter(id, parameter);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void vtkPVTimeLine::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "InactiveColor: " << this->InactiveColor[0] << ", "
    << this->InactiveColor[1] << ", " << this->InactiveColor[2] << endl;
  os << indent << "ActiveColor: " << this->ActiveColor[0] << ", "
    << this->ActiveColor[1] << ", " << this->ActiveColor[2] << endl;
  os << indent << "Focus: " << this->Focus << endl;
  os << indent << "OldSelection: " << this->OldSelection << endl;
  os << indent << "TraceHelper: " << this->TraceHelper << endl;
}
