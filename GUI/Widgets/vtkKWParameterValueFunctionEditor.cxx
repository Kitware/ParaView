/*=========================================================================

  Module:    vtkKWParameterValueFunctionEditor.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWParameterValueFunctionEditor.h"

#include "vtkCallbackCommand.h"
#include "vtkImageData.h"
#include "vtkKWApplication.h"
#include "vtkKWCanvas.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWHistogram.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWRange.h"
#include "vtkKWTkUtilities.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkImageBlend.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWIcon.h"

#include <vtkstd/string>

vtkCxxRevisionMacro(vtkKWParameterValueFunctionEditor, "1.24");

int vtkKWParameterValueFunctionEditorCommand(ClientData cd, Tcl_Interp *interp, int argc, char *argv[]);

//----------------------------------------------------------------------------
#define VTK_KW_PVFE_POINT_RADIUS_MIN         2

#define VTK_KW_PVFE_CANVAS_BORDER            1
#define VTK_KW_PVFE_CANVAS_WIDTH_MIN         (5 + 10)
#define VTK_KW_PVFE_CANVAS_HEIGHT_MIN        10
#define VTK_KW_PVFE_CANVAS_DELETE_MARGIN     35

#define VTK_KW_PVFE_TICKS_TEXT_SIZE          7
#define VTK_KW_PVFE_TICKS_SEP                2
#define VTK_KW_PVFE_TICKS_VALUE_CANVAS_WIDTH ((int)ceil((double)VTK_KW_PVFE_TICKS_TEXT_SIZE * 6.2))
#define VTK_KW_PVFE_TICKS_PARAMETER_CANVAS_HEIGHT ((int)ceil((double)VTK_KW_PVFE_TICKS_TEXT_SIZE * 1.45))
  
// For some reasons, the end-point of a line/rectangle is not drawn on Win32. 
// Comply with that.

#ifndef _WIN32
#define LSTRANGE 0
#else
#define LSTRANGE 1
#endif
#define RSTRANGE 1

#define VTK_KW_PVFE_TESTING 0

//----------------------------------------------------------------------------
vtkKWParameterValueFunctionEditor::vtkKWParameterValueFunctionEditor()
{
  this->ShowParameterRange          = 1;
  this->ShowValueRange              = 1;
  this->PointPositionInValueRange   = vtkKWParameterValueFunctionEditor::PointPositionAtValue;
  this->ParameterRangePosition      = vtkKWParameterValueFunctionEditor::ParameterRangePositionAtBottom;
  this->CanvasHeight                = 55;
  this->CanvasWidth                 = 55;
  this->ExpandCanvasWidth           = 1;
  this->LockEndPointsParameter      = 0;
  this->RescaleBetweenEndPoints     = 0;
  this->DisableAddAndRemove         = 0;
  this->DisableRedraw               = 0;
  this->PointRadius                 = 4;
  this->SelectedPointRadius         = 1.45;
  this->DisableCommands             = 0;
  this->SelectedPoint               = -1;
  this->FunctionLineWidth           = 2;
  this->PointOutlineWidth           = 1;
  this->FunctionLineStyle           = vtkKWParameterValueFunctionEditor::LineStyleSolid;
  this->PointGuidelineStyle         = vtkKWParameterValueFunctionEditor::LineStyleDash;
  this->PointStyle                  = vtkKWParameterValueFunctionEditor::PointStyleDisc;
  this->FirstPointStyle             = vtkKWParameterValueFunctionEditor::PointStyleDefault;
  this->LastPointStyle              = vtkKWParameterValueFunctionEditor::PointStyleDefault;
  this->ShowCanvasOutline           = 1;
  this->CanvasOutlineStyle          = vtkKWParameterValueFunctionEditor::CanvasOutlineStyleAllSides;
  this->ShowParameterTicks          = 0;
  this->ShowValueTicks              = 0;
  this->ComputeValueTicksFromHistogram = 0;
  this->ShowCanvasBackground        = 1;
  this->ShowFunctionLine            = 1;
  this->ShowPointIndex              = 0;
  this->ShowPointGuideline          = 0;
  this->ShowSelectedPointIndex      = 1;
  this->LabelPosition               = vtkKWParameterValueFunctionEditor::LabelPositionAtDefault;
  this->ShowRangeLabel              = 1;
  this->RangeLabelPosition         = vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault;
  this->ParameterEntryPosition         = vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault;
  this->ShowParameterEntry          = 1;
  this->ShowUserFrame               = 0;
  this->PointMarginToCanvas         = vtkKWParameterValueFunctionEditor::PointMarginAllSides;
  this->TicksLength                 = 5;
  this->NumberOfParameterTicks      = 6;
  this->NumberOfValueTicks          = 6;
  this->ValueTicksCanvasWidth       = VTK_KW_PVFE_TICKS_VALUE_CANVAS_WIDTH;
  this->ChangeMouseCursor          = 1;

  this->ParameterTicksFormat        = NULL;
  this->SetParameterTicksFormat("%-#6.3g");
  this->ValueTicksFormat            = NULL;
  this->SetValueTicksFormat(this->GetParameterTicksFormat());
  this->ParameterEntryFormat        = NULL;

#if 1
  this->FrameBackgroundColor[0]     = 0.25;
  this->FrameBackgroundColor[1]     = 0.56;
  this->FrameBackgroundColor[2]     = 0.77;
#else
  this->FrameBackgroundColor[0]     = 0.83;
  this->FrameBackgroundColor[1]     = 0.83;
  this->FrameBackgroundColor[2]     = 0.83;
#endif

  this->HistogramColor[0]           = 0.63;
  this->HistogramColor[1]           = 0.63;
  this->HistogramColor[2]           = 0.63;

  this->ShowHistogramLogModeOptionMenu  = 0;
  this->HistogramLogModeOptionMenu      = vtkKWOptionMenu::New();
  this->HistogramLogModeChangedCommand  = NULL;

  this->SecondaryHistogramColor[0]  = 0.0;
  this->SecondaryHistogramColor[1]  = 0.0;
  this->SecondaryHistogramColor[2]  = 0.0;

  this->ComputeHistogramColorFromValue = 0;
  this->HistogramStyle     = vtkKWHistogram::ImageDescriptor::STYLE_BARS;
  this->SecondaryHistogramStyle     = vtkKWHistogram::ImageDescriptor::STYLE_DOTS;

  this->ParameterCursorColor[0]     = 0.2;
  this->ParameterCursorColor[1]     = 0.2;
  this->ParameterCursorColor[2]     = 0.4;

  this->PointColor[0]               = 1.0;
  this->PointColor[1]               = 1.0;
  this->PointColor[2]               = 1.0;

  this->SelectedPointColor[0]       = 0.737; // 0.59;
  this->SelectedPointColor[1]       = 0.772; // 0.63;
  this->SelectedPointColor[2]       = 0.956; // 0.82;

  this->PointTextColor[0]           = 0.0;
  this->PointTextColor[1]           = 0.0;
  this->PointTextColor[2]           = 0.0;

  this->SelectedPointTextColor[0]   = 0.0;
  this->SelectedPointTextColor[1]   = 0.0;
  this->SelectedPointTextColor[2]   = 0.0;

  this->ComputePointColorFromValue     = 0;

  this->PointAddedCommand           = NULL;
  this->PointMovingCommand          = NULL;
  this->PointMovedCommand           = NULL;
  this->PointRemovedCommand         = NULL;
  this->SelectionChangedCommand     = NULL;
  this->FunctionChangedCommand      = NULL;
  this->FunctionChangingCommand     = NULL;
  this->VisibleRangeChangedCommand  = NULL;
  this->VisibleRangeChangingCommand = NULL;

  this->Canvas                      = vtkKWCanvas::New();
  this->ParameterRange              = vtkKWRange::New();
  this->ValueRange                  = vtkKWRange::New();
  this->TopLeftContainer            = vtkKWFrame::New();
  this->TopLeftFrame                = vtkKWFrame::New();
  this->UserFrame                   = vtkKWFrame::New();
  this->TopRightFrame               = vtkKWFrame::New();
  this->RangeLabel                  = vtkKWLabel::New();
  this->ParameterEntry              = vtkKWLabeledEntry::New();
  this->ValueTicksCanvas            = vtkKWCanvas::New();
  this->ParameterTicksCanvas        = vtkKWCanvas::New();

  this->DisplayedWholeParameterRange[0] = 0.0;
  this->DisplayedWholeParameterRange[1] = 
    this->DisplayedWholeParameterRange[0];

  this->ShowParameterCursor         = 0;
  this->ParameterCursorPosition     = this->ParameterRange->GetRange()[0];

  this->LastRedrawFunctionTime      = 0;

  this->LastSelectCanvasCoordinates[0]    = 0;
  this->LastSelectCanvasCoordinates[1]    = 0;
  this->LastConstrainedMove               = vtkKWParameterValueFunctionEditor::ConstrainedMoveFree;

  // Synchronization callbacks
  
  this->SynchronizeCallbackCommand = vtkCallbackCommand::New();
  this->SynchronizeCallbackCommand->SetClientData(this); 
  this->SynchronizeCallbackCommand->SetCallback(
    vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents);

  this->SynchronizeCallbackCommand2 = vtkCallbackCommand::New();
  this->SynchronizeCallbackCommand2->SetClientData(this); 
  this->SynchronizeCallbackCommand2->SetCallback(
    vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents2);

  // Histogram

  this->Histogram                         = NULL;
  this->SecondaryHistogram                = NULL;
  this->HistogramImageDescriptor          = NULL;
  this->SecondaryHistogramImageDescriptor = NULL;

  this->LastHistogramBuildTime = 0;
}

//----------------------------------------------------------------------------
vtkKWParameterValueFunctionEditor::~vtkKWParameterValueFunctionEditor()
{
  // Commands

  if (this->PointAddedCommand)
    {
    delete [] this->PointAddedCommand;
    this->PointAddedCommand = NULL;
    }

  if (this->PointMovingCommand)
    {
    delete [] this->PointMovingCommand;
    this->PointMovingCommand = NULL;
    }

  if (this->PointMovedCommand)
    {
    delete [] this->PointMovedCommand;
    this->PointMovedCommand = NULL;
    }

  if (this->PointRemovedCommand)
    {
    delete [] this->PointRemovedCommand;
    this->PointRemovedCommand = NULL;
    }

  if (this->SelectionChangedCommand)
    {
    delete [] this->SelectionChangedCommand;
    this->SelectionChangedCommand = NULL;
    }

  if (this->FunctionChangedCommand)
    {
    delete [] this->FunctionChangedCommand;
    this->FunctionChangedCommand = NULL;
    }

  if (this->FunctionChangingCommand)
    {
    delete [] this->FunctionChangingCommand;
    this->FunctionChangingCommand = NULL;
    }

  if (this->VisibleRangeChangedCommand)
    {
    delete [] this->VisibleRangeChangedCommand;
    this->VisibleRangeChangedCommand = NULL;
    }

  if (this->VisibleRangeChangingCommand)
    {
    delete [] this->VisibleRangeChangingCommand;
    this->VisibleRangeChangingCommand = NULL;
    }

  // GUI

  if (this->Canvas)
    {
    this->Canvas->Delete();
    this->Canvas = NULL;
    }

  if (this->ParameterRange)
    {
    this->ParameterRange->Delete();
    this->ParameterRange = NULL;
    }

  if (this->ValueRange)
    {
    this->ValueRange->Delete();
    this->ValueRange = NULL;
    }

  if (this->TopLeftContainer)
    {
    this->TopLeftContainer->Delete();
    this->TopLeftContainer = NULL;
    }

  if (this->TopLeftFrame)
    {
    this->TopLeftFrame->Delete();
    this->TopLeftFrame = NULL;
    }

  if (this->UserFrame)
    {
    this->UserFrame->Delete();
    this->UserFrame = NULL;
    }

  if (this->TopRightFrame)
    {
    this->TopRightFrame->Delete();
    this->TopRightFrame = NULL;
    }

  if (this->ParameterEntry)
    {
    this->ParameterEntry->Delete();
    this->ParameterEntry = NULL;
    }

  if (this->RangeLabel)
    {
    this->RangeLabel->Delete();
    this->RangeLabel = NULL;
    }

  if (this->SynchronizeCallbackCommand)
    {
    this->SynchronizeCallbackCommand->Delete();
    this->SynchronizeCallbackCommand = NULL;
    }

  if (this->SynchronizeCallbackCommand2)
    {
    this->SynchronizeCallbackCommand2->Delete();
    this->SynchronizeCallbackCommand2 = NULL;
    }

  if (this->ValueTicksCanvas)
    {
    this->ValueTicksCanvas->Delete();
    this->ValueTicksCanvas = NULL;
    }

  if (this->ParameterTicksCanvas)
    {
    this->ParameterTicksCanvas->Delete();
    this->ParameterTicksCanvas = NULL;
    }

  // Histogram

  this->SetHistogram(NULL);
  this->SetSecondaryHistogram(NULL);

  if (this->HistogramImageDescriptor)
    {
    delete this->HistogramImageDescriptor;
    }
  if (this->SecondaryHistogramImageDescriptor)
    {
    delete this->SecondaryHistogramImageDescriptor;
    }
  if (this->HistogramLogModeOptionMenu)
    {
    this->HistogramLogModeOptionMenu->Delete();
    this->HistogramLogModeOptionMenu = NULL;
    }
  if (this->HistogramLogModeChangedCommand)
    {
    delete [] this->HistogramLogModeChangedCommand;
    this->HistogramLogModeChangedCommand = NULL;
    }

  this->SetParameterTicksFormat(NULL);
}

//----------------------------------------------------------------------------
vtkKWParameterValueFunctionEditor::Ranges::Ranges()
{
  this->WholeParameterRange[0] = 0.0;
  this->WholeParameterRange[1] = 0.0;
  this->VisibleParameterRange[0] = 0.0;
  this->VisibleParameterRange[1] = 0.0;

  this->WholeValueRange[0] = 0.0;
  this->WholeValueRange[1] = 0.0;
  this->VisibleValueRange[0] = 0.0;
  this->VisibleValueRange[1] = 0.0;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Ranges::GetRangesFrom(
  vtkKWParameterValueFunctionEditor *editor)
{
  if (!editor)
    {
    return;
    }

  editor->GetWholeParameterRange(this->WholeParameterRange);
  editor->GetVisibleParameterRange(this->VisibleParameterRange);

  editor->GetWholeValueRange(this->WholeValueRange);
  editor->GetVisibleValueRange(this->VisibleValueRange);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::Ranges::HasSameWholeRangesComparedTo(
  vtkKWParameterValueFunctionEditor::Ranges *ranges)
{
  return (ranges &&
          this->WholeParameterRange[0] == ranges->WholeParameterRange[0] &&
          this->WholeParameterRange[1] == ranges->WholeParameterRange[1] &&
          this->WholeValueRange[0]     == ranges->WholeValueRange[0] &&
          this->WholeValueRange[1]     == ranges->WholeValueRange[1]);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::Ranges::NeedResizeComparedTo(
  vtkKWParameterValueFunctionEditor::Ranges *ranges)
{
  if (!ranges)
    {
    return 0;
    }

  if (!this->HasSameWholeRangesComparedTo(ranges))
    {
    return 1;
    }

  double p_v_rel = fabs(
    (this->VisibleParameterRange[1] - this->VisibleParameterRange[0]) / 
    (this->WholeParameterRange[1] - this->WholeParameterRange[0]));

  double p_v_rel_r = fabs(
    (ranges->VisibleParameterRange[1] - ranges->VisibleParameterRange[0]) / 
    (ranges->WholeParameterRange[1] - ranges->WholeParameterRange[0]));

  double v_v_rel = fabs(
    (this->VisibleValueRange[1] - this->VisibleValueRange[0]) / 
    (this->WholeValueRange[1] - this->WholeValueRange[0]));

  double v_v_rel_r = fabs(
    (ranges->VisibleValueRange[1] - ranges->VisibleValueRange[0]) / 
    (ranges->WholeValueRange[1] - ranges->WholeValueRange[0]));

  return (fabs(p_v_rel - p_v_rel_r) > 0.001 ||
          fabs(v_v_rel - v_v_rel_r) > 0.001);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::Ranges::NeedPanOnlyComparedTo(
  vtkKWParameterValueFunctionEditor::Ranges *ranges)
{
  if (this->NeedResizeComparedTo(ranges))
    {
    return 0;
    }
 
  return
    (this->VisibleParameterRange[0] != ranges->VisibleParameterRange[0] ||
     this->VisibleParameterRange[1] != ranges->VisibleParameterRange[1] ||
     this->VisibleValueRange[0] != ranges->VisibleValueRange[0] ||
     this->VisibleValueRange[1] != ranges->VisibleValueRange[1]);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FunctionPointCanBeAdded()
{
  return !this->DisableAddAndRemove;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FunctionPointCanBeRemoved(int id)
{
  // Usually if the parameter is locked, the point should stay

  return (!this->DisableAddAndRemove && 
          !this->FunctionPointParameterIsLocked(id) &&
          !(this->RescaleBetweenEndPoints &&
            (id == 0 || id == this->GetFunctionSize() - 1)));
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FunctionPointParameterIsLocked(int id)
{
  return (this->HasFunction() &&
          this->LockEndPointsParameter &&
          (id == 0 || 
           (this->GetFunctionSize() && id == this->GetFunctionSize() - 1)));
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FunctionPointValueIsLocked(int)
{
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FunctionPointCanBeMovedToParameter(
  int id, double parameter)
{
  if (this->FunctionPointParameterIsLocked(id))
    {
    return 0;
    }

  // Are we within the whole parameter range

  double *p_w_range = this->GetWholeParameterRange();
  if (parameter < p_w_range[0] || parameter > p_w_range[1])
    {
    return 0;
    }
  
  double neighbor_parameter;

  // Check if we are trying to move the point before its previous neighbor

  if (id > 0 && 
      this->GetFunctionPointParameter(id - 1, &neighbor_parameter) &&
      parameter <= neighbor_parameter)
    {
    return 0;
    }

  // Check if we are trying to move the point past its next neighbor

  if (id < this->GetFunctionSize() - 1 && 
      this->GetFunctionPointParameter(id + 1, &neighbor_parameter) &&
      parameter >= neighbor_parameter)
    {
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetFunctionPointColorInCanvas(
  int id, double rgb[3])
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  if (!this->ComputePointColorFromValue)
    {
    if (id == this->SelectedPoint)
      {
      rgb[0] = this->SelectedPointColor[0];
      rgb[1] = this->SelectedPointColor[1];
      rgb[2] = this->SelectedPointColor[2];
      }
    else
      {
      rgb[0] = this->PointColor[0];
      rgb[1] = this->PointColor[1];
      rgb[2] = this->PointColor[2];
      }
  
    return 1;
    }

  // If 3 or 4 components (RGB, RGBA), use the first 3 as R, G, B
  // otherwise use the first one as a level of gray

  double values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  if (!this->GetFunctionPointValues(id, values))
    {
    return 0;
    }

  double *v_w_range = this->GetWholeValueRange();
  int dim = this->GetFunctionPointDimensionality();

  if (dim == 3 || dim == 4)
    {
    for (int i = 0; i < 3; i++)
      {
      rgb[i] = (values[i] - v_w_range[0]) / (v_w_range[1] - v_w_range[0]);
      }
    }
  else
    {
    rgb[0] = rgb[1] = rgb[2] = 
      (values[0] - v_w_range[0]) / (v_w_range[1] - v_w_range[0]);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetFunctionPointTextColorInCanvas(
  int id, double rgb[3])
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  if (!this->ComputePointColorFromValue)
    {
    if (id == this->SelectedPoint)
      {
      rgb[0] = this->SelectedPointTextColor[0];
      rgb[1] = this->SelectedPointTextColor[1];
      rgb[2] = this->SelectedPointTextColor[2];
      }
    else
      {
      rgb[0] = this->PointTextColor[0];
      rgb[1] = this->PointTextColor[1];
      rgb[2] = this->PointTextColor[2];
      }
    return 1;
    }

  // Get the point color

  double prgb[3];
  if (!this->GetFunctionPointColorInCanvas(id, prgb))
    {
    return 0;
    }

  double l = (rgb[0] + rgb[1] + rgb[2]) / 3.0;
  if (l > 0.5)
    {
    rgb[0] = rgb[1] = rgb[2] = 0.0;
    }
  else
    {
    rgb[0] = rgb[1] = rgb[2] = 1.0;
    }
    
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetFunctionPointCanvasCoordinates(
  int id, int &x, int &y)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  x = vtkMath::Round(parameter * factors[0]);

  double *v_w_range = this->GetWholeValueRange();
  double *v_v_range = this->GetVisibleValueRange();

  // If the value is forced to be placed at top

  if (this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtTop)
    {
    y = vtkMath::Round((v_w_range[1] - v_v_range[1]) * factors[1]);
    }

  // If the value is forced to be placed at bottom

  else if (this->PointPositionInValueRange == 
           vtkKWParameterValueFunctionEditor::PointPositionAtBottom)
    {
    y = vtkMath::Round((v_w_range[1] - v_v_range[0]) * factors[1]);
    }

  // If the value is forced to be placed at center, or is multi-dimensional, 
  // just place the point in the middle of the current value range

  else if (this->PointPositionInValueRange == 
           vtkKWParameterValueFunctionEditor::PointPositionAtCenter ||
           this->GetFunctionPointDimensionality() != 1)
    {
    y = vtkMath::Floor(
      (v_w_range[1] - (v_v_range[1] + v_v_range[0]) * 0.5) * factors[1]);
    }

  // The value is mono-dimensional, use it to compute the y coord

  else
    {
    double values[
      vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
    if (!this->GetFunctionPointValues(id, values))
      {
      return 0;
      }
    y = vtkMath::Round((v_w_range[1] - values[0]) * factors[1]);
    }
    
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddFunctionPointAtCanvasCoordinates(
  int x, int y, int &id)
{
  if (!this->IsCreated() || !this->HasFunction() ||
      !this->FunctionPointCanBeAdded())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  double parameter = ((double)x / factors[0]);

  double values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];

  // If the value is multi-dimensional, just add
  // a point interpolated from the function.

  if (this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtCenter ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtTop ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtBottom ||
      this->GetFunctionPointDimensionality() != 1)
    {
    if (!this->InterpolateFunctionPointValues(parameter, values))
      {
      return 0;
      }
    }

  // The value is mono-dimensional, use the y coord to compute it

  else
    {
    double *v_w_range = this->GetWholeValueRange();
    values[0] = (v_w_range[1] - ((double)y / factors[1]));
    }

  // Add the point

  return this->AddFunctionPoint(parameter, values, &id);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddFunctionPointAtParameter(
  double parameter, int &id)
{
  if (!this->HasFunction() || !this->FunctionPointCanBeAdded())
    {
    return 0;
    }

  // Get the interpolated value

  double values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  if (!this->InterpolateFunctionPointValues(parameter, values))
    {
    return 0;
    }

  // Add the point

  return this->AddFunctionPoint(parameter, values, &id);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::MoveFunctionPointToCanvasCoordinates(
  int id, int x, int y)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  // Get the new param given the x coord

  double parameter = ((double)x / factors[0]);

  double values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];

  // The value is multi-dimensional, just move
  // the point in the parameter range, keep the same value.

  if (this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtCenter ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtTop ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionAtBottom ||
      this->GetFunctionPointDimensionality() != 1)
    {
    if (!this->GetFunctionPointValues(id, values))
      {
      return 0;
      }
    }

  // The value is mono-dimensional, use the y coord to compute it

  else
    {
    double *v_w_range = this->GetWholeValueRange();
    values[0] = (v_w_range[1] - ((double)y / factors[1]));
    }

  // Move the point

  return this->MoveFunctionPoint(id, parameter, values);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::MoveFunctionPointToParameter(
  int id, double parameter, int interpolate)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  double values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];

  // Get current value if point value is locked or no interpolation

  if (!interpolate || this->FunctionPointValueIsLocked(id))
    {
    if (!this->GetFunctionPointValues(id, values))
      {
      return 0;
      }
    }
  else
    {
    if (!this->InterpolateFunctionPointValues(parameter, values))
      {
      return 0;
      }
    }
  
  // Move the point

  return this->MoveFunctionPoint(id, parameter, values);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::MoveFunctionPoint(
  int id, double parameter, const double *values)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

  // Get old parameter and values

  double old_parameter;
  double old_values[
    vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
  if (!this->GetFunctionPointParameter(id, &old_parameter) ||
      !this->GetFunctionPointValues(id, old_values))
    {
    return 0;
    }
  
  // Same value, bye

  int equal_values = this->EqualFunctionPointValues(values, old_values);
  if (parameter == old_parameter && equal_values)
    {
    return 0;
    }

  // Check the value constraint, can the value be changed ?

  if (!equal_values && this->FunctionPointValueIsLocked(id))
    {
    values = old_values;
    }

  // Check the parameter constraint, can the (clamped) parameter be changed ?

  if (parameter != old_parameter)
    {
    vtkMath::ClampValue(&parameter, this->GetWholeParameterRange());
    if (!this->FunctionPointCanBeMovedToParameter(id, parameter))
      {
      parameter = old_parameter;
      }
    }

  // Replace the parameter / value

  unsigned long mtime = this->GetFunctionMTime();
  if (!this->SetFunctionPoint(id, parameter, values) ||
      this->GetFunctionMTime() <= mtime)
    {
    return 0;
    }

  // Redraw the point

  this->RedrawPoint(id); 
  if (id == this->SelectedPoint)
    {
    this->UpdatePointEntries(id);
    }

  // If we are moving the end points and we should rescale

  if (this->RescaleBetweenEndPoints &&
      (id == 0 || id == this->GetFunctionSize() - 1))
    {
    this->RescaleFunctionBetweenEndPoints(id, old_parameter);
    }

  this->RedrawFunctionDependentElements();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::EqualFunctionPointValues(
  const double *values1, const double *values2)
{
  if (!values1 || !values2)
    {
    return 0;
    }

  const double *values1_end = values1 + this->GetFunctionPointDimensionality();
  while (values1 < values1_end)
    {
    if (*values1 != *values2)
      {
      return 0;
      }
    values1++;
    values2++;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RescaleFunctionBetweenEndPoints(
  int id, double old_parameter)
{
  if (!this->HasFunction() || this->GetFunctionSize() < 3)
    {
    return;
    }

  int first_id = 0, last_id = this->GetFunctionSize() - 1;

  // Get the current parameters of the end-points

  double first_parameter, last_parameter;
  if (!this->GetFunctionPointParameter(first_id, &first_parameter) ||
      !this->GetFunctionPointParameter(last_id, &last_parameter))
    {
    return;
    }

  // Get the old parameters of the end-points

  double first_old_parameter, last_old_parameter;
  if (id == first_id)
    {
    first_old_parameter = old_parameter;
    last_old_parameter = last_parameter;
    }
  else if (id == last_id)
    {
    first_old_parameter = first_parameter;
    last_old_parameter = old_parameter;
    }
  else
    {
    return;
    }

  double range = (last_parameter - first_parameter);
  double old_range = (last_old_parameter - first_old_parameter);

  // The order the points are modified depends on which end-point we
  // are manipulating, in which direction, otherwise the ordering of 
  // points gets corrupted

  int start_loop, end_loop, inc_loop;
  if ((id == first_id && range > old_range) ||
      (id == last_id && range < old_range))
    {
    start_loop = first_id + 1;
    end_loop = last_id;
    }
  else
    {
    start_loop = last_id - 1;
    end_loop = first_id;
    }
  inc_loop = (start_loop <= end_loop ? 1 : -1);

  // Move all points in between
  // Prevent any redraw first

  int old_disable_redraw = this->GetDisableRedraw();
  this->SetDisableRedraw(1);

  double parameter;
  for (id = start_loop; id != end_loop; id += inc_loop)
    {
    if (this->GetFunctionPointParameter(id, &parameter))
      {
      double old_pos = (parameter - first_old_parameter) / old_range;
      this->MoveFunctionPointToParameter(
        id, first_parameter + old_pos * range, 0);
      }
    }

  this->SetDisableRedraw(old_disable_redraw);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdatePointEntries(
  int id)
{
  this->UpdateParameterEntry(id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Create(vtkKWApplication *app, 
                                               const char *args)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("widget already created " << this->GetClassName());
    return;
    }

  // Call the superclass to create the widget and set the appropriate flags

  this->Superclass::Create(app, args);

  // Create the canvas

  this->Canvas->SetParent(this);
  this->Canvas->Create(app, "-highlightthickness 0 -relief solid -bd 0");

  this->Script("%s config -height %d -width %d",
               this->Canvas->GetWidgetName(), 
               this->CanvasHeight, 
               (this->ExpandCanvasWidth ? 0 : this->CanvasWidth));

  this->Script("bind %s <Configure> {%s ConfigureCallback}",
               this->Canvas->GetWidgetName(), this->GetTclName());

  // Create the ranges
  // Note that if they are created now only if they are supposed to be shown,
  // otherwise they will be created on the fly once the visibility flag is
  // changed. Even if they are not created, the application is set so that
  // the callbacks can be triggered.

  this->ParameterRange->SetOrientationToHorizontal();
  this->ParameterRange->InvertedOff();
  this->ParameterRange->AdjustResolutionOn();
  this->ParameterRange->SetThickness(12);
  this->ParameterRange->SetInternalThickness(0.5);
  this->ParameterRange->SetSliderSize(3);
  this->ParameterRange->SliderCanPushOff();
  this->ParameterRange->ShowLabelOff();
  this->ParameterRange->ShowEntriesOff();
  this->ParameterRange->ShowZoomButtonsOff();
  this->ParameterRange->SetZoomButtonsPositionToAligned();
  this->ParameterRange->SetCommand(
    this, "VisibleParameterRangeChangingCallback");
  this->ParameterRange->SetEndCommand(
    this, "VisibleParameterRangeChangedCallback");

  if (this->ShowParameterRange)
    {
    this->CreateParameterRange(app);
    }
  else
    {
    this->ParameterRange->SetApplication(app);
    }

  this->ValueRange->SetOrientationToVertical();
  this->ValueRange->InvertedOn();
  this->ValueRange->SetAdjustResolution(
    this->ParameterRange->GetAdjustResolution());
  this->ValueRange->SetThickness(
    this->ParameterRange->GetThickness());
  this->ValueRange->SetInternalThickness(
    this->ParameterRange->GetInternalThickness());
  this->ValueRange->SetSliderSize(
    this->ParameterRange->GetSliderSize());
  this->ValueRange->SetSliderCanPush(
    this->ParameterRange->GetSliderCanPush());
  this->ValueRange->SetShowLabel(
    this->ParameterRange->GetShowLabel());
  this->ValueRange->SetShowEntries(
    this->ParameterRange->GetShowEntries());
  this->ValueRange->SetShowZoomButtons(
    this->ParameterRange->GetShowZoomButtons());
  this->ValueRange->SetZoomButtonsPosition(
    this->ParameterRange->GetZoomButtonsPosition());
  this->ValueRange->SetCommand(
    this, "VisibleValueRangeChangingCallback");
  this->ValueRange->SetEndCommand(
    this, "VisibleValueRangeChangedCallback");

  if (this->ShowValueRange)
    {
    this->CreateValueRange(app);
    }
  else
    {
    this->ValueRange->SetApplication(app);
    }

  // Create the top left container
  // It will be created automatically when sub-elements will be created
  // (for ex: UserFrame or TopLeftFrame)

  // Create the top left/right frames only if we know that we are going to
  // need the, (otherwise they will be
  // create on the fly later once elements are moved)

  if (this->IsTopLeftFrameUsed())
    {
    this->CreateTopLeftFrame(app);
    }

  if (this->IsTopRightFrameUsed())
    {
    this->CreateTopRightFrame(app);
    }

  // Create the user frame

  if (this->ShowUserFrame)
    {
    this->CreateUserFrame(app);
    }

  // Create the label now if it has to be shown now

  if (this->ShowLabel)
    {
    this->CreateLabel(app);
    }

  // Create the range label

  if (this->ShowRangeLabel)
    {
    this->CreateRangeLabel(app);
    }

  // Create the top right frame
  // It will be created automatically when sub-elements will be created
  // (for ex: ParameterEntry)

  // Create the parameter entry

  if (this->ShowParameterEntry)
    {
    this->CreateParameterEntry(app);
    }

  // Create the ticks canvas

  if (this->ShowValueTicks)
    {
    this->CreateValueTicksCanvas(app);
    }

  if (this->ShowParameterTicks)
    {
    this->CreateParameterTicksCanvas(app);
    }

  // Histogram log mode

  if (this->ShowHistogramLogModeOptionMenu)
    {
    this->CreateHistogramLogModeOptionMenu(app);
    }

  // Set the bindings

  this->Bind();

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateLabel(vtkKWApplication *app)
{
  // Override the parent's CreateLabel()
  // This will also create the label on the fly, if needed
  
  vtkKWLabel *label = this->GetLabel();
  if (label->IsCreated())
    {
    return;
    }

  label->SetParent(this);
  label->Create(app, "-anchor w -bd 0");
  vtkKWTkUtilities::ChangeFontWeightToBold(
    app->GetMainInterp(), label->GetWidgetName());

  label->SetBalloonHelpString(this->GetBalloonHelpString());
  label->SetBalloonHelpJustification(this->GetBalloonHelpJustification());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateParameterRange(
  vtkKWApplication *app)
{
  if (this->ParameterRange && !this->ParameterRange->IsCreated())
    {
    this->ParameterRange->SetParent(this);
    this->ParameterRange->Create(app);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateValueRange(
  vtkKWApplication *app)
{
  if (this->ValueRange && !this->ValueRange->IsCreated())
    {
    this->ValueRange->SetParent(this);
    this->ValueRange->Create(app);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateRangeLabel(
  vtkKWApplication *app)
{
  if (this->RangeLabel && !this->RangeLabel->IsCreated())
    {
    this->RangeLabel->SetParent(this);
    this->RangeLabel->Create(app, "-bd 0 -anchor w");
    this->UpdateRangeLabel();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateTopRightFrame(
  vtkKWApplication *app)
{
  if (this->TopRightFrame && !this->TopRightFrame->IsCreated())
    {
    this->TopRightFrame->SetParent(this);
    this->TopRightFrame->Create(app, NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateParameterEntry(
  vtkKWApplication *app)
{
  if (this->ParameterEntry && !this->ParameterEntry->IsCreated())
    {
    this->ParameterEntry->SetParent(this);
    this->ParameterEntry->Create(app, "");
    this->ParameterEntry->GetEntry()->SetWidth(9);
    this->ParameterEntry->SetLabel("P:");

    this->UpdateParameterEntry(this->SelectedPoint);

    this->ParameterEntry->GetEntry()->BindCommand(
      this, "ParameterEntryCallback");
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateHistogramLogModeOptionMenu(
  vtkKWApplication *app)
{
  if (this->HistogramLogModeOptionMenu && 
      !this->HistogramLogModeOptionMenu->IsCreated())
    {
    this->CreateTopLeftFrame(app);

    this->HistogramLogModeOptionMenu->SetParent(this->TopLeftFrame);
    this->HistogramLogModeOptionMenu->Create(app, "-padx 1 -pady 0");
    this->HistogramLogModeOptionMenu->IndicatorOff();
    this->HistogramLogModeOptionMenu->SetBalloonHelpString(
      "Change the histogram mode from log to linear.");

    vtkKWIcon *icon = vtkKWIcon::New();
    vtkstd::string img_name;

    icon->SetImage(vtkKWIcon::ICON_GRID_LINEAR);
    img_name = this->HistogramLogModeOptionMenu->GetWidgetName();
    img_name += ".img0";
    vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                  img_name.c_str(), 
                                  icon->GetData(),
                                  icon->GetWidth(),
                                  icon->GetHeight(),
                                  icon->GetPixelSize());
 
    this->HistogramLogModeOptionMenu->AddImageEntryWithCommand(
      img_name.c_str(), this, "HistogramLogModeCallback 0");

    icon->SetImage(vtkKWIcon::ICON_GRID_LOG);
    img_name = this->HistogramLogModeOptionMenu->GetWidgetName();
    img_name += ".img1";
    vtkKWTkUtilities::UpdatePhoto(this->GetApplication()->GetMainInterp(),
                                  img_name.c_str(), 
                                  icon->GetData(),
                                  icon->GetWidth(),
                                  icon->GetHeight(),
                                  icon->GetPixelSize());
 
    this->HistogramLogModeOptionMenu->AddImageEntryWithCommand(
      img_name.c_str(), this, "HistogramLogModeCallback 1");

    icon->Delete();

    this->UpdateHistogramLogModeOptionMenu();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateTopLeftContainer(
  vtkKWApplication *app)
{
  if (this->TopLeftContainer && !this->TopLeftContainer->IsCreated())
    {
    this->TopLeftContainer->SetParent(this);
    this->TopLeftContainer->Create(app, NULL);
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::IsTopLeftFrameUsed()
{
  return ((this->ShowLabel && 
           (this->LabelPosition == 
            vtkKWParameterValueFunctionEditor::LabelPositionAtDefault)) ||
          (this->ShowRangeLabel && 
           (this->RangeLabelPosition == 
            vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault)) ||
          this->ShowHistogramLogModeOptionMenu);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::IsTopRightFrameUsed()
{
  return 
    (this->ShowParameterEntry && 
     (this->ParameterEntryPosition == 
      vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault));
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateTopLeftFrame(
  vtkKWApplication *app)
{
  if (this->TopLeftFrame && !this->TopLeftFrame->IsCreated())
    {
    this->CreateTopLeftContainer(app);
    this->TopLeftFrame->SetParent(this->TopLeftContainer);
    this->TopLeftFrame->Create(app, NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateUserFrame(
  vtkKWApplication *app)
{
  if (this->UserFrame && !this->UserFrame->IsCreated())
    {
    this->CreateTopLeftContainer(app);
    this->UserFrame->SetParent(this->TopLeftContainer);
    this->UserFrame->Create(app, NULL);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateValueTicksCanvas(
  vtkKWApplication *app)
{
  if (this->ValueTicksCanvas && !this->ValueTicksCanvas->IsCreated())
    {
    this->ValueTicksCanvas->SetParent(this);
    this->ValueTicksCanvas->Create(
      app, "-highlightthickness 0 -relief solid -height 0 -bd 0");
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateParameterTicksCanvas(
  vtkKWApplication *app)
{
  if (this->ParameterTicksCanvas && !this->ParameterTicksCanvas->IsCreated())
    {
    this->ParameterTicksCanvas->SetParent(this);
    this->ParameterTicksCanvas->Create(
      app, "-highlightthickness 0 -relief solid -width 0 -bd 0");
    this->Script("%s config -height %d",
                 this->ParameterTicksCanvas->GetWidgetName(), 
                 VTK_KW_PVFE_TICKS_PARAMETER_CANVAS_HEIGHT);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Update()
{
  this->UpdateEnableState();

  this->UpdateRangeLabel();

  this->Redraw();

  if (!this->HasSelection())
    {
    this->ParameterEntry->SetEnabled(0);
    }
  else
    {
    this->UpdatePointEntries(this->SelectedPoint);
    }

  this->UpdateHistogramLogModeOptionMenu();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything

  this->Canvas->UnpackSiblings();

  // Repack everything

  ostrstream tk_cmd;

  /*
    TLC: TopLeftContainer, contains the TopLeftFrame (TLF) and UserFrame (UF)
    TLF: TopLeftFrame, may contain the Label (L) and/or the RangeLabel (RL)...
    L: Label, usually the title of the whole dialog
    RL: RangeLabel, displays the current visible parameter range
    TRF: TopRightFrame, contains the ParameterEntry (PE) and subclasses entries
    PE: Parameter Entry
    PR: Parameter Range
    VR: Value Range
    VT: Value Ticks
    PT: Parameter Ticks
    [---]: Canvas

            a b  c              d   e  f
         +------------------------------
        0|       TLC            TRF             ShowLabel: On
        1|    VT [--------------]   VR          LabelPosition: Default
        2|       PT                             RangeLabelPosition: Default
        3|       PR                   

            a b  c              d   e  f
         +------------------------------
        0|       L              RL              ShowLabel: On
        1|       TLC            TRF             LabelPosition: Top
        2|    VT [--------------]   VR          RangeLabelPosition: Top
        3|       PT 
        4|       PR

            a b  c              d   e  f
         +------------------------------
        0|       TLC            TRF            ShowLabel: On
        1|  L VT [--------------]   VR PE      LabelPosition: Left
        2|       PT                            RangeLabelPosition: Default
        3|       PR                            ParameterEntryPosition: Right

            a b  c              d   e  f
         +------------------------------
        0|       TLC            TRF            ShowLabel: On
        1|       PR                            ParameterEntryPosition: Right
        2|  L VT [--------------]   VR PE      LabelPosition: Left
        3|       PT                            RangeLabelPosition: Default
                                               ParameterRangePosition: Top
  */

  // We need a grid

  int row = 0, row_inc = 0;
  int col_a = 0, col_b = 1, col_c = 2, col_d = 3, col_e = 4, col_f = 5;
  
  // Label (L) if on top, i.e. its on own row not in the top left frame (TLF)

  if (this->ShowLabel && 
      (this->LabelPosition == 
       vtkKWParameterValueFunctionEditor::LabelPositionAtTop) &&
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
           << " -stick wns -padx 0 -pady 0 -in "
           << this->GetWidgetName()
           << " -column " << col_c << " -row " << row << endl;
    row_inc = 1;
    }
  
  // RangeLabel (RL) on top, i.e. on its own row not in the top left frame(TLF)
  
  if (this->ShowRangeLabel && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionAtTop) &&
      this->RangeLabel->IsCreated())
    {
    tk_cmd << "grid " << this->RangeLabel->GetWidgetName() 
           << " -stick ens -padx 0 -pady 0  -in "
           << this->GetWidgetName() 
           << " -column " << col_d << " -row " << row << endl;
    row_inc = 1;
    }

  row += row_inc;
  
  // Top left container (TLC)

  if (this->TopLeftContainer->IsCreated())
    {
    this->TopLeftContainer->UnpackChildren();
    if (this->IsTopLeftFrameUsed() || this->ShowUserFrame)
      {
      tk_cmd << "grid " << this->TopLeftContainer->GetWidgetName() 
             << " -stick ewns -pady 1 "
             << " -column " << col_c << " -row " << row << endl;
      }
    }

  // Top left frame (TLF) and User frame (UF)
  // inside the top left container (TLC)
  
  if (this->TopLeftFrame->IsCreated())
    {
    this->TopLeftFrame->UnpackChildren();
    if (this->IsTopLeftFrameUsed())
      {
      tk_cmd << "pack " << this->TopLeftFrame->GetWidgetName()
             << " -side left -fill both -padx 0 -pady 0" << endl;
      }
    }

  if (this->UserFrame->IsCreated())
    {
    tk_cmd << "pack " << this->UserFrame->GetWidgetName() 
           << " -side left -fill both -padx 0 -pady 0" << endl;
    }

  // Label (L) at default position, i.e inside top left frame (TLF)

  if (this->ShowLabel && 
      (this->LabelPosition == 
       vtkKWParameterValueFunctionEditor::LabelPositionAtDefault) &&
      this->HasLabel() && this->GetLabel()->IsCreated() &&
      this->TopLeftFrame->IsCreated())
    {
    tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
           << " -side left -fill both -padx 0 -pady 0 -in " 
           << this->TopLeftFrame->GetWidgetName() << endl;
    }
  
  // Histogram log mode (in top left frame)

  if (this->ShowHistogramLogModeOptionMenu &&
      this->HistogramLogModeOptionMenu->IsCreated())
    {
    tk_cmd << "pack " << this->HistogramLogModeOptionMenu->GetWidgetName() 
           << " -side left -fill both -padx 0" << endl;
    }
  
  // RangeLabel (RL) at default position, i.e. inside top left frame (TLF)

  if (this->ShowRangeLabel && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault) &&
      this->RangeLabel->IsCreated() &&
      this->TopLeftFrame->IsCreated())
    {
    tk_cmd << "pack " << this->RangeLabel->GetWidgetName() 
           << " -side left -fill both -padx 0 -pady 0 -in " 
           << this->TopLeftFrame->GetWidgetName() << endl;
    }
  
  // TopRightFrame (TRF)

  if (this->TopRightFrame->IsCreated())
    {
    this->TopRightFrame->UnpackChildren();
    if (this->IsTopRightFrameUsed())
      {
      tk_cmd << "grid " << this->TopRightFrame->GetWidgetName() 
             << " -stick ens -pady 1"
             << " -column " << col_d << " -row " << row << endl;
      }
    }
  
  // ParameterEntry (PE) if at default position inside top right frame (TRF)
  
  if (this->ShowParameterEntry && 
      (this->ParameterEntryPosition == 
       vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault) &&
      this->ParameterEntry->IsCreated() &&
      this->TopRightFrame->IsCreated())
    {
    tk_cmd << "pack " << this->ParameterEntry->GetWidgetName() 
           << " -side left -padx 2 -in "
           << this->TopRightFrame->GetWidgetName() << endl;
    }
  
  row++;
  
  // Parameter range (PR) if at top
  
  if (this->ShowParameterRange && 
      this->ParameterRange->IsCreated() &&
      (this->ParameterRangePosition == 
       vtkKWParameterValueFunctionEditor::ParameterRangePositionAtTop))
    {
    tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 2"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    row++;
    }

  // Label (L) if at left

  if (this->ShowLabel && 
      (this->LabelPosition == 
       vtkKWParameterValueFunctionEditor::LabelPositionAtLeft) &&
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
           << " -stick wns -padx 0 -pady 0 -in "
           << this->GetWidgetName() 
           << " -column " << col_a << " -row " << row << endl;
    }
  
  // Value Ticks (VT)
  
  if (this->ShowValueTicks && this->ValueTicksCanvas->IsCreated())
    {
    tk_cmd << "grid " << this->ValueTicksCanvas->GetWidgetName() 
           << " -sticky ns -padx 0 -pady 0 "
           << " -column " << col_b << " -row " << row << endl;
    }
  
  // Canvas ([------])
  
  tk_cmd << "grid " << this->Canvas->GetWidgetName() 
         << " -sticky news -padx 0 -pady 0 "
         << " -columnspan 2 -column " << col_c << " -row " << row << endl;
  
  // Value range (VR)
  
  if (this->ShowValueRange && this->ValueRange->IsCreated())
    {
    tk_cmd << "grid " << this->ValueRange->GetWidgetName() 
           << " -sticky ns -padx 2 -pady 0 "
           << " -column " << col_e << " -row " << row << endl;
    }
  
  // ParameterEntry (PE) if at right
  
  if (this->ShowParameterEntry && 
      (this->ParameterEntryPosition == 
       vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtRight) &&
      this->ParameterEntry->IsCreated())
    {
    tk_cmd << "grid " << this->ParameterEntry->GetWidgetName() 
           << " -sticky wns -padx 2 -pady 0 -in "
           << this->GetWidgetName() 
           << " -column " << col_f << " -row " << row << endl;
    }

  tk_cmd << "grid rowconfigure " 
         << this->GetWidgetName() << " " << row << " -weight 1" << endl;
  
  row++;
    
  // Parameter Ticks (PT)
  
  if (this->ShowParameterTicks && this->ParameterTicksCanvas->IsCreated())
    {
    tk_cmd << "grid " << this->ParameterTicksCanvas->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 0"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    row++;
    }
  
  // Parameter range (PR)
  
  if (this->ShowParameterRange && 
      this->ParameterRange->IsCreated() &&
      (this->ParameterRangePosition == 
       vtkKWParameterValueFunctionEditor::ParameterRangePositionAtBottom))
    {
    tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 2"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    }
  
  // Make sure it will resize properly
  
  tk_cmd << "grid columnconfigure " 
         << this->GetWidgetName() << " " << col_c << " -weight 1" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Bind()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();

    // Mouse motion

    tk_cmd << "bind " <<  canv
           << " <ButtonPress-1> {" << this->GetTclName() 
           << " StartInteractionCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_POINT_TAG
           << " <B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 0}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_TEXT_TAG
           << " <B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 0}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_POINT_TAG
           << " <Shift-B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 1}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_TEXT_TAG
           << " <Shift-B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 1}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_POINT_TAG 
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " EndInteractionCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_TEXT_TAG 
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " EndInteractionCallback %%x %%y}" << endl;

    // Key bindings

    tk_cmd << "bind " <<  canv
           << " <Enter> {" << this->GetTclName() 
           << " CanvasEnterCallback}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-n> {" << this->GetTclName() 
           << " SelectNextPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Next> {" << this->GetTclName() 
           << " SelectNextPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-p> {" << this->GetTclName() 
           << " SelectPreviousPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Prior> {" << this->GetTclName() 
           << " SelectPreviousPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Home> {" << this->GetTclName() 
           << " SelectFirstPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-End> {" << this->GetTclName() 
           << " SelectLastPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-x> {" << this->GetTclName() 
           << " RemoveSelectedPoint}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Delete> {" << this->GetTclName() 
           << " RemoveSelectedPoint}" << endl;

    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UnBind()
{
  if (!this->IsCreated())
    {
    return;
    }

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();

    // Mouse motion

    tk_cmd << "bind " << canv
           << " <ButtonPress-1> {}" << endl;
    
    tk_cmd << canv << " bind " << VTK_KW_PVFE_POINT_TAG 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_TEXT_TAG 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_POINT_TAG 
           << " <Shift-B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_TEXT_TAG 
           << " <Shift-B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_POINT_TAG 
           << " <ButtonRelease-1> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_PVFE_TEXT_TAG 
           << " <ButtonRelease-1> {}" << endl;

    // Key bindings

    tk_cmd << "bind " <<  canv
           << " <Enter> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-n> {}"<< endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Next> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-p> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Prior> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Home> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-End> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-x> {}" << endl;

    tk_cmd << "bind " <<  canv
           << " <KeyPress-Delete> {}" << endl;
    }
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetWholeParameterRange()
{
  return this->ParameterRange->GetWholeRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeParameterRange(
  double r0, double r1)
{
  this->ParameterRange->SetWholeRange(r0, r1);

  this->Redraw();
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetVisibleParameterRange()
{
  return this->ParameterRange->GetRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleParameterRange(
  double r0, double r1)
{
  this->ParameterRange->SetRange(r0, r1); 

  // VisibleParameterRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleParameterRange(
  double &r0, double &r1)
{
  this->ParameterRange->GetRelativeRange(r0, r1);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRelativeVisibleParameterRange(
  double r0, double r1)
{
  this->ParameterRange->SetRelativeRange(r0, r1);

  // VisibleParameterRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeParameterRangeAndMaintainVisible(
  double r0, double r1)
{
  double range[2];
  this->GetRelativeVisibleParameterRange(range);
  this->ParameterRange->SetWholeRange(r0, r1);
  if (range[0] == range[1]) // avoid getting stuck
    {
    range[0] = 0.0;
    range[1] = 1.0;
    }
  this->SetRelativeVisibleParameterRange(range);

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowParameterRange(int arg)
{
  if (this->ShowParameterRange == arg)
    {
    return;
    }

  this->ShowParameterRange = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ShowParameterRange && this->IsCreated())
    {
    this->CreateParameterRange(this->GetApplication());
    }

  this->Modified();

  this->Pack();
  this->UpdateRangeLabel();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterRangePosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::ParameterRangePositionAtTop)
    {
    arg = vtkKWParameterValueFunctionEditor::ParameterRangePositionAtTop;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::ParameterRangePositionAtBottom)
    {
    arg = vtkKWParameterValueFunctionEditor::ParameterRangePositionAtBottom;
    }

  if (this->ParameterRangePosition == arg)
    {
    return;
    }

  this->ParameterRangePosition = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetWholeValueRange()
{
  return this->ValueRange->GetWholeRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeValueRange(double r0, double r1)
{
  this->ValueRange->SetWholeRange(r0, r1);

  this->Redraw();
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetVisibleValueRange()
{
  return this->ValueRange->GetRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleValueRange(
  double r0, double r1)
{
  this->ValueRange->SetRange(r0, r1);

  // VisibleValueRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleValueRange(
  double &r0, double &r1)
{
  this->ValueRange->GetRelativeRange(r0, r1);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRelativeVisibleValueRange(
  double r0, double r1)
{
  this->ValueRange->SetRelativeRange(r0, r1);

  // VisibleValueRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeValueRangeAndMaintainVisible(
  double r0, double r1)
{
  double range[2];
  this->GetRelativeVisibleValueRange(range);
  this->ValueRange->SetWholeRange(r0, r1);
  if (range[0] == range[1]) // avoid getting stuck
    {
    range[0] = 0.0;
    range[1] = 1.0;
    }
  this->SetRelativeVisibleValueRange(range);

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowValueRange(int arg)
{
  if (this->ShowValueRange == arg)
    {
    return;
    }

  this->ShowValueRange = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ShowValueRange && this->IsCreated())
    {
    this->CreateValueRange(this->GetApplication());
    }

  this->Modified();

  this->Pack();
  this->UpdateRangeLabel();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointPositionInValueRange(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointPositionAtValue)
    {
    arg = vtkKWParameterValueFunctionEditor::PointPositionAtValue;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::PointPositionAtCenter)
    {
    arg = vtkKWParameterValueFunctionEditor::PointPositionAtCenter;
    }

  if (this->PointPositionInValueRange == arg)
    {
    return;
    }

  this->PointPositionInValueRange = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowLabel(int arg)
{
  // If we are displaying the label in the top left frame, make sure it has
  // been created before we call the superclass (which will call our 
  // overriden CreateLabel()).

  if (arg && 
      (this->LabelPosition == 
       vtkKWParameterValueFunctionEditor::LabelPositionAtDefault) &&
      this->IsCreated())
    {
    this->CreateTopLeftFrame(this->GetApplication());
    }

  this->Superclass::SetShowLabel(arg);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetLabelPosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::LabelPositionAtDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::LabelPositionAtDefault;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::LabelPositionAtLeft)
    {
    arg = vtkKWParameterValueFunctionEditor::LabelPositionAtLeft;
    }

  if (this->LabelPosition == arg)
    {
    return;
    }

  this->LabelPosition = arg;

  // If we are displaying the label in the top left frame, make sure it has
  // been created. 

  if (this->ShowLabel && 
      (this->LabelPosition == 
       vtkKWParameterValueFunctionEditor::LabelPositionAtDefault) &&
      this->IsCreated())
    {
    this->CreateTopLeftFrame(this->GetApplication());
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowRangeLabel(int arg)
{
  if (this->ShowRangeLabel == arg)
    {
    return;
    }

  this->ShowRangeLabel = arg;

  // If we are displaying the range label in the top left frame, make sure it
  // has been created. 

  if (this->ShowRangeLabel && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault) &&
      this->IsCreated())
    {
    this->CreateTopLeftFrame(this->GetApplication());
    }

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ShowRangeLabel && this->IsCreated())
    {
    this->CreateRangeLabel(this->GetApplication());
    }

  this->UpdateRangeLabel();

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRangeLabelPosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::RangeLabelPositionAtTop)
    {
    arg = vtkKWParameterValueFunctionEditor::RangeLabelPositionAtTop;
    }

  if (this->RangeLabelPosition == arg)
    {
    return;
    }

  this->RangeLabelPosition = arg;

  // If we are displaying the range label in the top left frame, make sure it
  // has been created. 

  if (this->ShowRangeLabel && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionAtDefault) &&
      this->IsCreated())
    {
    this->CreateTopLeftFrame(this->GetApplication());
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowParameterEntry(int arg)
{
  if (this->ShowParameterEntry == arg)
    {
    return;
    }

  this->ShowParameterEntry = arg;

  // If we are displaying the entry in the top right frame, make sure it
  // has been created. 

  if (this->ShowParameterEntry && 
      (this->ParameterEntryPosition == 
       vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault) &&
      this->IsCreated())
    {
    this->CreateTopRightFrame(this->GetApplication());
    }

  // Make sure that if the entry has to be shown, we create it on the fly if
  // needed

  if (this->ShowParameterEntry && this->IsCreated())
    {
    this->CreateParameterEntry(this->GetApplication());
    }

  this->UpdateParameterEntry(this->SelectedPoint);

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterEntryPosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtRight)
    {
    arg = vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtRight;
    }

  if (this->ParameterEntryPosition == arg)
    {
    return;
    }

  this->ParameterEntryPosition = arg;

  // If we are displaying the entry in the top right frame, make sure it
  // has been created. 

  if (this->ShowParameterEntry && 
      (this->ParameterEntryPosition == 
       vtkKWParameterValueFunctionEditor::ParameterEntryPositionAtDefault) &&
      this->IsCreated())
    {
    this->CreateTopRightFrame(this->GetApplication());
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterEntryFormat(const char *arg)
{
  if (this->ParameterEntryFormat == NULL && arg == NULL) 
    { 
    return;
    }

  if (this->ParameterEntryFormat && arg && 
      (!strcmp(this->ParameterEntryFormat, arg))) 
    {
    return;
    }

  if (this->ParameterEntryFormat) 
    { 
    delete [] this->ParameterEntryFormat; 
    }

  if (arg)
    {
    this->ParameterEntryFormat = new char[strlen(arg) + 1];
    strcpy(this->ParameterEntryFormat, arg);
    }
  else
    {
    this->ParameterEntryFormat = NULL;
    }

  this->Modified();
  
  this->UpdateParameterEntry(this->SelectedPoint);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowUserFrame(int arg)
{
  if (this->ShowUserFrame == arg)
    {
    return;
    }

  this->ShowUserFrame = arg;

  // Make sure that if the frame has to be shown, we create it on the fly if
  // needed

  if (this->ShowUserFrame && this->IsCreated())
    {
    this->CreateUserFrame(this->GetApplication());
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasHeight(int arg)
{
  if (this->CanvasHeight == arg || arg < VTK_KW_PVFE_CANVAS_HEIGHT_MIN)
    {
    return;
    }

  this->CanvasHeight = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasWidth(int arg)
{
  if (this->CanvasWidth == arg || arg < VTK_KW_PVFE_CANVAS_WIDTH_MIN)
    {
    return;
    }

  this->CanvasWidth = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetExpandCanvasWidth(int arg)
{
  if (this->ExpandCanvasWidth == arg)
    {
    return;
    }

  this->ExpandCanvasWidth = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowFunctionLine(int arg)
{
  if (this->ShowFunctionLine == arg)
    {
    return;
    }

  this->ShowFunctionLine = arg;

  this->Modified();

  // Remove the polyline in the canvas. RedrawFunction only changes items
  // coordinates if they already exist. To make sure the line is hidden,
  // we have to remove the item.

  if (!this->ShowFunctionLine)
    {
    this->CanvasRemoveTag(VTK_KW_PVFE_LINE_TAG);
    }

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionLineWidth(int arg)
{
  if (this->FunctionLineWidth == arg)
    {
    return;
    }

  this->FunctionLineWidth = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointOutlineWidth(int arg)
{
  if (this->PointOutlineWidth == arg)
    {
    return;
    }

  this->PointOutlineWidth = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionLineStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::LineStyleSolid)
    {
    arg = vtkKWParameterValueFunctionEditor::LineStyleSolid;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::LineStyleDash)
    {
    arg = vtkKWParameterValueFunctionEditor::LineStyleDash;
    }

  if (this->FunctionLineStyle == arg)
    {
    return;
    }

  this->FunctionLineStyle = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointStyleDisc)
    {
    arg = vtkKWParameterValueFunctionEditor::PointStyleDisc;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::PointStyleDefault;
    }

  if (this->PointStyle == arg)
    {
    return;
    }

  this->PointStyle = arg;

  this->Modified();

  // Remove all points in the canvas. Point styles actually map to different
  // canvas items (rectangle, oval, polygons). If we don't purge, only the
  // coordinates of the items will be changed, and this will produce
  // unexpected results. 

  this->CanvasRemoveTag(VTK_KW_PVFE_POINT_TAG);

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFirstPointStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointStyleDisc)
    {
    arg = vtkKWParameterValueFunctionEditor::PointStyleDisc;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::PointStyleDefault;
    }

  if (this->FirstPointStyle == arg)
    {
    return;
    }

  this->FirstPointStyle = arg;

  this->Modified();

  // Remove all points in the canvas. Point styles actually map to different
  // canvas items (rectangle, oval, polygons). If we don't purge, only the
  // coordinates of the items will be changed, and this will produce
  // unexpected results. 

  this->CanvasRemoveTag(VTK_KW_PVFE_POINT_TAG);

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetLastPointStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointStyleDisc)
    {
    arg = vtkKWParameterValueFunctionEditor::PointStyleDisc;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::PointStyleDefault;
    }

  if (this->LastPointStyle == arg)
    {
    return;
    }

  this->LastPointStyle = arg;

  this->Modified();

  // Remove all points in the canvas. Point styles actually map to different
  // canvas items (rectangle, oval, polygons). If we don't purge, only the
  // coordinates of the items will be changed, and this will produce
  // unexpected results. 

  this->CanvasRemoveTag(VTK_KW_PVFE_POINT_TAG);

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowCanvasOutline(int arg)
{
  if (this->ShowCanvasOutline == arg)
    {
    return;
    }

  this->ShowCanvasOutline = arg;

  this->Modified();

  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasOutlineStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::CanvasOutlineStyleLeftSide)
    {
    arg = vtkKWParameterValueFunctionEditor::CanvasOutlineStyleLeftSide;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::CanvasOutlineStyleAllSides)
    {
    arg = vtkKWParameterValueFunctionEditor::CanvasOutlineStyleAllSides;
    }

  if (this->CanvasOutlineStyle == arg)
    {
    return;
    }

  this->CanvasOutlineStyle = arg;

  this->Modified();

  // Remove the outline now. This will force the style to be re-created
  // properly. 

  this->CanvasRemoveTag(VTK_KW_PVFE_FRAME_FG_TAG);

  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowCanvasBackground(int arg)
{
  if (this->ShowCanvasBackground == arg)
    {
    return;
    }

  this->ShowCanvasBackground = arg;

  this->Modified();

  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowParameterCursor(int arg)
{
  if (this->ShowParameterCursor == arg)
    {
    return;
    }

  this->ShowParameterCursor = arg;

  this->Modified();

  this->RedrawParameterCursor();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterCursorPosition(double arg)
{
  vtkMath::ClampValue(&arg, this->GetWholeParameterRange());
  
  if (this->ParameterCursorPosition == arg)
    {
    return;
    }

  this->ParameterCursorPosition = arg;

  this->Modified();

  this->RedrawParameterCursor();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowParameterTicks(int arg)
{
  if (this->ShowParameterTicks == arg)
    {
    return;
    }

  this->ShowParameterTicks = arg;

  this->Modified();

  if (this->ShowParameterTicks && this->IsCreated())
    {
    this->CreateParameterTicksCanvas(this->GetApplication());
    }

  this->RedrawRangeTicks();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowValueTicks(int arg)
{
  if (this->ShowValueTicks == arg)
    {
    return;
    }

  this->ShowValueTicks = arg;

  this->Modified();

  if (this->ShowValueTicks && this->IsCreated())
    {
    this->CreateValueTicksCanvas(this->GetApplication());
    }

  this->RedrawRangeTicks();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetComputeValueTicksFromHistogram(int arg)
{
  if (this->ComputeValueTicksFromHistogram == arg)
    {
    return;
    }

  this->ComputeValueTicksFromHistogram = arg;

  this->Modified();

  this->RedrawRangeTicks();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointRadius(int arg)
{
  if (this->PointRadius == arg || arg < VTK_KW_PVFE_POINT_RADIUS_MIN)
    {
    return;
    }

  this->PointRadius = arg;

  this->Modified();

  if (this->PointMarginToCanvas != 
      vtkKWParameterValueFunctionEditor::PointMarginNone)
    {
    this->Redraw();
    }
  else
    {
    this->RedrawFunction();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointRadius(double arg)
{
  if (this->SelectedPointRadius == arg || arg < 0.0)
    {
    return;
    }

  this->SelectedPointRadius = arg;

  this->Modified();

  if (this->PointMarginToCanvas != 
      vtkKWParameterValueFunctionEditor::PointMarginNone)
    {
    this->Redraw();
    }
  else
    {
    this->RedrawPoint(this->SelectedPoint);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetTicksLength(int arg)
{
  if (this->TicksLength == arg || arg < 1)
    {
    return;
    }

  this->TicksLength = arg;

  this->Modified();

  if (this->ShowParameterTicks || this->ShowValueTicks)
    {
    this->Redraw();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetNumberOfParameterTicks(int arg)
{
  if (this->NumberOfParameterTicks == arg || arg < 0)
    {
    return;
    }

  this->NumberOfParameterTicks = arg;

  this->Modified();

  this->CanvasRemoveTag(VTK_KW_PVFE_PARAMETER_TICKS_TAG);
  if (this->ParameterTicksCanvas->IsCreated())
    {
    this->CanvasRemoveTag(VTK_KW_PVFE_PARAMETER_TICKS_TAG, 
                          this->ParameterTicksCanvas->GetWidgetName());
    }

  if (this->ShowParameterTicks || this->ShowValueTicks)
    {
    this->RedrawRangeTicks();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterTicksFormat(const char *arg)
{
  if (this->ParameterTicksFormat == NULL && arg == NULL) 
    { 
    return;
    }

  if (this->ParameterTicksFormat && arg && 
      (!strcmp(this->ParameterTicksFormat, arg))) 
    {
    return;
    }

  if (this->ParameterTicksFormat) 
    { 
    delete [] this->ParameterTicksFormat; 
    }

  if (arg)
    {
    this->ParameterTicksFormat = new char[strlen(arg) + 1];
    strcpy(this->ParameterTicksFormat, arg);
    }
  else
    {
    this->ParameterTicksFormat = NULL;
    }

  this->Modified();
  
  if (this->ShowParameterTicks)
    {
    this->RedrawRangeTicks();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetNumberOfValueTicks(int arg)
{
  if (this->NumberOfValueTicks == arg || arg < 0)
    {
    return;
    }

  this->NumberOfValueTicks = arg;

  this->Modified();

  this->CanvasRemoveTag(VTK_KW_PVFE_VALUE_TICKS_TAG);
  if (this->ValueTicksCanvas->IsCreated())
    {
    this->CanvasRemoveTag(VTK_KW_PVFE_VALUE_TICKS_TAG, 
                          this->ValueTicksCanvas->GetWidgetName());
    }

  if (this->ShowParameterTicks || this->ShowValueTicks)
    {
    this->RedrawRangeTicks();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetValueTicksCanvasWidth(int arg)
{
  if (this->ValueTicksCanvasWidth == arg || arg < 0)
    {
    return;
    }

  this->ValueTicksCanvasWidth = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetValueTicksFormat(const char *arg)
{
  if (this->ValueTicksFormat == NULL && arg == NULL) 
    { 
    return;
    }

  if (this->ValueTicksFormat && arg && (!strcmp(this->ValueTicksFormat, arg))) 
    {
    return;
    }

  if (this->ValueTicksFormat) 
    { 
    delete [] this->ValueTicksFormat; 
    }

  if (arg)
    {
    this->ValueTicksFormat = new char[strlen(arg) + 1];
    strcpy(this->ValueTicksFormat, arg);
    }
  else
    {
    this->ValueTicksFormat = NULL;
    }

  this->Modified();
  
  if (this->ShowValueTicks)
    {
    this->RedrawRangeTicks();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvas(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointMarginNone)
    {
    arg = vtkKWParameterValueFunctionEditor::PointMarginNone;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::PointMarginAllSides)
    {
    arg = vtkKWParameterValueFunctionEditor::PointMarginAllSides;
    }

  if (this->PointMarginToCanvas == arg)
    {
    return;
    }

  this->PointMarginToCanvas = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFrameBackgroundColor(
  double r, double g, double b)
{
  if ((r == this->FrameBackgroundColor[0] &&
       g == this->FrameBackgroundColor[1] &&
       b == this->FrameBackgroundColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->FrameBackgroundColor[0] = r;
  this->FrameBackgroundColor[1] = g;
  this->FrameBackgroundColor[2] = b;

  this->Modified();

  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetBackgroundColor(int r, int g, int b)
{
  this->Superclass::SetBackgroundColor(r, g, b);
  if (this->Canvas)
    {
    this->Canvas->SetBackgroundColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramColor(
  double r, double g, double b)
{
  if ((r == this->HistogramColor[0] &&
       g == this->HistogramColor[1] &&
       b == this->HistogramColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->HistogramColor[0] = r;
  this->HistogramColor[1] = g;
  this->HistogramColor[2] = b;

  this->Modified();

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogramColor(
  double r, double g, double b)
{
  if ((r == this->SecondaryHistogramColor[0] &&
       g == this->SecondaryHistogramColor[1] &&
       b == this->SecondaryHistogramColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->SecondaryHistogramColor[0] = r;
  this->SecondaryHistogramColor[1] = g;
  this->SecondaryHistogramColor[2] = b;

  this->Modified();

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterCursorColor(
  double r, double g, double b)
{
  if ((r == this->ParameterCursorColor[0] &&
       g == this->ParameterCursorColor[1] &&
       b == this->ParameterCursorColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->ParameterCursorColor[0] = r;
  this->ParameterCursorColor[1] = g;
  this->ParameterCursorColor[2] = b;

  this->Modified();

  this->RedrawParameterCursor();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointColor(
  double r, double g, double b)
{
  if ((r == this->PointColor[0] &&
       g == this->PointColor[1] &&
       b == this->PointColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->PointColor[0] = r;
  this->PointColor[1] = g;
  this->PointColor[2] = b;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointColor(
  double r, double g, double b)
{
  if ((r == this->SelectedPointColor[0] &&
       g == this->SelectedPointColor[1] &&
       b == this->SelectedPointColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->SelectedPointColor[0] = r;
  this->SelectedPointColor[1] = g;
  this->SelectedPointColor[2] = b;

  this->Modified();

  this->RedrawPoint(this->SelectedPoint);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointTextColor(
  double r, double g, double b)
{
  if ((r == this->PointTextColor[0] &&
       g == this->PointTextColor[1] &&
       b == this->PointTextColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->PointTextColor[0] = r;
  this->PointTextColor[1] = g;
  this->PointTextColor[2] = b;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointTextColor(
  double r, double g, double b)
{
  if ((r == this->SelectedPointTextColor[0] &&
       g == this->SelectedPointTextColor[1] &&
       b == this->SelectedPointTextColor[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->SelectedPointTextColor[0] = r;
  this->SelectedPointTextColor[1] = g;
  this->SelectedPointTextColor[2] = b;

  this->Modified();

  this->RedrawPoint(this->SelectedPoint);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetComputePointColorFromValue(int arg)
{
  if (this->ComputePointColorFromValue == arg)
    {
    return;
    }

  this->ComputePointColorFromValue = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetComputeHistogramColorFromValue(
  int arg)
{
  if (this->ComputeHistogramColorFromValue == arg)
    {
    return;
    }

  this->ComputeHistogramColorFromValue = arg;

  this->Modified();

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramStyle(
  int arg)
{
  if (this->HistogramStyle == arg)
    {
    return;
    }

  this->HistogramStyle = arg;

  this->Modified();

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogramStyle(
  int arg)
{
  if (this->SecondaryHistogramStyle == arg)
    {
    return;
    }

  this->SecondaryHistogramStyle = arg;

  this->Modified();

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowPointIndex(int arg)
{
  if (this->ShowPointIndex == arg)
    {
    return;
    }

  this->ShowPointIndex = arg;

  this->Modified();

  this->RedrawFunction();

  // Since the point label can show the point index, update it too

  if (this->HasSelection())
    {
    this->UpdatePointEntries(this->SelectedPoint);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowPointGuideline(int arg)
{
  if (this->ShowPointGuideline == arg)
    {
    return;
    }

  this->ShowPointGuideline = arg;

  this->Modified();

  // Remove the polyline in the canvas. RedrawFunction only changes items
  // coordinates if they already exist. To make sure the line is hidden,
  // we have to remove the item.

  if (!this->ShowPointGuideline)
    {
    this->CanvasRemoveTag(VTK_KW_PVFE_POINT_GUIDELINE_TAG);
    }

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointGuidelineStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::LineStyleSolid)
    {
    arg = vtkKWParameterValueFunctionEditor::LineStyleSolid;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::LineStyleDash)
    {
    arg = vtkKWParameterValueFunctionEditor::LineStyleDash;
    }

  if (this->PointGuidelineStyle == arg)
    {
    return;
    }

  this->PointGuidelineStyle = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowSelectedPointIndex(int arg)
{
  if (this->ShowSelectedPointIndex == arg)
    {
    return;
    }

  this->ShowSelectedPointIndex = arg;

  this->Modified();

  this->RedrawPoint(this->SelectedPoint);

  // Since the point label can show the point index, update it too

  if (this->HasSelection())
    {
    this->UpdatePointEntries(this->SelectedPoint);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetShowHistogramLogModeOptionMenu(int arg)
{
  if (this->ShowHistogramLogModeOptionMenu == arg)
    {
    return;
    }

  this->ShowHistogramLogModeOptionMenu = arg;

  // Make sure that if the button has to be shown, we create it on the fly if
  // needed

  if (this->ShowHistogramLogModeOptionMenu && this->IsCreated())
    {
    this->CreateHistogramLogModeOptionMenu(this->GetApplication());
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeCommand(const char *command)
{
  if (command && *command && !this->DisableCommands)
    {
    this->Script("eval %s", command);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointCommand(
  const char *command, int id, const char *extra)
{
  if (command && *command && !this->DisableCommands && 
      this->HasFunction() && id >= 0 && id < this->GetFunctionSize())
    {
    this->Script("eval %s %d %s", command, id, (extra ? extra : ""));
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointAddedCommand(int id)
{
  this->InvokePointCommand(this->PointAddedCommand, id);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::PointAddedEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointMovingCommand(int id)
{
  this->InvokePointCommand(this->PointMovingCommand, id);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::PointMovingEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointMovedCommand(int id)
{
  this->InvokePointCommand(this->PointMovedCommand, id);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::PointMovedEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointRemovedCommand(
  int id, double parameter)
{
  ostrstream param_str;
  param_str << parameter << ends;
  this->InvokePointCommand(this->PointRemovedCommand, id, param_str.str());
  param_str.rdbuf()->freeze(0);

  double dargs[2];
  dargs[0] = id;
  dargs[1] = parameter;

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::PointRemovedEvent, dargs);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeSelectionChangedCommand()
{
  this->InvokeCommand(this->SelectionChangedCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::SelectionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeFunctionChangedCommand()
{
  this->InvokeCommand(this->FunctionChangedCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::FunctionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeFunctionChangingCommand()
{
  this->InvokeCommand(this->FunctionChangingCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::FunctionChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeVisibleRangeChangedCommand()
{
  this->InvokeCommand(this->VisibleRangeChangedCommand);

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleRangeChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeVisibleRangeChangingCommand()
{
  this->InvokeCommand(this->VisibleRangeChangingCommand);

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleRangeChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeHistogramLogModeChangedCommand()
{
  this->InvokeCommand(this->HistogramLogModeChangedCommand);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramLogModeChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->HistogramLogModeChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointAddedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointAddedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMovingCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointMovingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMovedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointMovedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointRemovedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointRemovedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectionChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->FunctionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionChangingCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->FunctionChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleRangeChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VisibleRangeChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleRangeChangingCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VisibleRangeChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Canvas)
    {
    this->Canvas->SetEnabled(this->Enabled);
    }

  if (this->ParameterRange)
    {
    this->ParameterRange->SetEnabled(this->Enabled);
    }

  if (this->ValueRange)
    {
    this->ValueRange->SetEnabled(this->Enabled);
    }

  if (this->TopLeftContainer)
    {
    this->TopLeftContainer->SetEnabled(this->Enabled);
    }

  if (this->TopLeftFrame)
    {
    this->TopLeftFrame->SetEnabled(this->Enabled);
    }

  if (this->UserFrame)
    {
    this->UserFrame->SetEnabled(this->Enabled);
    }

  if (this->TopRightFrame)
    {
    this->TopRightFrame->SetEnabled(this->Enabled);
    }

  if (this->RangeLabel)
    {
    this->RangeLabel->SetEnabled(this->Enabled);
    }

  if (this->ParameterEntry)
    {
    this->ParameterEntry->SetEnabled(this->Enabled);
    }

  if (this->HistogramLogModeOptionMenu)
    {
    this->HistogramLogModeOptionMenu->SetEnabled(this->Enabled);
    }

  if (this->Enabled)
    {
    this->Bind();
    }
  else
    {
    this->UnBind();
    }
}

// ---------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetBalloonHelpString(
  const char *string)
{
  this->Superclass::SetBalloonHelpString(string);

  if (this->Canvas)
    {
    this->Canvas->SetBalloonHelpString(string);
    }

  if (this->ParameterRange)
    {
    this->ParameterRange->SetBalloonHelpString(string);
    }

  if (this->ValueRange)
    {
    this->ValueRange->SetBalloonHelpString(string);
    }

  if (this->RangeLabel)
    {
    this->RangeLabel->SetBalloonHelpString(string);
    }

  if (this->ParameterEntry)
    {
    this->ParameterEntry->SetBalloonHelpString(string);
    }
}

// ---------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetBalloonHelpJustification(int j)
{
  this->Superclass::SetBalloonHelpJustification(j);

  if (this->Canvas)
    {
    this->Canvas->SetBalloonHelpJustification(j);
    }

  if (this->ParameterRange)
    {
    this->ParameterRange->SetBalloonHelpJustification(j);
    }

  if (this->ValueRange)
    {
    this->ValueRange->SetBalloonHelpJustification(j);
    }

  if (this->RangeLabel)
    {
    this->RangeLabel->SetBalloonHelpJustification(j);
    }

  if (this->ParameterEntry)
    {
    this->ParameterEntry->SetBalloonHelpJustification(j);
    }
}

// ---------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogram(vtkKWHistogram *arg)
{
  if (this->Histogram == arg)
    {
    return;
    }

  if (this->Histogram)
    {
    this->Histogram->UnRegister(this);
    }
    
  this->Histogram = arg;
  
  if (this->Histogram)
    {
    this->Histogram->Register(this);
    }
  
  this->Modified();
  
  this->LastHistogramBuildTime = 0;

  this->RedrawHistogram();
  if (this->ComputeValueTicksFromHistogram)
    {
    this->RedrawRangeTicks();
    }
}

// ---------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogram(
  vtkKWHistogram *arg)
{
  if (this->SecondaryHistogram == arg)
    {
    return;
    }

  if (this->SecondaryHistogram)
    {
    this->SecondaryHistogram->UnRegister(this);
    }
    
  this->SecondaryHistogram = arg;
  
  if (this->SecondaryHistogram)
    {
    this->SecondaryHistogram->Register(this);
    }
  
  this->Modified();
  
  this->LastHistogramBuildTime = 0;

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetCanvasItemCenter(int item_id, 
                                                            int &x, int &y)
{
  if (!this->IsCreated())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  const char *type = this->Script("%s type %d", canv, item_id);
  if (!type || !*type)
    {
    return;
    }

  if (!strcmp(type, "oval"))
    {
    double c_x1, c_y1, c_x2, c_y2;
    if (sscanf(this->Script("%s coords %d", canv, item_id), 
               "%lf %lf %lf %lf", 
               &c_x1, &c_y1, &c_x2, &c_y2) != 4)
      {
      return;
      }
    x = vtkMath::Round((c_x1 + c_x2) * 0.5);
    y = vtkMath::Round((c_y1 + c_y2) * 0.5);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetCanvasScalingFactors(
  double factors[2])
{
  int margin_left, margin_right, margin_top, margin_bottom;
  this->GetCanvasMargin(
    &margin_left, &margin_right, &margin_top, &margin_bottom);

  double *p_v_range = this->GetVisibleParameterRange();
  if (p_v_range[1] != p_v_range[0])
    {
    factors[0] = (double)(this->CanvasWidth - 1 - margin_left - margin_right)
      / (p_v_range[1] - p_v_range[0]);
    }
  else
    {
    factors[0] = 0.0;
    }

  double *v_v_range = this->GetVisibleValueRange();
  if (v_v_range[1] != v_v_range[0])
    {
    factors[1] = (double)(this->CanvasHeight - 1 - margin_top - margin_bottom)
      / (v_v_range[1] - v_v_range[0]);
    }
  else
    {
    factors[1] = 0.0;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetCanvasMargin(
  int *margin_left, 
  int *margin_right, 
  int *margin_top, 
  int *margin_bottom)
{
  // Compute the point margin

  int max_point_radius = this->PointRadius;
  if (this->SelectedPointRadius > 1.0)
    {
    max_point_radius = 
      (int)ceil((double)max_point_radius * this->SelectedPointRadius);
    }

  int point_margin = (int)floor(
    (double)max_point_radius + (double)this->PointOutlineWidth * 0.5);

  if (margin_left)
    {
    *margin_left = (this->PointMarginToCanvas & 
                    vtkKWParameterValueFunctionEditor::PointMarginLeftSide 
                    ? point_margin : 0);
    }

  if (margin_right)
    {
    *margin_right = (this->PointMarginToCanvas & 
                     vtkKWParameterValueFunctionEditor::PointMarginRightSide 
                     ? point_margin : 0);
    }

  if (margin_top)
    {
    *margin_top = (this->PointMarginToCanvas & 
                   vtkKWParameterValueFunctionEditor::PointMarginTopSide 
                   ? point_margin : 0);
    }

  if (margin_bottom)
    {
    *margin_bottom = (this->PointMarginToCanvas & 
                      vtkKWParameterValueFunctionEditor::PointMarginBottomSide 
                      ? point_margin : 0);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetCanvasScrollRegion(
  double *x, 
  double *y, 
  double *x2, 
  double *y2)
{
  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  // Compute the starting point for the scrollregion.
  // (note that the y axis is inverted)

  int margin_left, margin_top;
  this->GetCanvasMargin(&margin_left, NULL, &margin_top, NULL);

  double *p_v_range = this->GetVisibleParameterRange();
  double c_x = factors[0] * p_v_range[0] - (double)margin_left;
  if (x)
    {
    *x = c_x;
    }

  double *v_w_range = this->GetWholeValueRange();
  double *v_v_range = this->GetVisibleValueRange();
  double c_y = factors[1] * (v_w_range[1] - v_v_range[1]) - (double)margin_top;
  if (y)
    {
    *y = c_y;
    }

  if (x2)
    {
    *x2 = (c_x + (double)(this->CanvasWidth - 0));
    }

  if (y2)
    {
    *y2 = (c_y + (double)(this->CanvasHeight - 0));
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetCanvasHorizontalSlidingBounds(
  double p_v_range_ext[2], int bounds[2], int margins[2])
{
  if (!p_v_range_ext)
    {
    return;
    }
  
  double *p_v_range = this->GetVisibleParameterRange();
  double *p_w_range = this->GetWholeParameterRange();

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  int margin_left, margin_right;
  this->GetCanvasMargin(&margin_left, &margin_right, NULL, NULL);

  double p_v_range_margin_left = (double)margin_left;
  if (factors[0])
    {
    p_v_range_margin_left = (p_v_range_margin_left / factors[0]);
    }
  double p_v_range_margin_right = (double)margin_right;
  if (factors[0])
    {
    p_v_range_margin_right = (p_v_range_margin_right / factors[0]);
    }

  p_v_range_ext[0] = (p_v_range[0] - p_v_range_margin_left);
  if (p_v_range_ext[0] < p_w_range[0])
    {
    p_v_range_ext[0] = p_w_range[0];
    }
  p_v_range_ext[1] = (p_v_range[1] + p_v_range_margin_right);
  if (p_v_range_ext[1] > p_w_range[1])
    {
    p_v_range_ext[1] = p_w_range[1];
    }
  
  if (bounds)
    {
    bounds[0] = vtkMath::Round(p_v_range_ext[0] * factors[0]);
    bounds[1] = vtkMath::Round(p_v_range_ext[1] * factors[0]);
    }

  if (margins)
    {
    margins[0] = margin_left - 
      vtkMath::Round((p_v_range[0] - p_v_range_ext[0])  * factors[0]);
    margins[1] = margin_right - 
      vtkMath::Round((p_v_range_ext[1] - p_v_range[1])  * factors[0]);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Redraw()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() || 
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  const char *v_t_canv = NULL;
  if (this->ShowValueTicks)
    {
    v_t_canv = this->ValueTicksCanvas->GetWidgetName();
    }

  const char *p_t_canv = NULL;
  if (this->ShowParameterTicks)
    {
    p_t_canv = this->ParameterTicksCanvas->GetWidgetName();
    }

  ostrstream tk_cmd;

  // Get the new canvas size

  int old_c_width = atoi(this->Script("%s cget -width", canv));
  int old_c_height = atoi(this->Script("%s cget -height", canv));

  if (this->ExpandCanvasWidth)
    {
    this->CanvasWidth = atoi(this->Script("winfo width %s", canv));
    if (this->CanvasWidth < VTK_KW_PVFE_CANVAS_WIDTH_MIN)
      {
      this->CanvasWidth = VTK_KW_PVFE_CANVAS_WIDTH_MIN;
      }
    }

  tk_cmd << canv << " configure "
         << " -width " << this->CanvasWidth 
         << " -height " << this->CanvasHeight
         << endl;

  if (v_t_canv)
    {
    tk_cmd << v_t_canv << " configure "
           << " -height " << this->CanvasHeight << endl;
    }

  if (p_t_canv)
    {
    tk_cmd << p_t_canv << " configure "
           << " -width " << this->CanvasWidth << endl;
    }

  // In that visible area, we must fit the visible parameter in the
  // width dimension, and the visible value range in the height dimension.
  // Get the corresponding scaling factors.

  double c_x, c_y, c_x2, c_y2;
  this->GetCanvasScrollRegion(&c_x, &c_y, &c_x2, &c_y2);

  tk_cmd << canv << " configure "
         << " -scrollregion {" 
         << c_x << " " << c_y << " " << c_x2 << " " << c_y2 << "}" << endl;

  if (v_t_canv)
    {
    tk_cmd << v_t_canv << " configure -width " << this->ValueTicksCanvasWidth
           << " -scrollregion {" 
           << 0 << " " << c_y << " " 
           << this->ValueTicksCanvasWidth << " " << c_y2 << "}" 
           << endl;
    }

  if (p_t_canv)
    {
    tk_cmd << p_t_canv << " configure "
           << " -scrollregion {" 
           << c_x << " " << 0 << " " 
           << c_x2 << " " << VTK_KW_PVFE_TICKS_PARAMETER_CANVAS_HEIGHT << "}" 
           << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // If the canvas has been resized,
  // or if the visible range has changed (i.e. if the relative size of the
  // visible range compared to the whole range has changed significantly)
  // or if the function has changed, etc.
 
  vtkKWParameterValueFunctionEditor::Ranges ranges;
  ranges.GetRangesFrom(this);

  if (old_c_width != this->CanvasWidth || 
      old_c_height != this->CanvasHeight ||
      ranges.NeedResizeComparedTo(&this->LastRanges))
    {
    this->RedrawSizeDependentElements();
    }

  if (ranges.NeedPanOnlyComparedTo(&this->LastRanges))
    {
    this->RedrawPanOnlyDependentElements();
    }

  if (!this->HasFunction() ||
      this->GetFunctionMTime() > this->LastRedrawFunctionTime)
    {
    this->RedrawFunctionDependentElements();
    }

  this->LastRanges.GetRangesFrom(this);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawSizeDependentElements()
{
  this->RedrawRangeFrame();
  this->RedrawHistogram();
  this->RedrawRangeTicks();
  this->RedrawParameterCursor();
  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawPanOnlyDependentElements()
{
  this->RedrawHistogram();
  this->RedrawRangeTicks();

  if (this->PointPositionInValueRange != 
      vtkKWParameterValueFunctionEditor::PointPositionAtValue)
    {
    this->RedrawFunction();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawFunctionDependentElements()
{
  this->RedrawFunction();
  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawRangeFrame()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  ostrstream tk_cmd;

  // Create the frames if not created already
  // We use two frames, one frame is a black outline, the other is the
  // solid background. Then we can insert objects between those two
  // frames (histogram for example)

  int has_tag = this->CanvasHasTag(VTK_KW_PVFE_FRAME_FG_TAG);
  if (!has_tag)
    {
    if (this->ShowCanvasOutline)
      {
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleLeftSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_l " << VTK_KW_PVFE_FRAME_FG_TAG << "}\n";
        }
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleRightSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_r " << VTK_KW_PVFE_FRAME_FG_TAG << "}\n";
        }
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleTopSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_t " << VTK_KW_PVFE_FRAME_FG_TAG << "}\n";
        }
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleBottomSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_b " << VTK_KW_PVFE_FRAME_FG_TAG << "}\n";
        }
      tk_cmd << canv << " lower " << VTK_KW_PVFE_FRAME_FG_TAG
             << " {" << VTK_KW_PVFE_FUNCTION_TAG << "}" << endl;
      }
    }
  else 
    {
    if (!this->ShowCanvasOutline)
      {
      tk_cmd << canv << " delete " << VTK_KW_PVFE_FRAME_FG_TAG << endl;
      }
    }

  // The background

  has_tag = this->CanvasHasTag(VTK_KW_PVFE_FRAME_BG_TAG);
  if (!has_tag)
    {
    if (this->ShowCanvasBackground)
      {
      tk_cmd << canv << " create rectangle 0 0 0 0 "
             << " -tags {" << VTK_KW_PVFE_FRAME_BG_TAG << "}" << endl;
      tk_cmd << canv << " lower " << VTK_KW_PVFE_FRAME_BG_TAG 
             << " all" << endl;
      }
    }
  else 
    {
    if (!this->ShowCanvasBackground)
      {
      tk_cmd << canv << " delete " << VTK_KW_PVFE_FRAME_BG_TAG << endl;
      }
    }

  // Update the frame position to match the whole range
  
  double p_w_range[2];
  this->GetWholeParameterRange(p_w_range);

  // If there are points outside the whole range, expand the frame position
  // (this can happen in WindowLevelMode, see vtkKWPiecewiseFunctionEditor)

  if (this->HasFunction())
    {
    double param;
    if (this->GetFunctionPointParameter(0, &param))
      {
      if (param < p_w_range[0])
        {
        p_w_range[0] = param;
        }
      }
    if (this->GetFunctionPointParameter(this->GetFunctionSize() - 1, &param))
      {
      if (param > p_w_range[1])
        {
        p_w_range[1] = param;
        }
      }
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  double v_w_range[2];
  this->GetWholeValueRange(v_w_range);

  // Update coordinates and colors

  if (this->ShowCanvasOutline)
    {
    double c1_x = p_w_range[0] * factors[0];
    double c1_y = v_w_range[0] * factors[1];
    double c2_x = p_w_range[1] * factors[0];
    double c2_y = v_w_range[1] * factors[1];

    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleLeftSide)
      {
      tk_cmd << canv << " coords framefg_l " 
             << c1_x << " " << c2_y << " " 
             << c1_x << " " << c1_y - LSTRANGE << endl;
      }
    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleRightSide)
      {
      tk_cmd << canv << " coords framefg_r " 
             << c2_x << " " << c2_y << " " 
             << c2_x << " " << c1_y - LSTRANGE << endl;
      }
    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleTopSide)
      {
      tk_cmd << canv << " coords framefg_t " 
             << c2_x << " " << c1_y << " " 
             << c1_x - LSTRANGE << " " << c1_y << endl;
      }
    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleBottomSide)
      {
      tk_cmd << canv << " coords framefg_b " 
             << c2_x << " " << c2_y << " " 
             << c1_x - LSTRANGE << " " << c2_y << endl;
      }
    }

  if (this->ShowCanvasBackground)
    {
    tk_cmd << canv << " coords " << VTK_KW_PVFE_FRAME_BG_TAG 
           << " " << p_w_range[0] * factors[0]
           << " " << v_w_range[0] * factors[1]
           << " " << p_w_range[1] * factors[0]
           << " " << v_w_range[1] * factors[1] 
           << endl;
    char color[10];
    sprintf(color, "#%02x%02x%02x", 
            (int)(this->FrameBackgroundColor[0] * 255.0),
            (int)(this->FrameBackgroundColor[1] * 255.0),
            (int)(this->FrameBackgroundColor[2] * 255.0));
    tk_cmd << canv << " itemconfigure " << VTK_KW_PVFE_FRAME_BG_TAG 
           << " -outline " << color << " -fill " << color << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawRangeTicks()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  const char *v_t_canv = NULL;
  if (this->ShowValueTicks)
    {
    v_t_canv = this->ValueTicksCanvas->GetWidgetName();
    }

  const char *p_t_canv = NULL;
  if (this->ShowParameterTicks)
    {
    p_t_canv = this->ParameterTicksCanvas->GetWidgetName();
    }

  ostrstream tk_cmd;

  // Create the ticks if not created already

  int has_p_tag = this->CanvasHasTag(VTK_KW_PVFE_PARAMETER_TICKS_TAG);
  if (!has_p_tag)
    {
    if (this->ShowParameterTicks)
      {
      for (int i = 0; i < this->NumberOfParameterTicks; i++)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {p_tick_t" << i << " " 
               << VTK_KW_PVFE_PARAMETER_TICKS_TAG << "}" << endl;
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {p_tick_b" << i << " " 
               << VTK_KW_PVFE_PARAMETER_TICKS_TAG << "}" << endl;
        tk_cmd << p_t_canv << " create text 0 0 -text {} -anchor n " 
               << "-font {{fixed} " << VTK_KW_PVFE_TICKS_TEXT_SIZE << "} "
               << "-tags {p_tick_b_t" << i << " " 
               << VTK_KW_PVFE_PARAMETER_TICKS_TAG << "}" << endl;
        }
      }
    }
  else 
    {
    if (!this->ShowParameterTicks)
      {
      tk_cmd << canv << " delete " 
             << VTK_KW_PVFE_PARAMETER_TICKS_TAG << endl;
      tk_cmd << p_t_canv << " delete " 
             << VTK_KW_PVFE_PARAMETER_TICKS_TAG << endl;
      }
    }

  int has_v_tag = this->CanvasHasTag(VTK_KW_PVFE_VALUE_TICKS_TAG);
  if (!has_v_tag)
    {
    if (this->ShowValueTicks)
      {
      for (int i = 0; i < this->NumberOfValueTicks; i++)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {v_tick_l" << i << " " 
               << VTK_KW_PVFE_VALUE_TICKS_TAG << "}" << endl;
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {v_tick_r" << i << " " 
               << VTK_KW_PVFE_VALUE_TICKS_TAG << "}" << endl;
        tk_cmd << v_t_canv << " create text 0 0 -text {} -anchor e " 
               << "-font {{fixed} " << VTK_KW_PVFE_TICKS_TEXT_SIZE << "} "
               << "-tags {v_tick_l_t" << i << " " 
               << VTK_KW_PVFE_VALUE_TICKS_TAG << "}" << endl;
        }
      }
    }
  else 
    {
    if (!this->ShowValueTicks)
      {
      tk_cmd << canv << " delete " 
             << VTK_KW_PVFE_VALUE_TICKS_TAG << endl;
      tk_cmd << v_t_canv << " delete " 
             << VTK_KW_PVFE_VALUE_TICKS_TAG << endl;
      }
    }

  // Update coordinates and colors

  if (this->ShowParameterTicks || this->ShowValueTicks)
    {
    double factors[2] = {0.0, 0.0};
    this->GetCanvasScalingFactors(factors);

    double *p_v_range = this->GetVisibleParameterRange();
    double *v_v_range = this->GetVisibleValueRange();
    double *v_w_range = this->GetWholeValueRange();

    char buffer[100];

    if (this->ShowParameterTicks)
      {
      double y_t = (v_w_range[1] - v_v_range[1]) * factors[1];
      double y_b = (v_w_range[1] - v_v_range[0]) * factors[1];

      double p_v_step = (p_v_range[1] - p_v_range[0]) / 
        (double)(this->NumberOfParameterTicks + 1);
      double p_v_pos = p_v_range[0] + p_v_step;

      for (int i = 0; i < this->NumberOfParameterTicks; i++)
        {
        double x = p_v_pos * factors[0];
        tk_cmd << canv << " coords p_tick_t" <<  i << " " 
               << x << " " << y_t << " " << x << " " << y_t + this->TicksLength
               << endl;
        tk_cmd << canv << " coords p_tick_b" <<  i << " " 
               << x << " " << y_b << " " << x << " " << y_b - this->TicksLength
               << endl;
        tk_cmd << p_t_canv << " coords p_tick_b_t" <<  i << " " 
               << x << " " << -1 << endl;
        if (this->ParameterTicksFormat)
          {
          sprintf(buffer, this->ParameterTicksFormat, p_v_pos);
          tk_cmd << p_t_canv << " itemconfigure p_tick_b_t" <<  i << " " 
                 << " -text {" << buffer << "}" << endl;
          }
        p_v_pos += p_v_step;
        }
      }

    if (this->ShowValueTicks)
      {
      double x_l = (p_v_range[0] * factors[0]);
      double x_r = (p_v_range[1] * factors[0]);
      
      double v_v_delta = (v_v_range[1] - v_v_range[0]);
      double v_v_step = v_v_delta / (double)(this->NumberOfValueTicks + 1);
      double v_v_pos = v_v_range[0] + v_v_step;

      double val;
      
      for (int i = 0; i < this->NumberOfValueTicks; i++)
        {
        double y = (v_w_range[1] - v_v_pos) * factors[1];
        tk_cmd << canv << " coords v_tick_l" <<  i << " " 
               << x_l << " " << y << " " << x_l + this->TicksLength << " " << y
               << endl;
        tk_cmd << canv << " coords v_tick_r" <<  i << " " 
               << x_r << " " << y << " " << x_r - this->TicksLength << " " << y
               << endl;
        tk_cmd << v_t_canv << " coords v_tick_l_t" <<  i << " " 
               << this->ValueTicksCanvasWidth - 1 << " " << y
               << endl;

        if (this->ComputeValueTicksFromHistogram &&
            this->Histogram &&
            this->HistogramImageDescriptor)
          {
          double norm = ((v_v_pos - v_v_range[0]) * v_v_delta);
          if (this->Histogram->GetLogMode())
            {
            val = exp(norm * log(
                        this->HistogramImageDescriptor->LastMaximumOccurence));
            }
          else
            {
            val = norm * this->HistogramImageDescriptor->LastMaximumOccurence;
            }
          }
        else
          {
          val = v_v_pos;
          }
        
        if (this->ValueTicksFormat)
          {
          sprintf(buffer, this->ValueTicksFormat, val);
          tk_cmd << v_t_canv << " itemconfigure v_tick_l_t" <<  i << " " 
                 << " -text {" << buffer << "}" << endl;
          }

        v_v_pos += v_v_step;
        }
      }
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawParameterCursor()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  ostrstream tk_cmd;

  // Create the cursor if not created already

  int has_tag = this->CanvasHasTag(VTK_KW_PVFE_PARAMETER_CURSOR_TAG);
  if (!has_tag)
    {
    if (this->ShowParameterCursor)
      {
      tk_cmd << canv << " create line 0 0 0 0 "
             << " -tags {" << VTK_KW_PVFE_PARAMETER_CURSOR_TAG << "}" << endl;
      tk_cmd << canv << " lower " << VTK_KW_PVFE_PARAMETER_CURSOR_TAG
             << " {" << VTK_KW_PVFE_FUNCTION_TAG << "}" << endl;
      }
    }
  else 
    {
    if (!this->ShowParameterCursor)
      {
      tk_cmd << canv << " delete " << VTK_KW_PVFE_PARAMETER_CURSOR_TAG<< endl;
      }
    }

  // Update the cursor position and style
  
  if (this->ShowParameterCursor)
    {
    double v_v_range[2];
    this->GetWholeValueRange(v_v_range);

    double factors[2] = {0.0, 0.0};
    this->GetCanvasScalingFactors(factors);

    tk_cmd << canv << " coords " << VTK_KW_PVFE_PARAMETER_CURSOR_TAG
           << " " << this->ParameterCursorPosition * factors[0]
           << " " << v_v_range[0] * factors[1]
           << " " << this->ParameterCursorPosition * factors[0]
           << " " << v_v_range[1] * factors[1] + LSTRANGE
           << endl;

    char color[10];
    sprintf(color, "#%02x%02x%02x", 
            (int)(this->ParameterCursorColor[0] * 255.0),
            (int)(this->ParameterCursorColor[1] * 255.0),
            (int)(this->ParameterCursorColor[2] * 255.0));
    
    tk_cmd << canv << " itemconfigure " << VTK_KW_PVFE_PARAMETER_CURSOR_TAG
           << " -fill " << color << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawPoint(int id, 
                                                    ostrstream *tk_cmd)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || 
      id < 0 || 
      id >= this->GetFunctionSize() ||
      this->DisableRedraw)
    {
    return;
    }

  // If there is no stream, then it means we want to execute that command
  // right now (so create a stream)

  int stream_was_created = 0;
  if (!tk_cmd)
    {
    tk_cmd = new ostrstream;
    stream_was_created = 1;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // Get the point coords, color, radius (different size if point is selected)

  int x, y;
  this->GetFunctionPointCanvasCoordinates(id, x, y);

  int r = this->PointRadius;
  if (id == this->SelectedPoint)
    {
    r = (int)ceil((double)r * this->SelectedPointRadius);
    }

  // Create the text item (index at each point)

  if (!this->CanvasHasTag("t", &id))
    {
    *tk_cmd << canv << " create text 0 0 -text {} " 
            << "-tags {t" << id 
            << " " << VTK_KW_PVFE_TEXT_TAG
            << " " << VTK_KW_PVFE_FUNCTION_TAG << "}\n";
    }

  // Get the style

  // Points are reused. Since each point is of a specific type, it is OK
  // as long as the point styles are not mixed. If they are (i.e., a special
  // style for the first or last point for example), we have to check
  // for the point type. If it is not the right type, the point can not be
  // reused as the coordinates spec would not match. In that case, delete
  // the point right now.

  int func_size = this->GetFunctionSize();

  int style = this->PointStyle;
  int must_check_for_type = 0;

  if (this->FirstPointStyle != 
      vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    must_check_for_type = 1;
    if (id == 0)
      {
      style = this->FirstPointStyle;
      }
    }
  if (this->LastPointStyle != 
      vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    must_check_for_type = 1;
    if (id == func_size - 1)
      {
      style = this->LastPointStyle;
      }
    }

  int point_exists = this->CanvasHasTag("p", &id);
  if (point_exists && must_check_for_type)
    {
    int must_delete_point = 0;
    switch (style)
      {
      case vtkKWParameterValueFunctionEditor::PointStyleDefault:
      case vtkKWParameterValueFunctionEditor::PointStyleDisc:
        must_delete_point = !this->CanvasCheckTagType("p", id, "oval");
        break;

      case vtkKWParameterValueFunctionEditor::PointStyleRectangle:
        must_delete_point = !this->CanvasCheckTagType("p", id, "rectange");
        break;

      case vtkKWParameterValueFunctionEditor::PointStyleCursorDown:
      case vtkKWParameterValueFunctionEditor::PointStyleCursorUp:
      case vtkKWParameterValueFunctionEditor::PointStyleCursorLeft:
      case vtkKWParameterValueFunctionEditor::PointStyleCursorRight:
        must_delete_point = !this->CanvasCheckTagType("p", id, "polygon");
        break;
      }
    if (must_delete_point)
      {
      *tk_cmd << canv << " delete p" << id << endl;
      point_exists = 0;
      }
    }

  // Create the point item

  if (!point_exists)
    {
    switch (style)
      {
      case vtkKWParameterValueFunctionEditor::PointStyleDefault:
      case vtkKWParameterValueFunctionEditor::PointStyleDisc:
        *tk_cmd << canv << " create oval";
        break;

      case vtkKWParameterValueFunctionEditor::PointStyleRectangle:
        *tk_cmd << canv << " create rectangle";
        break;

      case vtkKWParameterValueFunctionEditor::PointStyleCursorDown:
      case vtkKWParameterValueFunctionEditor::PointStyleCursorUp:
      case vtkKWParameterValueFunctionEditor::PointStyleCursorLeft:
      case vtkKWParameterValueFunctionEditor::PointStyleCursorRight:
        *tk_cmd << canv << " create polygon 0 0 0 0 0 0";
        break;
      }
    *tk_cmd << " 0 0 0 0 -tags {p" << id 
            << " " << VTK_KW_PVFE_POINT_TAG 
            << " " << VTK_KW_PVFE_FUNCTION_TAG << "}\n";
    *tk_cmd << canv << " lower p" << id << " t" << id << endl;
    }

  // Create the point guideline

  if (this->ShowPointGuideline)
    {
    if (!this->CanvasHasTag("g", &id))
      {
      *tk_cmd << canv << " create line 0 0 0 0 -fill black -width 1 " 
              << " -tags {g" << id << " " << VTK_KW_PVFE_POINT_GUIDELINE_TAG 
              << " " << VTK_KW_PVFE_FUNCTION_TAG
              << "}" << endl;
      *tk_cmd << canv << " lower g" << id << " p" << id << endl;
      }
    }
  
  // Create the line between a point and its predecessor

  if (this->ShowFunctionLine)
    {
    if (id > 0)
      {
      if (!this->CanvasHasTag("l", &id))
        {
        *tk_cmd << canv << " create line 0 0 0 0 -fill black -width 2 " 
                << " -tags {l" << id 
                << " " << VTK_KW_PVFE_LINE_TAG
                << " " << VTK_KW_PVFE_FUNCTION_TAG << "}\n";
        *tk_cmd << canv << " lower l" << id << " p" << id - 1 << endl;
        }
      }
    }
  
  // Update the point coordinates and style

  switch (style)
    {
    case vtkKWParameterValueFunctionEditor::PointStyleDefault:
    case vtkKWParameterValueFunctionEditor::PointStyleDisc:
      *tk_cmd << canv << " coords p" << id 
              << " " << x - r << " " << y - r 
              << " " << x + r << " " << y + r << endl;
      break;
      
    case vtkKWParameterValueFunctionEditor::PointStyleRectangle:
      *tk_cmd << canv << " coords p" << id 
              << " " << x - r << " " << y - r 
              << " " << x + r + LSTRANGE << " " << y + r + LSTRANGE << endl;
      break;
      
    case vtkKWParameterValueFunctionEditor::PointStyleCursorDown:
      *tk_cmd << canv << " coords p" << id 
              << " " << x - r << " " << y 
              << " " << x     << " " << y + r
              << " " << x + r << " " << y 
              << " " << x + r << " " << y - r + 1 
              << " " << x - r << " " << y - r + 1 
              << endl;
      break;

    case vtkKWParameterValueFunctionEditor::PointStyleCursorUp:
      *tk_cmd << canv << " coords p" << id 
              << " " << x - r << " " << y 
              << " " << x     << " " << y - r
              << " " << x + r << " " << y 
              << " " << x + r << " " << y + r - 1 
              << " " << x - r << " " << y + r - 1 
              << endl;
      break;

    case vtkKWParameterValueFunctionEditor::PointStyleCursorLeft:
      *tk_cmd << canv << " coords p" << id 
              << " " << x         << " " << y + r
              << " " << x - r     << " " << y
              << " " << x         << " " << y - r
              << " " << x + r - 1 << " " << y - r 
              << " " << x + r - 1 << " " << y + r
              << endl;
      break;

    case vtkKWParameterValueFunctionEditor::PointStyleCursorRight:
      *tk_cmd << canv << " coords p" << id 
              << " " << x         << " " << y + r
              << " " << x + r     << " " << y
              << " " << x         << " " << y - r
              << " " << x - r + 1 << " " << y - r 
              << " " << x - r + 1 << " " << y + r
              << endl;
      break;
    }

  *tk_cmd << canv << " itemconfigure p" << id 
          << " -width " << this->PointOutlineWidth << endl;

  // Update the text coordinates

  *tk_cmd << canv << " coords t" << id << " " << x << " " << y << endl;

  // Update the guideline coordinates and style

  if (this->ShowPointGuideline)
    {
    double factors[2] = {0.0, 0.0};
    this->GetCanvasScalingFactors(factors);
    double *v_w_range = this->GetWholeValueRange();
    int y1 = vtkMath::Round(v_w_range[0] * factors[1]);
    int y2 = vtkMath::Round(v_w_range[1] * factors[1]);
    *tk_cmd << canv << " coords g" << id << " "
            << x << " " << y1 << " " << x << " " << y2 << endl;
    *tk_cmd << canv << " itemconfigure g" << id;
    if (this->PointGuidelineStyle == 
        vtkKWParameterValueFunctionEditor::LineStyleDash)
      {
      *tk_cmd << " -dash {.}";
      }
    else
      {
      *tk_cmd << " -dash {}";
      }
    *tk_cmd << endl;
    }

  // Update the line coordinates and style

  if (this->ShowFunctionLine)
    {
    if (id > 0)
      {
      int prev_x, prev_y;
      this->GetFunctionPointCanvasCoordinates(id - 1, prev_x, prev_y);
      *tk_cmd << canv << " coords l" << id << " "
              << prev_x << " " << prev_y << " " << x << " " << y << endl;
      *tk_cmd << canv << " itemconfigure l" << id 
              << " -width " << this->FunctionLineWidth;
      if (this->FunctionLineStyle == 
          vtkKWParameterValueFunctionEditor::LineStyleDash)
        {
        *tk_cmd << " -dash {.}";
        }
      else
        {
        *tk_cmd << " -dash {}";
        }
      *tk_cmd << endl;
      }
    if (id < func_size - 1)
      {
      int next_x, next_y;
      this->GetFunctionPointCanvasCoordinates(id + 1, next_x, next_y);
      *tk_cmd << canv << " coords l" << id + 1 << " "
              << x << " " << y << " " << next_x << " " << next_y << endl;
      }
    }

  // Update the point color

  double rgb[3];
  char color[10];

  if (this->GetFunctionPointColorInCanvas(id, rgb))
    {
    sprintf(color, "#%02x%02x%02x", 
            (int)(rgb[0]*255.0), (int)(rgb[1]*255.0), (int)(rgb[2]*255.0));
    *tk_cmd << canv << " itemconfigure p" << id;
    if (style != vtkKWParameterValueFunctionEditor::PointStyleRectangle)
      {
      *tk_cmd << " -outline black -fill " << color << endl;
      }
    else
      {
      *tk_cmd << " -fill {} -outline " << color << endl;
      }
    }

  // Update the text color

  if (this->ShowPointIndex ||
      (this->ShowSelectedPointIndex && id == this->SelectedPoint))
    {
    if (this->GetFunctionPointTextColorInCanvas(id, rgb))
      {
      sprintf(color, "#%02x%02x%02x", 
              (int)(rgb[0]*255.0), (int)(rgb[1]*255.0), (int)(rgb[2]*255.0));
      *tk_cmd << canv << " itemconfigure t" << id
              << " -font {{fixed} " << 7 - (id > 8 ? 1 : 0) << "} -text " 
              << id + 1 << " -fill " << color << endl;
      }
    }
  else
    {
    *tk_cmd << canv << " itemconfigure t" << id << " -text {}" << endl;
    }

  // Execute the command, free the stream

  if (stream_was_created)
    {
    *tk_cmd << ends;
    this->Script(tk_cmd->str());
    tk_cmd->rdbuf()->freeze(0);
    delete tk_cmd;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawFunction()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If no function, or empty, remove everything

  if (!this->HasFunction() || !this->GetFunctionSize())
    {
    this->CanvasRemoveTag(VTK_KW_PVFE_FUNCTION_TAG);
    return;
    }

  // Are we going to create or delete points ?

  int c_nb_points = this->CanvasHasTag(VTK_KW_PVFE_POINT_TAG);
  int nb_points_changed = (c_nb_points != this->GetFunctionSize());

  // Try to save the selection before (eventually) creating new points

  int s_x = 0, s_y = 0;
  if (nb_points_changed && this->HasSelection())
    {
    int item_id = atoi(
      this->Script("lindex [%s find withtag %s] 0",
                   canv, VTK_KW_PVFE_SELECTED_TAG));
    this->GetCanvasItemCenter(item_id, s_x, s_y);
    }

  // Create the points 

  ostrstream tk_cmd;

  int i, nb_points = this->GetFunctionSize();
  for (i = 0; i < nb_points; i++)
    {
    this->RedrawPoint(i, &tk_cmd);
    }

  // Delete the extra points

  c_nb_points = this->CanvasHasTag(VTK_KW_PVFE_POINT_TAG);
  for (i = nb_points; i < c_nb_points; i++)
    {
    tk_cmd << canv << " delete p" << i << " l" << i << " t" << i << endl;
    }

  // Execute all of this

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  this->LastRedrawFunctionTime = this->GetFunctionMTime();

  // Try to restore the selection

  if (nb_points_changed && this->HasSelection())
    {
    int p_x = 0, p_y = 0;
    for (i = 0; i < nb_points; i++)
      {
      this->GetFunctionPointCanvasCoordinates(i, p_x, p_y);
      if (p_x == s_x && p_y == s_y)
        {
        this->SelectPoint(i);
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateHistogramImageDescriptor(
  vtkKWHistogram::ImageDescriptor *desc)
{
  // Provide additional margin around the histogram

  double p_v_range_ext[2];
  int bounds[2];
  this->GetCanvasHorizontalSlidingBounds(p_v_range_ext, bounds, NULL);

  int margin_top, margin_bottom;
  this->GetCanvasMargin(NULL, NULL, &margin_top, &margin_bottom);

  desc->SetRange(p_v_range_ext[0], p_v_range_ext[1]);
  desc->SetDimensions(
    bounds[1] - bounds[0] + 1, 
    this->CanvasHeight - margin_top - margin_bottom);

  desc->SetBackgroundColor(this->FrameBackgroundColor);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawHistogram()
{
  if (!this->IsCreated() || 
      !this->Canvas || 
      !this->Canvas->IsAlive() ||
      this->DisableRedraw)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  ostrstream img_name;
  img_name << canv << "." << VTK_KW_PVFE_HISTOGRAM_TAG << ends;

  int has_tag = this->CanvasHasTag(VTK_KW_PVFE_HISTOGRAM_TAG);

  // Create the image if not created already

  if ((this->Histogram || this->SecondaryHistogram) && !has_tag)
    {
    this->Script("image create photo %s -width 0 -height 0", img_name.str());
    }

  // Update the image if needed

  if (!this->HistogramImageDescriptor)
    {
    this->HistogramImageDescriptor = new vtkKWHistogram::ImageDescriptor; 
    }
  this->HistogramImageDescriptor->SetColor(this->HistogramColor);
  this->HistogramImageDescriptor->Style = this->HistogramStyle;
  this->HistogramImageDescriptor->DrawBackground = 1;
  this->UpdateHistogramImageDescriptor(this->HistogramImageDescriptor);

  if (!this->SecondaryHistogramImageDescriptor)
    {
    this->SecondaryHistogramImageDescriptor = 
      new vtkKWHistogram::ImageDescriptor;
    }

  this->SecondaryHistogramImageDescriptor->SetColor(
    this->SecondaryHistogramColor);
  this->SecondaryHistogramImageDescriptor->Style = 
    this->SecondaryHistogramStyle;
  this->SecondaryHistogramImageDescriptor->DrawBackground = 0;
  this->UpdateHistogramImageDescriptor(
    this->SecondaryHistogramImageDescriptor);

  double *p_v_range = this->GetVisibleParameterRange();
  if ((this->Histogram || this->SecondaryHistogram) && 
      p_v_range[0] != p_v_range[1])
    {
    unsigned long image_mtime = 0;
    vtkImageData *image = NULL;
    if (this->Histogram)
      {
      image = this->Histogram->GetImage(this->HistogramImageDescriptor);
      image_mtime = image->GetMTime();
      }

    unsigned long secondary_image_mtime = 0;
    vtkImageData *secondary_image = NULL;
    if (this->SecondaryHistogram)
      {
      if (image)
        {
        this->SecondaryHistogramImageDescriptor->DefaultMaximumOccurence = 
          this->HistogramImageDescriptor->LastMaximumOccurence;
        }
      secondary_image = this->SecondaryHistogram->GetImage(
        this->SecondaryHistogramImageDescriptor);
      secondary_image_mtime = secondary_image->GetMTime();
      }
    
    if (image_mtime > this->LastHistogramBuildTime ||
        secondary_image_mtime > this->LastHistogramBuildTime)
      {
      Tcl_Interp *interp = this->GetApplication()->GetMainInterp();
      if (image && secondary_image)
        {
        vtkImageBlend *blend = vtkImageBlend::New();
        blend->AddInput(image);
        blend->AddInput(secondary_image);
        blend->Update();
        vtkKWTkUtilities::UpdatePhoto(
          interp, img_name.str(), blend->GetOutput(), canv);
        blend->Delete();
        }
      else if (image)
        {
        vtkKWTkUtilities::UpdatePhoto(
          interp, img_name.str(), image,canv);
        }
      else if (secondary_image)
        {
        vtkKWTkUtilities::UpdatePhoto(
          interp, img_name.str(), secondary_image, canv);
        }
      this->LastHistogramBuildTime = (image_mtime > secondary_image_mtime 
                                      ? image_mtime : secondary_image_mtime);
      }
    }

  // Update the histogram position (or delete it if not needed anymore)

  ostrstream tk_cmd;

  if (this->Histogram || this->SecondaryHistogram)
    {
    if (!has_tag)
      {
      tk_cmd << canv << " create image 0 0 -anchor nw "
             << " -image " << img_name.str()
             << " -tags {" << VTK_KW_PVFE_HISTOGRAM_TAG << "}"
             << endl;
      if (this->ShowCanvasBackground)
        {
        tk_cmd << canv << " raise " << VTK_KW_PVFE_HISTOGRAM_TAG 
               << " " << VTK_KW_PVFE_FRAME_BG_TAG
               << endl;
        }
      }

    double factors[2] = {0.0, 0.0};
    this->GetCanvasScalingFactors(factors);
    
    double *v_w_range = this->GetWholeValueRange();
    double *v_v_range = this->GetVisibleValueRange();
    double c_y = factors[1] * (v_w_range[1] - v_v_range[1]);

    tk_cmd << canv << " coords " << VTK_KW_PVFE_HISTOGRAM_TAG
           << " " << this->HistogramImageDescriptor->Range[0] * factors[0] 
           << " " << c_y << endl;
    }
  else
    {
    if (has_tag)
      {
      tk_cmd << canv << " delete " << VTK_KW_PVFE_HISTOGRAM_TAG << endl;
      tk_cmd << "image delete " << img_name.str() << endl;
      }
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  img_name.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::HasSelection()
{
  return (this->SelectedPoint >= 0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectPoint(int id)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      this->SelectedPoint == id)
    {
    return;
    }

  // First deselect any selection

  this->ClearSelection();

  // Now selects

  this->SelectedPoint = id;

  // Add the selection tag to the point, raise the point

  if (this->IsCreated())
    {
    const char *canv = this->Canvas->GetWidgetName();

    ostrstream tk_cmd;

    tk_cmd << canv << " addtag " << VTK_KW_PVFE_SELECTED_TAG 
           << " withtag p" <<  this->SelectedPoint << endl;
    tk_cmd << canv << " addtag " << VTK_KW_PVFE_SELECTED_TAG 
           << " withtag t" <<  this->SelectedPoint << endl;
    tk_cmd << canv << " raise " << VTK_KW_PVFE_SELECTED_TAG << " all" << endl;

    tk_cmd << ends;
    this->Script(tk_cmd.str());
    tk_cmd.rdbuf()->freeze(0);
    }

  // Draw the selected point accordingly and update its aspect
  
  this->RedrawPoint(this->SelectedPoint);

  // Show the selected point description in the point label

  this->UpdatePointEntries(this->SelectedPoint);

  this->InvokeSelectionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ClearSelection()
{
  if (!this->HasSelection())
    {
    return;
    }

  // Remove the selection tag from the selected point

  if (this->IsCreated())
    {
    const char *canv = this->Canvas->GetWidgetName();

    ostrstream tk_cmd;

    tk_cmd << canv << " dtag p" <<  this->SelectedPoint
           << " " << VTK_KW_PVFE_SELECTED_TAG << endl;
    tk_cmd << canv << " dtag t" <<  this->SelectedPoint
           << " " << VTK_KW_PVFE_SELECTED_TAG << endl;

    tk_cmd << ends;
    this->Script(tk_cmd.str());
    tk_cmd.rdbuf()->freeze(0);
    }

  // Deselect

  int old_selection = this->SelectedPoint;
  this->SelectedPoint = -1;

  // Redraw the point that used to be selected and update its aspect

  this->RedrawPoint(old_selection);

  // Show the selected point description in the point label
  // Since nothing is selected, the expect side effect is to clear the
  // point label

  this->UpdatePointEntries(this->SelectedPoint);

  this->InvokeSelectionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectNextPoint()
{
  if (this->HasSelection())
    {
    this->SelectPoint(this->SelectedPoint == this->GetFunctionSize() - 1 
                      ? 0 : this->SelectedPoint + 1);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectPreviousPoint()
{
  if (this->HasSelection())
    {
    this->SelectPoint(this->SelectedPoint == 0  
                      ? this->GetFunctionSize() - 1 : this->SelectedPoint - 1);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectFirstPoint()
{
  this->SelectPoint(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectLastPoint()
{
  this->SelectPoint(this->GetFunctionSize() - 1);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::RemoveSelectedPoint()
{
  if (!this->HasSelection())
    {
    return 0;
    }

  return this->RemovePoint(this->SelectedPoint);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::RemovePoint(int id)
{
  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter) ||
      !this->FunctionPointCanBeRemoved(id) ||
      !this->RemoveFunctionPoint(id))
    {
    return 0;
    }

  // Redraw the points

  this->RedrawFunctionDependentElements();

  // If all points are gone, clear selection
  // If we the point was removed before the selection, shift the selection
  // If the selection was at the end, select the last one

  if (this->HasSelection())
    {
    if (!this->GetFunctionSize())
      {
      this->ClearSelection();
      }
    else if (id < this->SelectedPoint)
      {
      this->SelectPoint(this->SelectedPoint - 1);
      }
    else if (this->SelectedPoint >= this->GetFunctionSize())
      {
      this->SelectLastPoint();
      }
    }

  this->InvokePointRemovedCommand(id, parameter);
  this->InvokeFunctionChangedCommand();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::RemovePointAtParameter(double parameter)
{
  int fsize = this->GetFunctionSize();
  double point_param;
  for (int i = 0; i < fsize; i++)
    {
    if (this->GetFunctionPointParameter(i, &point_param) &&
        point_param == parameter)
      {
      return this->RemovePoint(i);
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddPointAtCanvasCoordinates(
  int x, int y, int &id)
{
  if (!this->AddFunctionPointAtCanvasCoordinates(x, y, id))
    {
    return 0;
    }

  // Draw the points (or all the points if the index have changed)

  this->RedrawFunctionDependentElements();

  // If the point was inserted before the selection, shift the selection

  if (this->HasSelection() && id <= this->SelectedPoint)
    {
    this->SelectPoint(this->SelectedPoint + 1);
    }

  this->InvokePointAddedCommand(id);
  this->InvokeFunctionChangedCommand();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddPointAtParameter(
  double parameter, int &id)
{
  if (!this->AddFunctionPointAtParameter(parameter, id))
    {
    return 0;
    }

  // Draw the points (or all the points if the index have changed)

  this->RedrawFunctionDependentElements();

  // If we the point was inserted before the selection, shift the selection

  if (this->HasSelection() && id <= this->SelectedPoint)
    {
    this->SelectPoint(this->SelectedPoint + 1);
    }

  this->InvokePointAddedCommand(id);
  this->InvokeFunctionChangedCommand();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::MergePointsFromEditor(
  vtkKWParameterValueFunctionEditor *editor)
{
  if (!this->HasFunction() || !editor || !editor->HasFunction())
    {
    return 0;
    }

  int old_size = this->GetFunctionSize();
  int editor_size = editor->GetFunctionSize();

  double parameter, editor_parameter;
  int new_id;

  // Browse all editor's point, get their parameters, add them to our own
  // function (the values will be interpolated automatically)

  for (int id = 0; id < editor_size; id++)
    {
    if (editor->GetFunctionPointParameter(id, &editor_parameter) &&
        (!this->GetFunctionPointParameter(id, &parameter) ||
         editor_parameter != parameter))
      {
      this->AddPointAtParameter(editor_parameter, new_id);
      }
    }

  // Do we have new points as the result of the merging ?

  int nb_merged = this->GetFunctionSize() - old_size;
  if (nb_merged)
    {
    this->InvokeFunctionChangedCommand();
    }

  return nb_merged;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::CanvasHasTag(const char *tag, 
                                                    int *suffix)
{
  if (!this->IsCreated())
    {
    return 0;
    }

  if (suffix)
    {
    return atoi(this->Script(
                  "llength [%s find withtag %s%d]",
                  this->Canvas->GetWidgetName(), tag, *suffix));
    }

  return atoi(this->Script(
                "llength [%s find withtag %s]",
                this->Canvas->GetWidgetName(), tag));
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CanvasRemoveTag(const char *tag, 
                                                        const char *canv_name)
{
  if (!this->IsCreated() || !tag || !*tag)
    {
    return;
    }

  if (!canv_name)
    {
    canv_name = this->Canvas->GetWidgetName();
    }

  this->Script("%s delete %s", canv_name, tag);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CanvasRemoveTag(const char *prefix, 
                                                        int id,
                                                        const char *canv_name)
{
  if (!this->IsCreated() || !prefix || !*prefix)
    {
    return;
    }

  if (!canv_name)
    {
    canv_name = this->Canvas->GetWidgetName();
    }

  this->Script("%s delete %s%d", canv_name, prefix, id);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::CanvasCheckTagType(
  const char *prefix, int id, const char *type)
{
  if (!this->IsCreated() || !prefix || !*prefix || !type || !*type)
    {
    return 0;
    }

  return !strcmp(
    type, 
    this->Script("%s type %s%d", this->Canvas->GetWidgetName(), prefix, id));
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetDisplayedWholeParameterRange(
  double r0, double r1)
{
  if (this->DisplayedWholeParameterRange[0] == r0 &&
      this->DisplayedWholeParameterRange[1] == r1)
    {
    return;
    }

  this->DisplayedWholeParameterRange[0] = r0;
  this->DisplayedWholeParameterRange[1] = r1;

  this->UpdateRangeLabel();
  this->UpdateParameterEntry(this->SelectedPoint);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetDisplayedVisibleParameterRange(
  double &r0, double &r1)
{
  if (this->DisplayedWholeParameterRange[0] !=
      this->DisplayedWholeParameterRange[1])
    {
    double displayed_delta = 
      (this->DisplayedWholeParameterRange[1] - 
       this->DisplayedWholeParameterRange[0]);
    double rel_vis_range[2];
    this->GetRelativeVisibleParameterRange(rel_vis_range);
    r0 = this->DisplayedWholeParameterRange[0] + 
      displayed_delta * rel_vis_range[0];
    r1 = this->DisplayedWholeParameterRange[0] + 
      displayed_delta * rel_vis_range[1];
    }
  else
    {
    this->GetVisibleParameterRange(r0, r1);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateRangeLabel()
{
  if (!this->IsCreated() || 
      !this->RangeLabel || 
      !this->RangeLabel->IsAlive() ||
      (!this->ShowParameterRange && !this->ShowValueRange))
    {
    return;
    }

  ostrstream ranges;
  int nb_ranges = 0;

  if (this->ShowParameterRange)
    {
    double param[2];
    this->GetDisplayedVisibleParameterRange(param[0], param[1]);
    char buffer[1024];
    sprintf(buffer, "[%g, %g]", param[0], param[1]);
    ranges << buffer;
    nb_ranges++;
    }

  double *value = GetVisibleValueRange();
  if (value && this->ShowValueRange)
    {
    char buffer[1024];
    sprintf(buffer, "[%g, %g]", value[0], value[1]);
    if (nb_ranges)
      {
      ranges << " x ";
      }
    ranges << buffer;
    nb_ranges++;
    }

  ranges << ends;
  this->RangeLabel->SetLabel(ranges.str());
  ranges.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateParameterEntry(int id)
{
  if (!this->IsCreated())
    {
    return;
    }
  
  // Update the parameter entry

  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter))
    {
    if (this->ParameterEntry)
      {
      if (this->ParameterEntry->GetEntry())
        {
        this->ParameterEntry->GetEntry()->SetValue("");
        }
      this->ParameterEntry->SetEnabled(0);
      }
    return;
    }

  this->ParameterEntry->SetEnabled(
    this->FunctionPointParameterIsLocked(id) ? 0 : this->Enabled);

  // Map from the displayed parameter range to the internal parameter range
  // if needed

  if (this->DisplayedWholeParameterRange[0] !=
      this->DisplayedWholeParameterRange[1])
    {
    double d_p_w_delta = 
      (this->DisplayedWholeParameterRange[1] - 
       this->DisplayedWholeParameterRange[0]);
    double *p_w_range = this->GetWholeParameterRange();
    double p_w_delta = p_w_range[1] - p_w_range[0];
    double rel_parameter = (parameter - p_w_range[0]) / p_w_delta;
    parameter = this->DisplayedWholeParameterRange[0] + 
      rel_parameter * d_p_w_delta;
    }

  if (this->ParameterEntryFormat)
    {
    char buffer[256];
    sprintf(buffer, this->ParameterEntryFormat, parameter);
    this->ParameterEntry->GetEntry()->SetValue(buffer);
    }
  else
    {
    this->ParameterEntry->GetEntry()->SetValue(parameter);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ParameterEntryCallback()
{
  if (!this->ParameterEntry || !this->HasSelection())
    {
    return;
    }

  unsigned long mtime = this->GetFunctionMTime();

  double parameter = this->ParameterEntry->GetEntry()->GetValueAsFloat();

  // Map from the internal parameter range to the displayed  parameter range
  // if needed

  if (this->DisplayedWholeParameterRange[0] !=
      this->DisplayedWholeParameterRange[1])
    {
    double d_p_w_delta = 
      (this->DisplayedWholeParameterRange[1] - 
       this->DisplayedWholeParameterRange[0]);
    double *p_w_range = this->GetWholeParameterRange();
    double p_w_delta = p_w_range[1] - p_w_range[0];
    double rel_parameter = 
      (parameter - this->DisplayedWholeParameterRange[0]) / d_p_w_delta;
    parameter = p_w_range[0] + rel_parameter * p_w_delta;
    }

  this->MoveFunctionPointToParameter(
    this->GetSelectedPoint(), parameter);

  if (this->GetFunctionMTime() > mtime)
    {
    this->InvokePointMovedCommand(this->SelectedPoint);
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateHistogramLogModeOptionMenu()
{
  if (this->HistogramLogModeOptionMenu)
    {
    vtkKWHistogram *hist = 
      this->Histogram ? this->Histogram : this->SecondaryHistogram;
    int log_mode = 1;
    if (hist)
      {
      log_mode = hist->GetLogMode();
      }
    ostrstream img_name;
    img_name << this->HistogramLogModeOptionMenu->GetWidgetName() 
             << ".img" << log_mode << ends;
    this->HistogramLogModeOptionMenu->SetCurrentImageEntry(img_name.str());
    img_name.rdbuf()->freeze(0);
    if (!hist)
      {
      this->HistogramLogModeOptionMenu->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddObserversList(
  int nb_events, int *events, vtkCommand *cmd)
{
 if (nb_events <= 0 || !events || !cmd)
   {
   return 0;
   }

  int added = 0;
  for (int i = 0; i < nb_events; i++)
    {
    if (!this->HasObserver(events[i], cmd))
      {
      this->AddObserver(events[i], cmd);
      added++;
      }
    }

  return added;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::RemoveObserversList(
  int nb_events, int *events, vtkCommand *cmd)
{
 if (nb_events <= 0 || !events || !cmd)
   {
   return 0;
   }

  int removed = 0;
  for (int i = 0; i < nb_events; i++)
    {
    if (this->HasObserver(events[i], cmd))
      {
      this->RemoveObservers(events[i], cmd);
      removed++;
      }
    }

  return removed;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizeVisibleParameterRange(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method

  if (!a || !b)
    {
    return 0;
    }
  
  // Make sure both editors have the same visible range from now

  b->SetVisibleParameterRange(a->GetVisibleParameterRange());

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent,
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent
    };

  b->AddObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand);

  a->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizeVisibleParameterRange(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method

  if (!a || !b)
    {
    return 0;
    }

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent,
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent
    };

  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand);

  a->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizePoints(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method

  if (!a || !b)
    {
    return 0;
    }

  // Make sure they share the same points in the parameter space from now

  a->MergePointsFromEditor(b);
  b->MergePointsFromEditor(a);

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::PointMovingEvent,
      vtkKWParameterValueFunctionEditor::PointRemovedEvent,
      vtkKWParameterValueFunctionEditor::FunctionChangedEvent
    };

  b->AddObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand);

  a->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizePoints(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method

  if (!a || !b)
    {
    return 0;
    }

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::PointMovingEvent,
      vtkKWParameterValueFunctionEditor::PointRemovedEvent,
      vtkKWParameterValueFunctionEditor::FunctionChangedEvent
    };

  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand);

  a->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizeSingleSelection(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method

  if (!a || !b)
    {
    return 0;
    }
  
  // Make sure only one of those editors has a selected point from now
  
  if (a->HasSelection())
    {
    b->ClearSelection();
    }
  else if (b->HasSelection())
    {
    a->ClearSelection();
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->AddObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand);

  a->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizeSingleSelection(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method
  
  if (!a || !b)
    {
    return 0;
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand);

  a->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizeSameSelection(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method

  if (!a || !b)
    {
    return 0;
    }
  
  // Make sure those editors have the same selected point from now
  
  if (a->HasSelection())
    {
    b->SelectPoint(a->GetSelectedPoint());
    }
  else if (b->HasSelection())
    {
    a->SelectPoint(b->GetSelectedPoint());
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->AddObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand2);

  a->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand2);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizeSameSelection(
  vtkKWParameterValueFunctionEditor *a,
  vtkKWParameterValueFunctionEditor *b)
{
  // Static method
  
  if (!a || !b)
    {
    return 0;
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, a->SynchronizeCallbackCommand2);
  
  a->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand2);
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents(
  vtkObject *object,
  unsigned long event,
  void *clientdata,
  void *calldata)
{
  vtkKWParameterValueFunctionEditor *pvfe =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(object);
  
  vtkKWParameterValueFunctionEditor *self =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(clientdata);
  
  double parameter;
  int *point_id = reinterpret_cast<int *>(calldata);
  double *dargs = reinterpret_cast<double *>(calldata);
  double range[2];

  switch (event)
    {
    // Synchronize visible range
    
    case vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent:
    case vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent:
      pvfe->GetRelativeVisibleParameterRange(range);
      self->SetRelativeVisibleParameterRange(range);
      break;
      
    // Synchronize points
      
    case vtkKWParameterValueFunctionEditor::PointMovingEvent:
      if (pvfe->GetFunctionPointParameter(*point_id, &parameter))
        {
        self->MoveFunctionPointToParameter(*point_id, parameter);
        }
      break;

    case vtkKWParameterValueFunctionEditor::PointRemovedEvent:
      self->RemovePointAtParameter(dargs[1]);
      break;

    case vtkKWParameterValueFunctionEditor::FunctionChangedEvent:
      self->MergePointsFromEditor(pvfe);
      break;

    // Synchronize Single selection

    case vtkKWParameterValueFunctionEditor::SelectionChangedEvent:
      if (pvfe->HasSelection())
        {
        self->ClearSelection();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents2(
  vtkObject *object,
  unsigned long event,
  void *clientdata,
  void *vtkNotUsed(calldata))
{
  vtkKWParameterValueFunctionEditor *pvfe =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(object);
  
  vtkKWParameterValueFunctionEditor *self =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(clientdata);
  
  switch (event)
    {
    // Synchronize Same selection

    case vtkKWParameterValueFunctionEditor::SelectionChangedEvent:
      if (pvfe->HasSelection())
        {
        self->SelectPoint(pvfe->GetSelectedPoint());
        }
      else
        {
        self->ClearSelection();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ConfigureCallback()
{
  this->Redraw();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CanvasEnterCallback()
{
  if (this->IsCreated())
    {
    this->Script("focus %s", this->Canvas->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingCallback()
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangingCommand();

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedCallback()
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangedCommand();

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleValueRangeChangingCallback()
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleValueRangeChangedCallback()
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::StartInteractionCallback(int x, int y)
{
  if (!this->IsCreated() || !this->HasFunction())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If we are out of the canvas, clamp the coordinates

  if (x < 0)
    {
    x = 0;
    }
  else if (x > this->CanvasWidth - 1)
    {
    x = this->CanvasWidth - 1;
    }

  if (y < 0)
    {
    y = 0;
    }
  else if (y > this->CanvasHeight - 1)
    {
    y = this->CanvasHeight - 1;
    }

  // Get the real canvas coordinates

  int c_x = atoi(this->Script("%s canvasx %d", canv, x));
  int c_y = atoi(this->Script("%s canvasy %d", canv, y));

  // Find the closest element
  // Get its first tag, which should be either a point or a text (in
  // the form of tid or pid, ex: t0, or p0)

  int id = -1;

  const char *closest = this->Script("%s find closest %d %d", 
                                     this->Canvas->GetWidgetName(), c_x, c_y);
  if (closest && *closest)
    {
    const char *tag = this->Script("lindex [%s itemcget %s -tags] 0",
                                   this->Canvas->GetWidgetName(), closest);
    if (tag && *tag && (tag[0] == 't' || tag[0] == 'p'))
      {
      id = atoi(tag + 1);
      }
    }

  // No point found, then let's add that point

  if (id < 0 || id >= this->GetFunctionSize())
    {
    if (!this->AddPointAtCanvasCoordinates(c_x, c_y, id))
      {
      return;
      }
    }

  // Select the point (that was found or just added)

  this->SelectPoint(id);
  this->GetFunctionPointCanvasCoordinates(this->SelectedPoint, c_x, c_y);
  this->LastSelectCanvasCoordinates[0] = c_x;
  this->LastSelectCanvasCoordinates[1] = c_y;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::MovePointCallback(
  int x, int y, int shift)
{
  if (!this->IsCreated() || !this->HasSelection())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If we are out of the canvas by a given "delete" margin, warn that 
  // the point is going to be deleted (do not delete here now to give
  // the user a chance to recover)

  int warn_delete = 
    (this->FunctionPointCanBeRemoved(this->SelectedPoint) &&
     (x < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
      x > this->CanvasWidth - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
      y < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
      y > this->CanvasHeight - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN));

  // If we are out of the canvas, clamp the coordinates

  if (x < 0)
    {
    x = 0;
    }
  else if (x > this->CanvasWidth - 1)
    {
    x = this->CanvasWidth - 1;
    }

  if (y < 0)
    {
    y = 0;
    }
  else if (y > this->CanvasHeight - 1)
    {
    y = this->CanvasHeight - 1;
    }

  // Get the real canvas coordinates

  int c_x = atoi(this->Script("%s canvasx %d", canv, x));
  int c_y = atoi(this->Script("%s canvasy %d", canv, y));

  // We assume we can not go before or beyond the previous or next point

  if (this->SelectedPoint > 0)
    {
    int prev_x, prev_y;
    this->GetFunctionPointCanvasCoordinates(this->SelectedPoint - 1, 
                                            prev_x, prev_y);
    if (c_x <= prev_x)
      {
      c_x = prev_x + 1;
      }
    }

  if (this->SelectedPoint < this->GetFunctionSize() - 1)
    {
    int next_x, next_y;
    this->GetFunctionPointCanvasCoordinates(this->SelectedPoint + 1,
                                            next_x, next_y);
    if (c_x >= next_x)
      {
      c_x = next_x - 1;
      }
    }

  // Are we constrained vertically or horizontally ?

  int move_h_only = this->FunctionPointValueIsLocked(this->SelectedPoint);
  int move_v_only = this->FunctionPointParameterIsLocked(this->SelectedPoint);

  if (shift)
    {
    if (this->LastConstrainedMove == 
        vtkKWParameterValueFunctionEditor::ConstrainedMoveFree)
      {
      if (fabs((double)(c_x - LastSelectCanvasCoordinates[0])) >
          fabs((double)(c_y - LastSelectCanvasCoordinates[1])))
        {
        this->LastConstrainedMove = 
          vtkKWParameterValueFunctionEditor::ConstrainedMoveHorizontal;
        }
      else
        {
        this->LastConstrainedMove = 
          vtkKWParameterValueFunctionEditor::ConstrainedMoveVertical;
        }
      }
    if (this->LastConstrainedMove == 
        vtkKWParameterValueFunctionEditor::ConstrainedMoveHorizontal)
      {
      move_h_only = 1;
      c_y = LastSelectCanvasCoordinates[1];
      }
    else if (this->LastConstrainedMove == 
             vtkKWParameterValueFunctionEditor::ConstrainedMoveVertical)
      {
      move_v_only = 1;
      c_x = LastSelectCanvasCoordinates[0];
      }
    }
  else
    {
    this->LastConstrainedMove = 
      vtkKWParameterValueFunctionEditor::ConstrainedMoveFree;
    }

  // Update cursor to show which interaction is going on

  if (this->ChangeMouseCursor)
    {
    const char *cursor;
    if (warn_delete)
      {
      cursor = "icon";
      }
    else
      {
      if (move_h_only && move_v_only)
        {
        cursor = "diamond_cross";
        }
      else if (move_h_only)
        {
        cursor = "sb_h_double_arrow";
        }
      else if (move_v_only)
        {
        cursor = "sb_v_double_arrow";
        }
      else
        {
        cursor = "fleur";
        }
      }
    this->Script("%s config -cursor %s", 
                 this->Canvas->GetWidgetName(), cursor);
    }

  // Now update the point given those coords, and update the info label

  this->MoveFunctionPointToCanvasCoordinates(
    this->SelectedPoint, c_x, c_y);

  // Invoke the commands/callbacks

  this->InvokePointMovingCommand(this->SelectedPoint);
  this->InvokeFunctionChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::EndInteractionCallback(int x, int y)
{
  if (!this->HasSelection())
    {
    return;
    }

  // Invoke the commands/callbacks
  // If we are out of the canvas by a given margin, delete the point

  if (this->FunctionPointCanBeRemoved(this->SelectedPoint) &&
      (x < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
       x > this->CanvasWidth - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
       y < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
       y > this->CanvasHeight - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN))
    {
    this->RemovePoint(this->SelectedPoint);
    }
  else
    {
    this->InvokePointMovedCommand(this->SelectedPoint);
    this->InvokeFunctionChangedCommand();
    }

  // Remove any interaction icon

  if (this->IsCreated() && this->ChangeMouseCursor)
    {
    this->Script("%s config -cursor {}", this->Canvas->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::HistogramLogModeCallback(int mode)
{
  if (this->Histogram)
    {
    this->Histogram->SetLogMode(mode);
    }
  if (this->SecondaryHistogram)
    {
    this->SecondaryHistogram->SetLogMode(mode);
    }

  this->UpdateHistogramLogModeOptionMenu();
  this->RedrawHistogram();
  if (this->ComputeValueTicksFromHistogram)
    {
    this->RedrawRangeTicks();
    }
  this->InvokeHistogramLogModeChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ShowParameterRange: "
     << (this->ShowParameterRange ? "On" : "Off") << endl;
  os << indent << "ShowValueRange: "
     << (this->ShowValueRange ? "On" : "Off") << endl;
  os << indent << "LabelPosition: " << this->LabelPosition << endl;
  os << indent << "ShowRangeLabel: "
     << (this->ShowRangeLabel ? "On" : "Off") << endl;
  os << indent << "RangeLabelPosition: " << this->RangeLabelPosition << endl;
  os << indent << "ParameterEntryPosition: " << this->ParameterEntryPosition << endl;
  os << indent << "ShowParameterEntry: "
     << (this->ShowParameterEntry ? "On" : "Off") << endl;
  os << indent << "ShowUserFrame: "
     << (this->ShowUserFrame ? "On" : "Off") << endl;
  os << indent << "CanvasHeight: "<< this->CanvasHeight << endl;
  os << indent << "CanvasWidth: "<< this->CanvasWidth << endl;
  os << indent << "ExpandCanvasWidth: "
     << (this->ExpandCanvasWidth ? "On" : "Off") << endl;
  os << indent << "PointRadius: "<< this->PointRadius << endl;
  os << indent << "TicksLength: "<< this->TicksLength << endl;
  os << indent << "NumberOfParameterTicks: "
     << this->NumberOfParameterTicks << endl;
  os << indent << "NumberOfValueTicks: "
     << this->NumberOfValueTicks << endl;
  os << indent << "ValueTicksCanvasWidth: "
     << this->ValueTicksCanvasWidth << endl;
  os << indent << "ValueTicksFormat: "
     << (this->ValueTicksFormat ? this->ValueTicksFormat : "(None)") << endl;
  os << indent << "ParameterTicksFormat: "
     << (this->ParameterTicksFormat ? this->ParameterTicksFormat : "(None)") << endl;
  os << indent << "ParameterEntryFormat: "
     << (this->ParameterEntryFormat ? this->ParameterEntryFormat : "(None)") << endl;
  os << indent << "SelectedPointRadius: " 
     << this->SelectedPointRadius << endl;
  os << indent << "DisableCommands: "
     << (this->DisableCommands ? "On" : "Off") << endl;
  os << indent << "LockEndPointsParameter: "
     << (this->LockEndPointsParameter ? "On" : "Off") << endl;
  os << indent << "RescaleBetweenEndPoints: "
     << (this->RescaleBetweenEndPoints ? "On" : "Off") << endl;
  os << indent << "PointMarginToCanvas: " << this->PointMarginToCanvas << endl;
  os << indent << "CanvasOutlineStyle: " << this->CanvasOutlineStyle << endl;
  os << indent << "DisableAddAndRemove: "
     << (this->DisableAddAndRemove ? "On" : "Off") << endl;
  os << indent << "ChangeMouseCursor: "
     << (this->ChangeMouseCursor ? "On" : "Off") << endl;
  os << indent << "SelectedPoint: "<< this->SelectedPoint << endl;
  os << indent << "FrameBackgroundColor: ("
     << this->FrameBackgroundColor[0] << ", " 
     << this->FrameBackgroundColor[1] << ", " 
     << this->FrameBackgroundColor[2] << ")" << endl;
  os << indent << "HistogramColor: ("
     << this->HistogramColor[0] << ", " 
     << this->HistogramColor[1] << ", " 
     << this->HistogramColor[2] << ")" << endl;
  os << indent << "SecondaryHistogramColor: ("
     << this->SecondaryHistogramColor[0] << ", " 
     << this->SecondaryHistogramColor[1] << ", " 
     << this->SecondaryHistogramColor[2] << ")" << endl;
  os << indent << "PointColor: ("
     << this->PointColor[0] << ", " 
     << this->PointColor[1] << ", " 
     << this->PointColor[2] << ")" << endl;
  os << indent << "SelectedPointColor: ("
     << this->SelectedPointColor[0] << ", " 
     << this->SelectedPointColor[1] << ", " 
     << this->SelectedPointColor[2] << ")" << endl;
  os << indent << "PointTextColor: ("
     << this->PointTextColor[0] << ", " 
     << this->PointTextColor[1] << ", " 
     << this->PointTextColor[2] << ")" << endl;
  os << indent << "SelectedPointTextColor: ("
     << this->SelectedPointTextColor[0] << ", " 
     << this->SelectedPointTextColor[1] << ", " 
     << this->SelectedPointTextColor[2] << ")" << endl;
  os << indent << "ParameterCursorColor: ("
     << this->ParameterCursorColor[0] << ", " 
     << this->ParameterCursorColor[1] << ", " 
     << this->ParameterCursorColor[2] << ")" << endl;
  os << indent << "ComputePointColorFromValue: "
     << (this->ComputePointColorFromValue ? "On" : "Off") << endl;
  os << indent << "ComputeHistogramColorFromValue: "
     << (this->ComputeHistogramColorFromValue ? "On" : "Off") << endl;
  os << indent << "HistogramStyle: "<< this->HistogramStyle << endl;
  os << indent << "SecondaryHistogramStyle: "<< this->SecondaryHistogramStyle << endl;
  os << indent << "ShowFunctionLine: "
     << (this->ShowFunctionLine ? "On" : "Off") << endl;
  os << indent << "ShowPointIndex: "
     << (this->ShowPointIndex ? "On" : "Off") << endl;
  os << indent << "ShowPointGuideline: "
     << (this->ShowPointGuideline ? "On" : "Off") << endl;
  os << indent << "ShowSelectedPointIndex: "
     << (this->ShowSelectedPointIndex ? "On" : "Off") << endl;
  os << indent << "ShowHistogramLogModeOptionMenu: "
     << (this->ShowHistogramLogModeOptionMenu ? "On" : "Off") << endl;
  os << indent << "ShowParameterCursor: "
     << (this->ShowParameterCursor ? "On" : "Off") << endl;
  os << indent << "DisplayedWholeParameterRange: ("
     << this->DisplayedWholeParameterRange[0] << ", " 
     << this->DisplayedWholeParameterRange[1] << ")" << endl;
  os << indent << "PointStyle: " << this->PointStyle << endl;
  os << indent << "FirstPointStyle: " << this->FirstPointStyle << endl;
  os << indent << "LastPointStyle: " << this->LastPointStyle << endl;
  os << indent << "FunctionLineStyle: " << this->FunctionLineStyle << endl;
  os << indent << "FunctionLineWidth: " << this->FunctionLineWidth << endl;
  os << indent << "ParameterCursorPosition: " 
     << this->ParameterCursorPosition << endl;
  os << indent << "PointGuidelineStyle: " << this->PointGuidelineStyle << endl;
  os << indent << "PointOutlineWidth: " << this->PointOutlineWidth << endl;
  os << indent << "PointPositionInValueRange: " << this->PointPositionInValueRange << endl;
  os << indent << "ParameterRangePosition: " << this->ParameterRangePosition << endl;
  os << indent << "ShowCanvasOutline: "
     << (this->ShowCanvasOutline ? "On" : "Off") << endl;
  os << indent << "ShowCanvasBackground: "
     << (this->ShowCanvasBackground ? "On" : "Off") << endl;
  os << indent << "ShowParameterTicks: "
     << (this->ShowParameterTicks ? "On" : "Off") << endl;
  os << indent << "ShowValueTicks: "
     << (this->ShowValueTicks ? "On" : "Off") << endl;
  os << indent << "ComputeValueTicksFromHistogram: "
     << (this->ComputeValueTicksFromHistogram ? "On" : "Off") << endl;

  os << indent << "ParameterRange: ";
  if (this->ParameterRange)
    {
    os << endl;
    this->ParameterRange->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ValueRange: ";
  if (this->ValueRange)
    {
    os << endl;
    this->ValueRange->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "Canvas: ";
  if (this->Canvas)
    {
    os << endl;
    this->Canvas->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "UserFrame: ";
  if (this->UserFrame)
    {
    os << endl;
    this->UserFrame->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "RangeLabel: ";
  if (this->RangeLabel)
    {
    os << endl;
    this->RangeLabel->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "ParameterEntry: ";
  if (this->ParameterEntry)
    {
    os << endl;
    this->ParameterEntry->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }

  os << indent << "Histogram: ";
  if (this->Histogram)
    {
    os << endl;
    this->Histogram->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "SecondaryHistogram: ";
  if (this->SecondaryHistogram)
    {
    os << endl;
    this->SecondaryHistogram->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
  os << indent << "HistogramLogModeOptionMenu: ";
  if (this->HistogramLogModeOptionMenu)
    {
    os << endl;
    this->HistogramLogModeOptionMenu->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "None" << endl;
    }
}
