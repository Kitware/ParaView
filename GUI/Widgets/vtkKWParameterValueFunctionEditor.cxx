/*=========================================================================

  Module:    vtkKWParameterValueFunctionEditor.cxx,v

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
#include "vtkKWCanvas.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWHistogram.h"
#include "vtkKWInternationalization.h"
#include "vtkKWLabel.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWRange.h"
#include "vtkKWTkUtilities.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkImageBlend.h"
#include "vtkKWMenuButton.h"
#include "vtkKWMenu.h"
#include "vtkKWIcon.h"
#include "vtkIntArray.h"

#include <ctype.h>

#include <vtksys/stl/string>
#include <vtksys/stl/vector>
#include <vtksys/stl/algorithm>
#include <vtksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkKWParameterValueFunctionEditor, "1.98");

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
#define VTK_KW_PVFE_GUIDELINE_VALUE_CANVAS_HEIGHT VTK_KW_PVFE_TICKS_PARAMETER_CANVAS_HEIGHT

// For some reasons, the end-point of a line/rectangle is not drawn on Win32. 
// Comply with that.

#ifndef _WIN32
#define LSTRANGE 0
#else
#define LSTRANGE 1
#endif
#define RSTRANGE 1

#define VTK_KW_PVFE_TESTING 0

const char *vtkKWParameterValueFunctionEditor::FunctionTag = "function_tag";
const char *vtkKWParameterValueFunctionEditor::SelectedTag = "selected_tag";
const char *vtkKWParameterValueFunctionEditor::PointTag = "point_tag";
const char *vtkKWParameterValueFunctionEditor::PointGuidelineTag = "point_guideline_tag";
const char *vtkKWParameterValueFunctionEditor::LineTag = "line_tag";
const char *vtkKWParameterValueFunctionEditor::PointTextTag = "point_text_tag";
const char *vtkKWParameterValueFunctionEditor::HistogramTag = "histogram_tag";
const char *vtkKWParameterValueFunctionEditor::SecondaryHistogramTag = "secondary_histogram_tag";
const char *vtkKWParameterValueFunctionEditor::FrameForegroundTag = "framefg_tag";
const char *vtkKWParameterValueFunctionEditor::FrameBackgroundTag = "framebg_tag";
const char *vtkKWParameterValueFunctionEditor::ParameterCursorTag = "cursor_tag";
const char *vtkKWParameterValueFunctionEditor::ParameterTicksTag = "p_ticks_tag";
const char *vtkKWParameterValueFunctionEditor::ValueTicksTag = "v_ticks_tag";

//----------------------------------------------------------------------------
vtkKWParameterValueFunctionEditor::vtkKWParameterValueFunctionEditor()
{
  this->ParameterRangeVisibility          = 1;
  this->ValueRangeVisibility              = 1;
  this->PointPositionInValueRange   = vtkKWParameterValueFunctionEditor::PointPositionValue;
  this->ParameterRangePosition      = vtkKWParameterValueFunctionEditor::ParameterRangePositionBottom;
  this->RequestedCanvasHeight       = 55;
  this->CurrentCanvasHeight         = this->RequestedCanvasHeight;
  this->RequestedCanvasWidth        = 150;
  this->CurrentCanvasWidth          = this->RequestedCanvasWidth;
  this->ExpandCanvasWidth           = 1;
  this->LockPointsParameter         = 0;
  this->LockEndPointsParameter      = 0;
  this->LockPointsValue             = 0;
  this->RescaleBetweenEndPoints     = 0;
  this->DisableAddAndRemove         = 0;
  this->DisableRedraw               = 0;
  this->PointRadiusX                = 4;
  this->PointRadiusY                = this->PointRadiusX;
  this->SelectedPointRadius         = 1.45;
  this->SelectedPointText         = NULL;
  this->DisableCommands             = 0;
  this->SelectedPoint               = -1;
  this->FunctionLineWidth           = 2;
  this->HistogramPolyLineWidth      = 1;
  this->PointOutlineWidth           = 1;
  this->FunctionLineStyle           = vtkKWParameterValueFunctionEditor::LineStyleSolid;
  this->PointGuidelineStyle         = vtkKWParameterValueFunctionEditor::LineStyleDash;
  this->PointStyle                  = vtkKWParameterValueFunctionEditor::PointStyleDisc;
  this->FirstPointStyle             = vtkKWParameterValueFunctionEditor::PointStyleDefault;
  this->LastPointStyle              = vtkKWParameterValueFunctionEditor::PointStyleDefault;
  this->CanvasOutlineVisibility           = 1;
  this->CanvasOutlineStyle          = vtkKWParameterValueFunctionEditor::CanvasOutlineStyleAllSides;
  this->ParameterCursorInteractionStyle          = vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleNone;
  this->ParameterTicksVisibility          = 0;
  this->ValueTicksVisibility              = 0;
  this->ComputeValueTicksFromHistogram = 0;
  this->CanvasBackgroundVisibility        = 1;
  this->FunctionLineVisibility            = 1;
  this->CanvasVisibility            = 1;
  this->PointIndexVisibility              = 0;
  this->PointGuidelineVisibility          = 0;
  this->PointVisibility          = 1;
  this->SelectedPointIndexVisibility      = 1;
  this->ParameterRangeLabelVisibility              = 1;
  this->ValueRangeLabelVisibility              = 1;
  this->RangeLabelPosition          = vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault;
  this->PointEntriesPosition      = vtkKWParameterValueFunctionEditor::PointEntriesPositionDefault;
  this->ParameterEntryVisibility          = 1;
  this->PointEntriesVisibility          = 1;
  this->UserFrameVisibility               = 0;
  this->PointMarginToCanvas         = vtkKWParameterValueFunctionEditor::PointMarginAllSides;
  this->TicksLength                 = 5;
  this->NumberOfParameterTicks      = 6;
  this->NumberOfValueTicks          = 6;
  this->ValueTicksCanvasWidth       = VTK_KW_PVFE_TICKS_VALUE_CANVAS_WIDTH;
  this->ChangeMouseCursor          = 1;
  this->PointColorStyle      = vtkKWParameterValueFunctionEditor::PointColorStyleFill;

  this->ParameterTicksFormat        = NULL;
  this->SetParameterTicksFormat("%-#6.3g");
  this->ValueTicksFormat            = NULL;
  this->SetValueTicksFormat(this->GetParameterTicksFormat());
  this->ParameterEntryFormat        = NULL;

#if 0
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

  this->HistogramLogModeOptionMenuVisibility  = 0;
  this->HistogramLogModeOptionMenu      = vtkKWMenuButton::New();
  this->HistogramLogModeChangedCommand  = NULL;

  this->SecondaryHistogramColor[0]  = 0.0;
  this->SecondaryHistogramColor[1]  = 0.0;
  this->SecondaryHistogramColor[2]  = 0.0;

  this->ComputeHistogramColorFromValue = 0;
  this->HistogramStyle     = vtkKWHistogram::ImageDescriptor::StyleBars;
  this->SecondaryHistogramStyle = vtkKWHistogram::ImageDescriptor::StyleDots;

  this->ParameterCursorColor[0]     = 0.2;
  this->ParameterCursorColor[1]     = 0.2;
  this->ParameterCursorColor[2]     = 0.4;

  this->PointColor[0]               = 1.0;
  this->PointColor[1]               = 1.0;
  this->PointColor[2]               = 1.0;

  this->SelectedPointColor[0]       = 0.737; // 0.59;
  this->SelectedPointColor[1]       = 0.772; // 0.63;
  this->SelectedPointColor[2]       = 0.956; // 0.82;

  this->SelectedPointColorInInteraction[0]       = -1;
  this->SelectedPointColorInInteraction[1]       = -1;
  this->SelectedPointColorInInteraction[2]       = -1;

  this->PointTextColor[0]           = 0.0;
  this->PointTextColor[1]           = 0.0;
  this->PointTextColor[2]           = 0.0;

  this->SelectedPointTextColor[0]   = 0.0;
  this->SelectedPointTextColor[1]   = 0.0;
  this->SelectedPointTextColor[2]   = 0.0;

  this->ComputePointColorFromValue     = 0;

  this->InUserInteraction           = 0;

  this->PointAddedCommand           = NULL;
  this->PointChangingCommand          = NULL;
  this->PointChangedCommand           = NULL;
  this->DoubleClickOnPointCommand  = NULL;
  this->PointRemovedCommand         = NULL;
  this->SelectionChangedCommand     = NULL;
  this->FunctionChangedCommand      = NULL;
  this->FunctionChangingCommand     = NULL;
  this->FunctionStartChangingCommand     = NULL;
  this->VisibleRangeChangedCommand  = NULL;
  this->VisibleRangeChangingCommand = NULL;
  this->ParameterCursorMovingCommand          = NULL;
  this->ParameterCursorMovedCommand           = NULL;

  this->Canvas                      = vtkKWCanvas::New();
  this->ParameterRange              = vtkKWRange::New();
  this->ValueRange                  = vtkKWRange::New();
  this->TopLeftContainer            = vtkKWFrame::New();
  this->TopLeftFrame                = vtkKWFrame::New();
  this->UserFrame                   = vtkKWFrame::New();
  this->PointEntriesFrame           = vtkKWFrame::New();
  this->RangeLabel                  = vtkKWLabel::New();
  this->ParameterEntry              = NULL;
  this->ValueTicksCanvas            = vtkKWCanvas::New();
  this->ParameterTicksCanvas        = vtkKWCanvas::New();
  this->GuidelineValueCanvas               = vtkKWCanvas::New();

  this->DisplayedWholeParameterRange[0] = 0.0;
  this->DisplayedWholeParameterRange[1] = 
    this->DisplayedWholeParameterRange[0];

  this->ParameterCursorVisibility         = 0;
  this->ParameterCursorPosition     = this->ParameterRange->GetRange()[0];

  this->LastRedrawFunctionTime      = 0;
  this->LastRedrawFunctionSize      = 0;

  this->LastSelectionCanvasCoordinateX    = 0;
  this->LastSelectionCanvasCoordinateY    = 0;
  this->LastConstrainedMove               = vtkKWParameterValueFunctionEditor::ConstrainedMoveFree;

  // Synchronization callbacks
  
  this->SynchronizeCallbackCommand = vtkCallbackCommand::New();
  this->SynchronizeCallbackCommand->SetClientData(this); 
  this->SynchronizeCallbackCommand->SetCallback(
    vtkKWParameterValueFunctionEditor::ProcessSynchronizationEventsFunction);

  this->SynchronizeCallbackCommand2 = vtkCallbackCommand::New();
  this->SynchronizeCallbackCommand2->SetClientData(this); 
  this->SynchronizeCallbackCommand2->SetCallback(
    vtkKWParameterValueFunctionEditor::ProcessSynchronizationEventsFunction2);

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

  if (this->PointChangingCommand)
    {
    delete [] this->PointChangingCommand;
    this->PointChangingCommand = NULL;
    }

  if (this->PointChangedCommand)
    {
    delete [] this->PointChangedCommand;
    this->PointChangedCommand = NULL;
    }

  if (this->DoubleClickOnPointCommand)
    {
    delete [] this->DoubleClickOnPointCommand;
    this->DoubleClickOnPointCommand = NULL;
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

  if (this->FunctionStartChangingCommand)
    {
    delete [] this->FunctionStartChangingCommand;
    this->FunctionStartChangingCommand = NULL;
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

  if (this->ParameterCursorMovingCommand)
    {
    delete [] this->ParameterCursorMovingCommand;
    this->ParameterCursorMovingCommand = NULL;
    }

  if (this->ParameterCursorMovedCommand)
    {
    delete [] this->ParameterCursorMovedCommand;
    this->ParameterCursorMovedCommand = NULL;
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

  if (this->PointEntriesFrame)
    {
    this->PointEntriesFrame->Delete();
    this->PointEntriesFrame = NULL;
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

  if (this->GuidelineValueCanvas)
    {
    this->GuidelineValueCanvas->Delete();
    this->GuidelineValueCanvas = NULL;
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
  this->SetValueTicksFormat(NULL);
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
  return 
    (this->HasFunction() &&
     (this->LockPointsParameter ||
      (this->LockEndPointsParameter &&
       (id == 0 || 
        (this->GetFunctionSize() && id == this->GetFunctionSize() - 1)))));
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FunctionPointValueIsLocked(int)
{
 return (this->HasFunction() && this->LockPointsValue);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetLockPointsParameter(int arg)
{
  if (this->LockPointsParameter == arg)
    {
    return;
    }

  this->LockPointsParameter = arg;
  this->Modified();

  this->UpdatePointEntries(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetLockEndPointsParameter(int arg)
{
  if (this->LockEndPointsParameter == arg)
    {
    return;
    }

  this->LockEndPointsParameter = arg;
  this->Modified();

  this->UpdatePointEntries(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetLockPointsValue(int arg)
{
  if (this->LockPointsValue == arg)
    {
    return;
    }

  this->LockPointsValue = arg;
  this->Modified();

  this->UpdatePointEntries(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetReadOnly(int arg)
{
  this->SetLockPointsParameter(arg);
  this->SetLockPointsValue(arg);
  this->SetDisableAddAndRemove(arg);
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
 
  // Neighborhood checks do not apply if the point being moved if one of the
  // end points and the RescaleBetweenEndPoints has been enabled.
  if (this->RescaleBetweenEndPoints &&
    (id == 0 || id == this->GetFunctionSize() - 1))
    {
    return 1;
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

  if (id == this->GetSelectedPoint() &&
      this->InUserInteraction &&
      this->SelectedPointColorInInteraction[0] >= 0.0 &&
      this->SelectedPointColorInInteraction[1] >= 0.0 &&
      this->SelectedPointColorInInteraction[2] >= 0.0)
    {
    rgb[0] = this->SelectedPointColorInInteraction[0];
    rgb[1] = this->SelectedPointColorInInteraction[1];
    rgb[2] = this->SelectedPointColorInInteraction[2];
    return 1;
    }

  if (!this->ComputePointColorFromValue)
    {
    if (id == this->GetSelectedPoint())
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
      // Just in case the point values are outside the whole range, which
      // is not likely to happen, but who knows.
      if (rgb[i] < 0.0)
        {
        rgb[i] = 0;
        } 
      else if (rgb[i] > 1.0) 
        {
        rgb[i] = 1;
        }
      }
    }
  else
    {
    rgb[0] = (values[0] - v_w_range[0]) / (v_w_range[1] - v_w_range[0]);
    // Just in case the point values are outside the whole range, which
    // is not likely to happen, but who knows.
    if (rgb[0] < 0.0)
      {
      rgb[0] = 0;
      } 
    else if (rgb[0] > 1.0) 
      {
      rgb[0] = 1;
      }
    rgb[1] = rgb[2] = rgb[0];
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
    if (id == this->GetSelectedPoint())
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

  double l = (prgb[0] + prgb[1] + prgb[2]) / 3.0;
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
int vtkKWParameterValueFunctionEditor::GetFunctionPointCanvasCoordinatesAtParameter(double parameter, int *x, int *y)
{
  if (!this->IsCreated() || !this->HasFunction())
    {
    return 0;
    }

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  *x = vtkMath::Round(parameter * factors[0]);

  double *v_w_range = this->GetWholeValueRange();
  double *v_v_range = this->GetVisibleValueRange();

  // If the value is forced to be placed at top

  if (this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionTop)
    {
    *y = vtkMath::Round((v_w_range[1] - v_v_range[1]) * factors[1]);
    }

  // If the value is forced to be placed at bottom

  else if (this->PointPositionInValueRange == 
           vtkKWParameterValueFunctionEditor::PointPositionBottom)
    {
    *y = vtkMath::Round((v_w_range[1] - v_v_range[0]) * factors[1]);
    }

  // If the value is forced to be placed at center, or is multi-dimensional, 
  // just place the point in the middle of the current value range

  else if (this->PointPositionInValueRange == 
           vtkKWParameterValueFunctionEditor::PointPositionCenter ||
           this->GetFunctionPointDimensionality() != 1)
    {
    *y = (int)floor(
      (v_w_range[1] - (v_v_range[1] + v_v_range[0]) * 0.5) * factors[1]);
    }

  // The value is mono-dimensional, use it to compute the y coord

  else
    {
    double values[
      vtkKWParameterValueFunctionEditor::MaxFunctionPointDimensionality];
    if (!this->InterpolateFunctionPointValues(parameter, values))
      {
      return 0;
      }
    *y = vtkMath::Round((v_w_range[1] - values[0]) * factors[1]);
    }
    
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetFunctionPointCanvasCoordinates(
  int id, int *x, int *y)
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

  return this->GetFunctionPointCanvasCoordinatesAtParameter(parameter, x, y);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddFunctionPointAtCanvasCoordinates(
  int x, int y, int *id)
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
      vtkKWParameterValueFunctionEditor::PointPositionCenter ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionTop ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionBottom ||
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

  return this->AddFunctionPoint(parameter, values, id);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddFunctionPointAtParameter(
  double parameter, int *id)
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

  return this->AddFunctionPoint(parameter, values, id);
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
      vtkKWParameterValueFunctionEditor::PointPositionCenter ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionTop ||
      this->PointPositionInValueRange == 
      vtkKWParameterValueFunctionEditor::PointPositionBottom ||
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
  // the point we just moved will be redrawn by
  // the call to RedrawFunctionDependentElements

  this->RedrawSinglePointDependentElements(id);

  // If we are moving the end points and we should rescale

  if (this->RescaleBetweenEndPoints && 
      (id == 0 || id == this->GetFunctionSize() - 1))
    {
    this->RescaleFunctionBetweenEndPoints(id, old_parameter);
    this->RedrawFunctionDependentElements();
    }

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
void vtkKWParameterValueFunctionEditor::CreateWidget()
{
  if (this->IsCreated())
    {
    vtkErrorMacro("widget already created " << this->GetClassName());
    return;
    }

  // Call the superclass to create the widget and set the appropriate flags

  this->Superclass::CreateWidget();

  // Create the canvas

  this->Canvas->SetParent(this);
  this->Canvas->Create();
  this->Canvas->SetHighlightThickness(0);
  this->Canvas->SetReliefToSolid();
  this->Canvas->SetBorderWidth(0);
  this->Canvas->SetHeight(this->RequestedCanvasHeight);
  this->Canvas->SetWidth(
    this->ExpandCanvasWidth ? 0 : this->RequestedCanvasWidth);

  // Both are needed, the first one in case the canvas is not visible, the
  // second because if it is visible, we want it to notify us precisely
  // when it needs re-configuration

  this->SetBinding("<Configure>", this, "ConfigureCallback");
  this->Canvas->SetBinding("<Configure>", this, "ConfigureCallback");

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
  this->ParameterRange->LabelVisibilityOff();
  this->ParameterRange->EntriesVisibilityOff();
  this->ParameterRange->SetCommand(
    this, "VisibleParameterRangeChangingCallback");
  this->ParameterRange->SetEndCommand(
    this, "VisibleParameterRangeChangedCallback");

  if (this->ParameterRangeVisibility)
    {
    this->CreateParameterRange();
    }
  else
    {
    this->ParameterRange->SetApplication(this->GetApplication());
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
  this->ValueRange->SetLabelVisibility(
    this->ParameterRange->GetLabelVisibility());
  this->ValueRange->SetEntriesVisibility(
    this->ParameterRange->GetEntriesVisibility());
  this->ValueRange->SetCommand(
    this, "VisibleValueRangeChangingCallback");
  this->ValueRange->SetEndCommand(
    this, "VisibleValueRangeChangedCallback");

  if (this->ValueRangeVisibility)
    {
    this->CreateValueRange();
    }
  else
    {
    this->ValueRange->SetApplication(this->GetApplication());
    }

  // Create the top left container
  // It will be created automatically when sub-elements will be created
  // (for ex: UserFrame or TopLeftFrame)

  // Create the top left/right frames only if we know that we are going to
  // need the, (otherwise they will be
  // create on the fly later once elements are moved)

  if (this->IsTopLeftFrameUsed())
    {
    this->CreateTopLeftFrame();
    }

  if (this->IsPointEntriesFrameUsed())
    {
    this->CreatePointEntriesFrame();
    }

  // Create the user frame

  if (this->UserFrameVisibility)
    {
    this->CreateUserFrame();
    }

  // Create the label now if it has to be shown now

  if (this->LabelVisibility)
    {
    this->CreateLabel();
    }

  // Create the range label

  if (this->ParameterRangeLabelVisibility || 
      this->ValueRangeLabelVisibility)
    {
    this->CreateRangeLabel();
    }

  // Do not create the point entries frame
  // It will be created automatically when sub-elements will be created
  // (for ex: ParameterEntry)

  // Create the parameter entry

  if (this->ParameterEntryVisibility && this->PointEntriesVisibility)
    {
    this->CreateParameterEntry();
    }

  // Create the ticks canvas

  if (this->ValueTicksVisibility)
    {
    this->CreateValueTicksCanvas();
    }

  if (this->ParameterTicksVisibility)
    {
    this->CreateParameterTicksCanvas();
    }

  // Create the guideline value canvas

  if (this->IsGuidelineValueCanvasUsed())
    {
    this->CreateGuidelineValueCanvas();
    }

  // Histogram log mode

  if (this->HistogramLogModeOptionMenuVisibility)
    {
    this->CreateHistogramLogModeOptionMenu();
    }

  // Set the bindings

  this->Bind();

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateLabel()
{
  // If we are displaying the label in the top left frame, make sure it has
  // been created. 

  if (this->GetLabelVisibility() && 
      this->LabelPosition == vtkKWWidgetWithLabel::LabelPositionDefault)
    {
    this->CreateTopLeftFrame();
    }

  if (this->HasLabel() && this->GetLabel()->IsCreated())
    {
    return;
    }

  this->Superclass::CreateLabel();
  vtkKWTkUtilities::ChangeFontWeightToBold(this->GetLabel());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateParameterRange()
{
  if (this->ParameterRange && !this->ParameterRange->IsCreated())
    {
    this->ParameterRange->SetParent(this);
    this->ParameterRange->Create();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateValueRange()
{
  if (this->ValueRange && !this->ValueRange->IsCreated())
    {
    this->ValueRange->SetParent(this);
    this->ValueRange->Create();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateRangeLabel()
{
  if ((this->ParameterRangeLabelVisibility || 
       this->ValueRangeLabelVisibility) && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault))
    {
    this->CreateTopLeftFrame();
    }

  if (this->RangeLabel && !this->RangeLabel->IsCreated())
    {
    this->RangeLabel->SetParent(this);
    this->RangeLabel->Create();
    this->RangeLabel->SetBorderWidth(0);
    this->RangeLabel->SetAnchorToWest();
    this->UpdateRangeLabel();
    this->Bind(); // in case we have bindings on the label
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreatePointEntriesFrame()
{
  if (this->PointEntriesFrame && !this->PointEntriesFrame->IsCreated())
    {
    this->PointEntriesFrame->SetParent(this);
    this->PointEntriesFrame->Create();
    }
}

//----------------------------------------------------------------------------
vtkKWEntryWithLabel* vtkKWParameterValueFunctionEditor::GetParameterEntry()
{
  if (!this->ParameterEntry)
    {
    this->ParameterEntry = vtkKWEntryWithLabel::New();
    if (this->ParameterEntryVisibility &&  
        this->PointEntriesVisibility && 
        this->IsCreated())
      {
      this->CreateParameterEntry();
      }
    }
  return this->ParameterEntry;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateParameterEntry()
{
  if (this->GetParameterEntry() && !this->ParameterEntry->IsCreated())
    {
    this->CreatePointEntriesFrame();

    // If we are displaying the entry in the top right frame, make sure it
    // has been created. 

    this->ParameterEntry->SetParent(this->PointEntriesFrame);
    this->ParameterEntry->Create();
    this->ParameterEntry->GetWidget()->SetWidth(7);
    this->ParameterEntry->GetLabel()->SetText(
      ks_("Transfer Function Editor|Parameter|P:"));

    this->UpdateParameterEntry(this->GetSelectedPoint());

    this->ParameterEntry->GetWidget()->SetCommand(
      this, "ParameterEntryCallback");
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateHistogramLogModeOptionMenu()
{
  if (this->HistogramLogModeOptionMenu && 
      !this->HistogramLogModeOptionMenu->IsCreated())
    {
    this->CreateTopLeftFrame();

    this->HistogramLogModeOptionMenu->SetParent(this->TopLeftFrame);
    this->HistogramLogModeOptionMenu->Create();
    this->HistogramLogModeOptionMenu->SetPadX(1);
    this->HistogramLogModeOptionMenu->SetPadY(1);
    this->HistogramLogModeOptionMenu->IndicatorVisibilityOff();
    this->HistogramLogModeOptionMenu->SetBalloonHelpString(
      k_("Change the histogram mode from log to linear."));

    vtkKWMenu *menu = this->HistogramLogModeOptionMenu->GetMenu();

    vtksys_stl::string img_name;

    img_name = this->HistogramLogModeOptionMenu->GetWidgetName();
    img_name += ".img0";
    vtkKWTkUtilities::UpdatePhotoFromPredefinedIcon(
      this->GetApplication(), img_name.c_str(), vtkKWIcon::IconGridLinear);
    
    int index = menu->AddRadioButton(
      ks_("Transfer Function Editor|Histogram|Linear|Lin."), 
      this, "HistogramLogModeCallback 0");
    menu->SetItemImage(index, img_name.c_str());

    img_name = this->HistogramLogModeOptionMenu->GetWidgetName();
    img_name += ".img1";
    vtkKWTkUtilities::UpdatePhotoFromPredefinedIcon(
      this->GetApplication(), img_name.c_str(), vtkKWIcon::IconGridLog);
 
    index = menu->AddRadioButton(
      ks_("Transfer Function Editor|Histogram|Logarithmic|Log."), 
      this, "HistogramLogModeCallback 1");
    menu->SetItemImage(index, img_name.c_str());

    this->UpdateHistogramLogModeOptionMenu();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateTopLeftContainer()
{
  if (this->TopLeftContainer && !this->TopLeftContainer->IsCreated())
    {
    this->TopLeftContainer->SetParent(this);
    this->TopLeftContainer->Create();
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::IsTopLeftFrameUsed()
{
  return ((this->LabelVisibility && 
           (this->LabelPosition == 
            vtkKWWidgetWithLabel::LabelPositionDefault)) ||
          ((this->ParameterRangeLabelVisibility || 
            this->ValueRangeLabelVisibility) && 
           (this->RangeLabelPosition == 
            vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault)) ||
          this->HistogramLogModeOptionMenuVisibility);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::IsPointEntriesFrameUsed()
{
  return this->ParameterEntryVisibility && this->PointEntriesVisibility;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateTopLeftFrame()
{
  if (this->TopLeftFrame && !this->TopLeftFrame->IsCreated())
    {
    this->CreateTopLeftContainer();
    this->TopLeftFrame->SetParent(this->TopLeftContainer);
    this->TopLeftFrame->Create();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateUserFrame()
{
  if (this->UserFrame && !this->UserFrame->IsCreated())
    {
    this->CreateTopLeftContainer();
    this->UserFrame->SetParent(this->TopLeftContainer);
    this->UserFrame->Create();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateValueTicksCanvas()
{
  if (this->ValueTicksCanvas && !this->ValueTicksCanvas->IsCreated())
    {
    this->ValueTicksCanvas->SetParent(this);
    this->ValueTicksCanvas->Create();
    this->ValueTicksCanvas->SetHighlightThickness(0);
    this->ValueTicksCanvas->SetReliefToSolid();
    this->ValueTicksCanvas->SetHeight(0);
    this->ValueTicksCanvas->SetBorderWidth(0);
    this->Bind(); // in case we have bindings on this canvas
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateParameterTicksCanvas()
{
  if (this->ParameterTicksCanvas && !this->ParameterTicksCanvas->IsCreated())
    {
    this->ParameterTicksCanvas->SetParent(this);
    this->ParameterTicksCanvas->Create();
    this->ParameterTicksCanvas->SetHighlightThickness(0);
    this->ParameterTicksCanvas->SetReliefToSolid();
    this->ParameterTicksCanvas->SetWidth(0);
    this->ParameterTicksCanvas->SetBorderWidth(0);
    this->ParameterTicksCanvas->SetHeight(
      VTK_KW_PVFE_TICKS_PARAMETER_CANVAS_HEIGHT);
    this->Bind(); // in case we have bindings on this canvas
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::CreateGuidelineValueCanvas()
{
  if (this->GuidelineValueCanvas && !this->GuidelineValueCanvas->IsCreated())
    {
    this->GuidelineValueCanvas->SetParent(this);
    this->GuidelineValueCanvas->Create();
    this->GuidelineValueCanvas->SetHighlightThickness(0);
    this->GuidelineValueCanvas->SetReliefToSolid();
    this->GuidelineValueCanvas->SetWidth(0);
    this->GuidelineValueCanvas->SetBorderWidth(0);
    this->GuidelineValueCanvas->SetHeight(
      VTK_KW_PVFE_GUIDELINE_VALUE_CANVAS_HEIGHT);
    this->Bind(); // in case we have bindings on this canvas
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Update()
{
  this->UpdateEnableState();

  this->UpdateRangeLabel();

  this->UpdatePointEntries(this->GetSelectedPoint());

  this->Redraw();

  this->UpdateHistogramLogModeOptionMenu();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Pack()
{
  if (!this->IsAlive())
    {
    return;
    }

  // Unpack everything

  if (this->Canvas)
    {
    this->Canvas->UnpackSiblings();
    }

  // Repack everything

  ostrstream tk_cmd;

  /*
    TLC: TopLeftContainer, contains the TopLeftFrame (TLF) and UserFrame (UF)
    TLF: TopLeftFrame, may contain the Label (L) and/or the RangeLabel (RL)...
    L:   Label, usually the title of the whole dialog
    RL:  RangeLabel, displays the current visible parameter range
    PEF: PointEntriesFrame, has the ParameterEntry (PE)
         and subclasses entries
    PE:  Parameter Entry
    PR:  Parameter Range
    VR:  Value Range
    VT:  Value Ticks
    PT:  Parameter Ticks
    GVC:  Guideline Value Canvas
    [---]: Canvas

            a b  c              d   e  f
         +------------------------------
        0|       TLC            PEF             ShowLabel: On
        1|    VT [--------------]   VR          LabelPosition: Default
        2|       PT                             RangeLabelPosition: Default
        3|       PR                   

            a b  c              d   e  f
         +------------------------------
        0|       L            RL                ShowLabel: On
        1|       TLC            PEF             LabelPosition: Top
        2|    VT [--------------]   VR          RangeLabelPosition: Top
        3|       PT 
        4|       PR

            a b  c              d   e  f
         +------------------------------
        0|       TLC                           ShowLabel: On
        1|       GVC                           if guideline values displayed
        2|  L VT [--------------]   VR PEF     LabelPosition: Left
        3|       PT                            RangeLabelPosition: Default
        4|       PR                            PointEntriesPosition: Right

            a b  c              d   e  f
         +------------------------------
        0|       TLC                           ShowLabel: On
        1|       PR                            PointEntriesPosition: Right
        2|  L VT [--------------]   VR PEF     LabelPosition: Left
        3|       PT                            RangeLabelPosition: Default
                                               ParameterRangePosition: Top
  */

  // We need a grid

  int row = 0, row_inc = 0;
  int col_a = 0, col_b = 1, col_c = 2, col_d = 3, col_e = 4, col_f = 5;
  
  // Label (L) if on top, i.e. its on own row not in the top left frame (TLF)
  // Note that we span column col_c and col_d because (L) can get quite large
  // whereas TLC is usually small, so we would end up with a too large col_c

  if (this->LabelVisibility && 
      (this->LabelPosition == 
       vtkKWWidgetWithLabel::LabelPositionTop) &&
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
           << " -stick wns -padx 0 -pady 0  -columnspan 2 -in "
           << this->GetWidgetName()
           << " -column " << col_c << " -row " << row << endl;
    row_inc = 1;
    }
  
  // RangeLabel (RL) on top, i.e. on its own row not in the top left frame(TLF)
  // Note that we span column col_c and col_d because (L) can get quite large
  // whereas TLC is usually small, so we would end up with a too large col_c
  
  if ((this->ParameterRangeLabelVisibility ||
       this->ValueRangeLabelVisibility) && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionTop) &&
      this->RangeLabel && this->RangeLabel->IsCreated())
    {
    tk_cmd << "grid " << this->RangeLabel->GetWidgetName() 
           << " -stick ens -padx 0 -pady 0 -columnspan 2 -in "
           << this->GetWidgetName() 
           << " -column " << col_c << " -row " << row << endl;
    row_inc = 1;
    }

  row += row_inc;
  
  // Top left container (TLC)

  if (this->TopLeftContainer && this->TopLeftContainer->IsCreated())
    {
    this->TopLeftContainer->UnpackChildren();
    if (this->IsTopLeftFrameUsed() || this->UserFrameVisibility)
      {
      tk_cmd << "grid " << this->TopLeftContainer->GetWidgetName() 
             << " -stick ewns -pady 1 "
             << " -column " << col_c << " -row " << row << endl;
      }
    }

  // Top left frame (TLF) and User frame (UF)
  // inside the top left container (TLC)
  
  if (this->TopLeftFrame && this->TopLeftFrame->IsCreated())
    {
    this->TopLeftFrame->UnpackChildren();
    if (this->IsTopLeftFrameUsed())
      {
      tk_cmd << "pack " << this->TopLeftFrame->GetWidgetName()
             << " -side left -fill both -padx 0 -pady 0" << endl;
      }
    }

  if (this->UserFrame && this->UserFrame->IsCreated())
    {
    tk_cmd << "pack " << this->UserFrame->GetWidgetName() 
           << " -side left -fill both -padx 0 -pady 0" << endl;
    }

  // Label (L) at default position, i.e inside top left frame (TLF)

  if (this->LabelVisibility && 
      (this->LabelPosition == 
       vtkKWWidgetWithLabel::LabelPositionDefault) &&
      this->HasLabel() && this->GetLabel()->IsCreated() &&
      this->TopLeftFrame && this->TopLeftFrame->IsCreated())
    {
    tk_cmd << "pack " << this->GetLabel()->GetWidgetName() 
           << " -side left -fill both -padx 0 -pady 0 -in " 
           << this->TopLeftFrame->GetWidgetName() << endl;
    }
  
  // Histogram log mode (in top left frame)

  if (this->HistogramLogModeOptionMenuVisibility &&
      this->HistogramLogModeOptionMenu && 
      this->HistogramLogModeOptionMenu->IsCreated())
    {
    tk_cmd << "pack " << this->HistogramLogModeOptionMenu->GetWidgetName() 
           << " -side left -fill both -padx 0" << endl;
    }
  
  // RangeLabel (RL) at default position, i.e. inside top left frame (TLF)

  if ((this->ParameterRangeLabelVisibility ||
       this->ValueRangeLabelVisibility) && 
      (this->RangeLabelPosition == 
       vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault) &&
      this->RangeLabel && this->RangeLabel->IsCreated() &&
      this->TopLeftFrame && this->TopLeftFrame->IsCreated())
    {
    tk_cmd << "pack " << this->RangeLabel->GetWidgetName() 
           << " -side left -fill both -padx 0 -pady 0 -in " 
           << this->TopLeftFrame->GetWidgetName() << endl;
    }
  
  // PointEntriesFrame (PEF) if at default position, i.e. top right

  if (this->PointEntriesFrame && this->PointEntriesFrame->IsCreated() &&
      this->PointEntriesPosition == 
      vtkKWParameterValueFunctionEditor::PointEntriesPositionDefault &&
      this->IsPointEntriesFrameUsed())
    {
    tk_cmd << "grid " << this->PointEntriesFrame->GetWidgetName() 
           << " -stick ens -pady 1"
           << " -column " << col_d << " -row " << row << endl;
    }
  
  row++;
  
  // Parameter range (PR) if at top
  
  if (this->ParameterRangeVisibility && 
      this->ParameterRange && this->ParameterRange->IsCreated() &&
      (this->ParameterRangePosition == 
       vtkKWParameterValueFunctionEditor::ParameterRangePositionTop))
    {
    tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 2"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    row++;
    }

  // Guideline Value Canvas (GVC)
  
  if (this->IsGuidelineValueCanvasUsed() && 
      this->GuidelineValueCanvas && this->GuidelineValueCanvas->IsCreated())
    {
    tk_cmd << "grid " << this->GuidelineValueCanvas->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 0"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    row++;
    }
  
  // Label (L) if at left

  if (this->LabelVisibility && 
      (this->LabelPosition == 
       vtkKWWidgetWithLabel::LabelPositionLeft) &&
      this->HasLabel() && this->GetLabel()->IsCreated())
    {
    tk_cmd << "grid " << this->GetLabel()->GetWidgetName() 
           << " -stick wns -padx 0 -pady 0 -in "
           << this->GetWidgetName() 
           << " -column " << col_a << " -row " << row << endl;
    }
  
  // Value Ticks (VT)
  
  if (this->ValueTicksVisibility && 
      this->ValueTicksCanvas && this->ValueTicksCanvas->IsCreated())
    {
    tk_cmd << "grid " << this->ValueTicksCanvas->GetWidgetName() 
           << " -sticky ns -padx 0 -pady 0 "
           << " -column " << col_b << " -row " << row << endl;
    }
  
  // Canvas ([------])
  
  if (this->CanvasVisibility && 
      this->Canvas && this->Canvas->IsCreated())
    {
    tk_cmd << "grid " << this->Canvas->GetWidgetName() 
           << " -sticky news -padx 0 -pady 0 "
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    }
  
  // Value range (VR)
  
  if (this->ValueRangeVisibility && 
      this->ValueRange && this->ValueRange->IsCreated())
    {
    tk_cmd << "grid " << this->ValueRange->GetWidgetName() 
           << " -sticky ns -padx 2 -pady 0 "
           << " -column " << col_e << " -row " << row << endl;
    }
  
  // PointEntriesFrame (PEF) if at right of canvas

  if (this->PointEntriesFrame && this->PointEntriesFrame->IsCreated() &&
      this->PointEntriesPosition == 
      vtkKWParameterValueFunctionEditor::PointEntriesPositionRight &&
      this->IsPointEntriesFrameUsed())
    {
    tk_cmd << "grid " << this->PointEntriesFrame->GetWidgetName() 
           << " -sticky wns -padx 2 -pady 0 -column " << col_f 
           << " -row " << row << endl;
    }
  
  tk_cmd << "grid rowconfigure " 
         << this->GetWidgetName() << " " << row << " -weight 1" << endl;
  
  row++;
    
  // Parameter Ticks (PT)
  
  if (this->ParameterTicksVisibility && 
      this->ParameterTicksFormat &&
      this->ParameterTicksCanvas && this->ParameterTicksCanvas->IsCreated())
    {
    tk_cmd << "grid " << this->ParameterTicksCanvas->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 0"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    row++;
    }
  
  // Parameter range (PR)
  
  if (this->ParameterRangeVisibility && 
      this->ParameterRange && this->ParameterRange->IsCreated() &&
      (this->ParameterRangePosition == 
       vtkKWParameterValueFunctionEditor::ParameterRangePositionBottom))
    {
    tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
           << " -sticky ew -padx 0 -pady 2"
           << " -columnspan 2 -column " << col_c << " -row " << row << endl;
    }

  this->PackPointEntries();
  
  // Make sure it will resize properly
  
  tk_cmd << "grid columnconfigure " 
         << this->GetWidgetName() << " " << col_c << " -weight 1" << endl;
  
  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::PackPointEntries()
{
  if (!this->IsCreated())
    {
    return;
    }

  // Unpack everything in the point entries frame

  if (this->PointEntriesFrame)
    {
    this->PointEntriesFrame->UnpackChildren();
    }

  // Repack everything

  ostrstream tk_cmd;

  // ParameterEntry (PE)
  
  if (this->HasSelection() &&
      this->ParameterEntryVisibility && 
      this->PointEntriesVisibility &&
      this->ParameterEntry && this->ParameterEntry->IsCreated())
    {
    tk_cmd << "pack " << this->ParameterEntry->GetWidgetName() 
           << " -side left -padx 2 " << endl;
    }
  
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

  // Make sure we are clear

  this->UnBind();

  ostrstream tk_cmd;

  // Canvas

  if (this->Canvas && this->Canvas->IsAlive())
    {
    const char *canv = this->Canvas->GetWidgetName();

    // Mouse motion

    this->Canvas->SetBinding(
      "<Any-ButtonPress>", this, "StartInteractionCallback %x %y");

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag
           << " <B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 0}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag
           << " <B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 0}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag
           << " <Shift-B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 1}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag
           << " <Shift-B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 1}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag 
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " EndInteractionCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag 
           << " <ButtonRelease-1> {" << this->GetTclName() 
           << " EndInteractionCallback %%x %%y}" << endl;

    // Double click on point

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag
           << " <Double-1> {" << this->GetTclName() 
           << " DoubleClickOnPointCallback %%x %%y}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag
           << " <Double-1> {" << this->GetTclName() 
           << " DoubleClickOnPointCallback %%x %%y}" << endl;

    // Parameter Cursor

    if (this->ParameterCursorInteractionStyle & 
        vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleDragWithLeftButton)
      {
      tk_cmd << canv << " bind " 
             << vtkKWParameterValueFunctionEditor::ParameterCursorTag
             << " <ButtonPress-1> {" << this->GetTclName() 
             << " ParameterCursorStartInteractionCallback %%x}" << endl;
      tk_cmd << canv << " bind " 
             << vtkKWParameterValueFunctionEditor::ParameterCursorTag
             << " <ButtonRelease-1> {" << this->GetTclName() 
             << " ParameterCursorEndInteractionCallback}" << endl;
      tk_cmd << canv << " bind " 
             << vtkKWParameterValueFunctionEditor::ParameterCursorTag
             << " <B1-Motion> {" << this->GetTclName() 
             << " ParameterCursorMoveCallback %%x}" << endl;
      }

    if (this->ParameterCursorInteractionStyle & 
        vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleSetWithControlLeftButton)
      {
      tk_cmd << "bind " << canv
             << " <Control-ButtonPress-1> {" 
             << this->GetTclName() 
             << " ParameterCursorStartInteractionCallback %%x ; " 
             << this->GetTclName() 
             << " ParameterCursorMoveCallback %%x}" << endl;
      tk_cmd << "bind " << canv
             << " <Control-ButtonRelease-1> {" << this->GetTclName() 
             << " ParameterCursorEndInteractionCallback}" << endl;
      tk_cmd << "bind " << canv
             << " <Control-B1-Motion> {" << this->GetTclName() 
             << " ParameterCursorMoveCallback %%x}" << endl;
      }

    if (this->ParameterCursorInteractionStyle & 
        vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleSetWithRighButton)
      {
      tk_cmd << "bind " << canv
             << " <ButtonPress-3> {"
             << this->GetTclName() 
             << " ParameterCursorStartInteractionCallback %%x ; " 
             << this->GetTclName() 
             << " ParameterCursorMoveCallback %%x}" << endl;
      tk_cmd << "bind " << canv
             << " <ButtonRelease-3> {" << this->GetTclName() 
             << " ParameterCursorEndInteractionCallback}" << endl;
      tk_cmd << "bind " << canv
             << " <B3-Motion> {" << this->GetTclName() 
             << " ParameterCursorMoveCallback %%x}" << endl;
      }

    // Key bindings

    vtkKWWidget* to_focus[] = { 
      this, 
      this->Canvas, 
      this->RangeLabel, 
      this->ValueTicksCanvas, 
      this->ParameterTicksCanvas, 
      this->GuidelineValueCanvas
    };

    // Force the focus to the canvas to make sure key navigation works

    size_t i;
    for (i = 0; i < sizeof(to_focus) / sizeof(to_focus[0]); i++)
      {
      if (to_focus[i] && to_focus[i]->IsCreated())
        {
        tk_cmd << "bind " <<  to_focus[i]->GetWidgetName()
               << " <Button> {+" << this->Canvas->GetTclName() 
               << " Focus}" << endl;
        }
      }

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

    this->Canvas->RemoveBinding("<ButtonPress-1>");

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag 
           << " <Shift-B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag 
           << " <Shift-B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag 
           << " <ButtonRelease-1> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag 
           << " <ButtonRelease-1> {}" << endl;

    // Double click on point

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTag
           << " <Double-1> {}" << endl;

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::PointTextTag
           << " <Double-1> {}" << endl;


    // Parameter Cursor

    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::ParameterCursorTag
           << " <ButtonPress-1> {}"  << endl;
    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::ParameterCursorTag
           << " <ButtonRelease-1> {}" << endl;
    tk_cmd << canv << " bind " 
           << vtkKWParameterValueFunctionEditor::ParameterCursorTag
           << " <B1-Motion> {}" << endl;

    tk_cmd << "bind " << canv 
           << " <Control-ButtonPress-1> {}" << endl;
    tk_cmd << "bind " << canv
           << " <Control-ButtonRelease-1> {}" << endl;
    tk_cmd << "bind " << canv
           << " <Control-B1-Motion> {}" << endl;

    tk_cmd << "bind " << canv
           << " <ButtonPress-3> {}" << endl;
    tk_cmd << "bind " << canv
           << " <ButtonRelease-3> {}" << endl;
    tk_cmd << "bind " << canv
           << " <B3-Motion> {}" << endl;

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
void vtkKWParameterValueFunctionEditor::GetWholeParameterRange(
  double &r0, double &r1)
{ 
  r0 = this->GetWholeParameterRange()[0]; 
  r1 = this->GetWholeParameterRange()[1]; 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetWholeParameterRange(double range[2])
{ 
  this->GetWholeParameterRange(range[0], range[1]); 
};

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeParameterRange(double range[2])
{ 
  this->SetWholeParameterRange(range[0], range[1]); 
};

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeParameterRange(
  double r0, double r1)
{
  // First get the current relative visible range

  double range[2];
  this->GetRelativeVisibleParameterRange(range);
  if (range[0] == range[1]) // avoid getting stuck
    {
    range[0] = 0.0;
    range[1] = 1.0;
    }

  // Then set the whole range

  this->ParameterRange->SetWholeRange(r0, r1);

  // We update the label, but we do not redraw yet as the visible range
  // might just be several order of magnitude smaller or bigger than
  // the new whole parameter range, and extreme zoom could be
  // problematic for function that need sampling...
  // Instead, check the last redraw time, and if setting the
  // visible range below did not trigger a redraw, let's do it ourself.

  this->UpdateRangeLabel();

  //this->Redraw();
  unsigned long old_time = this->LastRedrawFunctionTime;

  // Reset the visible range but keep the same relative range compared
  // to the whole range

  this->SetRelativeVisibleParameterRange(range);

  if (old_time == this->LastRedrawFunctionTime)
    {
    this->Redraw();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeParameterRangeToFunctionRange()
{
  double start, end;
  if (this->GetFunctionSize() >= 2)
    {
    if (this->GetFunctionPointParameter(0, &start) &&
        this->GetFunctionPointParameter(this->GetFunctionSize() - 1, &end))
      {
      this->SetWholeParameterRange(start, end);
      }
    }
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetVisibleParameterRange()
{
  return this->ParameterRange->GetRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetVisibleParameterRange(
  double &r0, double &r1)
{ 
  r0 = this->GetVisibleParameterRange()[0]; 
  r1 = this->GetVisibleParameterRange()[1]; 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetVisibleParameterRange(
  double range[2])
{ 
  this->GetVisibleParameterRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleParameterRange(
  double range[2]) 
{ 
  this->SetVisibleParameterRange(range[0], range[1]); 
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
void vtkKWParameterValueFunctionEditor::SetVisibleParameterRangeToWholeParameterRange()
{
  this->SetVisibleParameterRange(this->GetWholeParameterRange());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleParameterRange(
  double &r0, double &r1)
{
  this->ParameterRange->GetRelativeRange(r0, r1);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleParameterRange(
  double range[2])
{ 
  this->GetRelativeVisibleParameterRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRelativeVisibleParameterRange(
  double range[2]) 
{ 
  this->SetRelativeVisibleParameterRange(range[0], range[1]); 
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
void vtkKWParameterValueFunctionEditor::SetParameterRangeVisibility(int arg)
{
  if (this->ParameterRangeVisibility == arg)
    {
    return;
    }

  this->ParameterRangeVisibility = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ParameterRangeVisibility && this->IsCreated())
    {
    this->CreateParameterRange();
    }

  this->Modified();

  this->Pack();
  this->UpdateRangeLabel();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterRangePosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::ParameterRangePositionTop)
    {
    arg = vtkKWParameterValueFunctionEditor::ParameterRangePositionTop;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::ParameterRangePositionBottom)
    {
    arg = vtkKWParameterValueFunctionEditor::ParameterRangePositionBottom;
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
void vtkKWParameterValueFunctionEditor::SetParameterRangePositionToTop()
{ 
  this->SetParameterRangePosition(
    vtkKWParameterValueFunctionEditor::ParameterRangePositionTop); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterRangePositionToBottom()
{ 
  this->SetParameterRangePosition(
    vtkKWParameterValueFunctionEditor::ParameterRangePositionBottom); 
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetWholeValueRange()
{
  return this->ValueRange->GetWholeRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetWholeValueRange(
  double &r0, double &r1)
{ 
  r0 = this->GetWholeValueRange()[0]; 
  r1 = this->GetWholeValueRange()[1]; 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetWholeValueRange(double range[2])
{ 
  this->GetWholeValueRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeValueRange(double range[2]) 
{ 
  this->SetWholeValueRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeValueRange(double r0, double r1)
{
  // First get the current relative visible range

  double range[2];
  this->GetRelativeVisibleValueRange(range);
  if (range[0] == range[1]) // avoid getting stuck
    {
    range[0] = 0.0;
    range[1] = 1.0;
    }

  // Then set the whole range

  this->ValueRange->SetWholeRange(r0, r1);

  // We update the label, but we do not redraw yet as the visible range
  // might just be several order of magnitude smaller or bigger than
  // the new whole value range, and extreme zoom could be
  // problematic for function that need sampling...
  // Instead, check the last redraw time, and if setting the
  // visible range below did not trigger a redraw, let's do it ourself.

  this->UpdateRangeLabel();

  //this->Redraw();
  unsigned long old_time = this->LastRedrawFunctionTime;

  // Reset the visible range but keep the same relative range compared
  // to the whole range

  this->SetRelativeVisibleValueRange(range);

  if (old_time == this->LastRedrawFunctionTime)
    {
    this->Redraw();
    }
}

//----------------------------------------------------------------------------
double* vtkKWParameterValueFunctionEditor::GetVisibleValueRange()
{
  return this->ValueRange->GetRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetVisibleValueRange(
  double &r0, double &r1)
{ 
  r0 = this->GetVisibleValueRange()[0]; 
  r1 = this->GetVisibleValueRange()[1]; 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetVisibleValueRange(double range[2])
{ 
  this->GetVisibleValueRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleValueRange(double range[2]) 
{ 
  this->SetVisibleValueRange(range[0], range[1]); 
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
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleValueRange(
  double range[2])
{ 
  this->GetRelativeVisibleValueRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRelativeVisibleValueRange(
  double range[2]) 
{ 
  this->SetRelativeVisibleValueRange(range[0], range[1]); 
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
void vtkKWParameterValueFunctionEditor::SetValueRangeVisibility(int arg)
{
  if (this->ValueRangeVisibility == arg)
    {
    return;
    }

  this->ValueRangeVisibility = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ValueRangeVisibility && this->IsCreated())
    {
    this->CreateValueRange();
    }

  this->Modified();

  this->Pack();
  this->UpdateRangeLabel();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointPositionInValueRange(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointPositionValue)
    {
    arg = vtkKWParameterValueFunctionEditor::PointPositionValue;
    }
  else if (arg > vtkKWParameterValueFunctionEditor::PointPositionCenter)
    {
    arg = vtkKWParameterValueFunctionEditor::PointPositionCenter;
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
void vtkKWParameterValueFunctionEditor::SetPointPositionInValueRangeToValue()
{ 
  this->SetPointPositionInValueRange(
    vtkKWParameterValueFunctionEditor::PointPositionValue); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointPositionInValueRangeToTop()
{ 
  this->SetPointPositionInValueRange(
    vtkKWParameterValueFunctionEditor::PointPositionTop); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointPositionInValueRangeToBottom()
{ 
  this->SetPointPositionInValueRange(
    vtkKWParameterValueFunctionEditor::PointPositionBottom); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointPositionInValueRangeToCenter()
{ 
  this->SetPointPositionInValueRange(
    vtkKWParameterValueFunctionEditor::PointPositionCenter); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetLabelPosition(int arg)
{
  if (arg != vtkKWParameterValueFunctionEditor::LabelPositionDefault &&
      arg != vtkKWParameterValueFunctionEditor::LabelPositionTop &&
      arg != vtkKWParameterValueFunctionEditor::LabelPositionLeft)
    {
    arg = vtkKWParameterValueFunctionEditor::LabelPositionDefault;
    }

  if (this->LabelPosition == arg)
    {
    return;
    }

  this->LabelPosition = arg;

  if (this->GetLabelVisibility() && this->IsCreated())
    {
    this->CreateLabel();
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterRangeLabelVisibility(int arg)
{
  if (this->ParameterRangeLabelVisibility == arg)
    {
    return;
    }

  this->ParameterRangeLabelVisibility = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ParameterRangeLabelVisibility && this->IsCreated())
    {
    this->CreateRangeLabel();
    }

  this->UpdateRangeLabel();

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetValueRangeLabelVisibility(int arg)
{
  if (this->ValueRangeLabelVisibility == arg)
    {
    return;
    }

  this->ValueRangeLabelVisibility = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if (this->ValueRangeLabelVisibility && this->IsCreated())
    {
    this->CreateRangeLabel();
    }

  this->UpdateRangeLabel();

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRangeLabelPosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::RangeLabelPositionTop)
    {
    arg = vtkKWParameterValueFunctionEditor::RangeLabelPositionTop;
    }

  if (this->RangeLabelPosition == arg)
    {
    return;
    }

  this->RangeLabelPosition = arg;

  // Make sure that if the range has to be shown, we create it on the fly if
  // needed

  if ((this->ParameterRangeLabelVisibility ||
       this->ValueRangeLabelVisibility) && this->IsCreated())
    {
    this->CreateRangeLabel();
    }

  this->UpdateRangeLabel();

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRangeLabelPositionToDefault()
{ 
  this->SetRangeLabelPosition(
    vtkKWParameterValueFunctionEditor::RangeLabelPositionDefault); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRangeLabelPositionToTop()
{ 
  this->SetRangeLabelPosition(
    vtkKWParameterValueFunctionEditor::RangeLabelPositionTop); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointEntriesPosition(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointEntriesPositionDefault)
    {
    arg = vtkKWParameterValueFunctionEditor::PointEntriesPositionDefault;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::PointEntriesPositionRight)
    {
    arg = vtkKWParameterValueFunctionEditor::PointEntriesPositionRight;
    }

  if (this->PointEntriesPosition == arg)
    {
    return;
    }

  this->PointEntriesPosition = arg;

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointEntriesPositionToDefault()
{ 
  this->SetPointEntriesPosition(
    vtkKWParameterValueFunctionEditor::PointEntriesPositionDefault); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointEntriesPositionToRight()
{ 
  this->SetPointEntriesPosition(
    vtkKWParameterValueFunctionEditor::PointEntriesPositionRight); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointEntriesVisibility(int arg)
{
  if (this->PointEntriesVisibility == arg)
    {
    return;
    }

  this->PointEntriesVisibility = arg;

  this->Modified();

  this->Pack();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterEntryVisibility(int arg)
{
  if (this->ParameterEntryVisibility == arg)
    {
    return;
    }

  this->ParameterEntryVisibility = arg;

  // Make sure that if the entry has to be shown, we create it on the fly if
  // needed, including all dependents widgets (like its container)

  if (this->ParameterEntryVisibility  && 
      this->PointEntriesVisibility && 
      this->IsCreated())
    {
    this->CreateParameterEntry();
    }

  this->UpdateParameterEntry(this->GetSelectedPoint());

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
  
  this->UpdateParameterEntry(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetUserFrameVisibility(int arg)
{
  if (this->UserFrameVisibility == arg)
    {
    return;
    }

  this->UserFrameVisibility = arg;

  // Make sure that if the frame has to be shown, we create it on the fly if
  // needed

  if (this->UserFrameVisibility && this->IsCreated())
    {
    this->CreateUserFrame();
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasHeight(int arg)
{
  if (this->RequestedCanvasHeight == arg || arg < VTK_KW_PVFE_CANVAS_HEIGHT_MIN)
    {
    return;
    }

  this->RequestedCanvasHeight = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetCanvasHeight()
{
  return this->RequestedCanvasHeight;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasWidth(int arg)
{
  if (this->RequestedCanvasWidth == arg || arg < VTK_KW_PVFE_CANVAS_WIDTH_MIN)
    {
    return;
    }

  this->RequestedCanvasWidth = arg;

  this->Modified();

  this->Redraw();
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetCanvasWidth()
{
  return this->RequestedCanvasWidth;
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
void vtkKWParameterValueFunctionEditor::SetCanvasVisibility(int arg)
{
  if (this->CanvasVisibility == arg)
    {
    return;
    }

  this->CanvasVisibility = arg;

  this->Modified();

  this->Pack();
  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionLineVisibility(int arg)
{
  if (this->FunctionLineVisibility == arg)
    {
    return;
    }

  this->FunctionLineVisibility = arg;

  this->Modified();

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
void vtkKWParameterValueFunctionEditor::SetFunctionLineStyleToSolid()
{ 
  this->SetFunctionLineStyle(
    vtkKWParameterValueFunctionEditor::LineStyleSolid); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionLineStyleToDash()
{ 
  this->SetFunctionLineStyle(
    vtkKWParameterValueFunctionEditor::LineStyleDash); 
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

  this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::PointTag);

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToDisc()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleDisc); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToCursorDown()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorDown); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToCursorUp()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorUp); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToCursorLeft()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorLeft); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToCursorRight()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleCursorRight); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToRectangle()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleRectangle); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointStyleToDefault()
{ 
  this->SetPointStyle(
    vtkKWParameterValueFunctionEditor::PointStyleDefault); 
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

  this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::PointTag);

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

  this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::PointTag);

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasOutlineVisibility(int arg)
{
  if (this->CanvasOutlineVisibility == arg)
    {
    return;
    }

  this->CanvasOutlineVisibility = arg;

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

  this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::FrameForegroundTag);

  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasBackgroundVisibility(int arg)
{
  if (this->CanvasBackgroundVisibility == arg)
    {
    return;
    }

  this->CanvasBackgroundVisibility = arg;

  this->Modified();

  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterCursorVisibility(int arg)
{
  if (this->ParameterCursorVisibility == arg)
    {
    return;
    }

  this->ParameterCursorVisibility = arg;

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
void vtkKWParameterValueFunctionEditor::SetParameterCursorInteractionStyle(
  int arg)
{
  if (arg < 
      vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleNone)
    {
    arg = 
      vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleNone;
    }
  else 
    if (arg >
        vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleAll)
    {
    arg =vtkKWParameterValueFunctionEditor::ParameterCursorInteractionStyleAll;
    }

  if (this->ParameterCursorInteractionStyle == arg)
    {
    return;
    }

  this->ParameterCursorInteractionStyle = arg;

  this->Modified();

  if (this->GetEnabled())
    {
    this->Bind();
    }
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::IsGuidelineValueCanvasUsed()
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterTicksVisibility(int arg)
{
  if (this->ParameterTicksVisibility == arg)
    {
    return;
    }

  this->ParameterTicksVisibility = arg;

  this->Modified();

  if (this->ParameterTicksVisibility && this->IsCreated())
    {
    this->CreateParameterTicksCanvas();
    }

  this->RedrawRangeTicks();
  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetValueTicksVisibility(int arg)
{
  if (this->ValueTicksVisibility == arg)
    {
    return;
    }

  this->ValueTicksVisibility = arg;

  this->Modified();

  if (this->ValueTicksVisibility && this->IsCreated())
    {
    this->CreateValueTicksCanvas();
    }

  this->RedrawRangeTicks();
  this->Pack();
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
  this->SetPointRadiusX(arg);
  this->SetPointRadiusY(arg);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointRadiusX(int arg)
{
  if (this->PointRadiusX == arg || arg < VTK_KW_PVFE_POINT_RADIUS_MIN)
    {
    return;
    }

  this->PointRadiusX = arg;

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
void vtkKWParameterValueFunctionEditor::SetPointRadiusY(int arg)
{
  if (this->PointRadiusY == arg || arg < VTK_KW_PVFE_POINT_RADIUS_MIN)
    {
    return;
    }

  this->PointRadiusY = arg;

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
    this->RedrawPoint(this->GetSelectedPoint());
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointText(const char *arg)
{
  if (this->SelectedPointText == NULL && arg == NULL) 
    { 
    return;
    }

  if (this->SelectedPointText && arg && 
      (!strcmp(this->SelectedPointText, arg))) 
    {
    return;
    }

  if (this->SelectedPointText) 
    { 
    delete [] this->SelectedPointText; 
    }

  if (arg)
    {
    this->SelectedPointText = new char[strlen(arg) + 1];
    strcpy(this->SelectedPointText, arg);
    }
  else
    {
    this->SelectedPointText = NULL;
    }

  this->Modified();
  
  this->RedrawPoint(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointTextToInt(int val)
{
  char buffer[256];
  sprintf(buffer, "%d", val);
  this->SetSelectedPointText(buffer);
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

  if (this->ParameterTicksVisibility || this->ValueTicksVisibility)
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

  this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::ParameterTicksTag);
  if (this->ParameterTicksCanvas->IsCreated())
    {
    this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::ParameterTicksTag,
                          this->ParameterTicksCanvas->GetWidgetName());
    }

  if (this->ParameterTicksVisibility || this->ValueTicksVisibility)
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
  
  if (this->ParameterTicksVisibility)
    {
    this->RedrawRangeTicks();
    this->Pack();
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

  this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::ValueTicksTag);
  if (this->ValueTicksCanvas->IsCreated())
    {
    this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::ValueTicksTag, 
                          this->ValueTicksCanvas->GetWidgetName());
    }

  if (this->ParameterTicksVisibility || this->ValueTicksVisibility)
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
  
  if (this->ValueTicksVisibility)
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

  // The previous call does not call RedrawSizeDependentElements because
  // technically the canvas size did not change, just the margin. Force
  // a redraw.

  this->RedrawSizeDependentElements();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToNone()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginNone); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToLeftSide()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginLeftSide); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToRightSide()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginRightSide); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToHorizontalSides()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginHorizontalSides); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToTopSide()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginTopSide); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToBottomSide()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginBottomSide); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToVerticalSides()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginVerticalSides); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointMarginToCanvasToAllSides()
{ 
  this->SetPointMarginToCanvas(
    vtkKWParameterValueFunctionEditor::PointMarginAllSides); 
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
  this->RedrawHistogram(); // for the background of the histogram
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFrameBackgroundColor(double rgb[3])
{ 
  this->SetFrameBackgroundColor(rgb[0], rgb[1], rgb[2]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetBackgroundColor(double r, double g, double b)
{
  this->Superclass::SetBackgroundColor(r, g, b);
  if (this->Canvas)
    {
    this->Canvas->SetBackgroundColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetBackgroundColor(double rgb[3])
{ 
  this->Superclass::SetBackgroundColor(rgb); 
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
void vtkKWParameterValueFunctionEditor::SetHistogramColor(double rgb[3])
{ 
  this->SetHistogramColor(rgb[0], rgb[1], rgb[2]); 
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
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogramColor(
  double rgb[3])
{ 
  this->SetSecondaryHistogramColor(rgb[0], rgb[1], rgb[2]); 
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
void vtkKWParameterValueFunctionEditor::SetParameterCursorColor(double rgb[3])
{ 
  this->SetParameterCursorColor(rgb[0], rgb[1], rgb[2]); 
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
void vtkKWParameterValueFunctionEditor::SetPointColor(double rgb[3])
{ 
  this->SetPointColor(rgb[0], rgb[1], rgb[2]); 
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

  this->RedrawPoint(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointColor(double rgb[3])
{ 
  this->SetSelectedPointColor(rgb[0], rgb[1], rgb[2]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointColorInInteraction(
  double r, double g, double b)
{
  if ((r == this->SelectedPointColorInInteraction[0] &&
       g == this->SelectedPointColorInInteraction[1] &&
       b == this->SelectedPointColorInInteraction[2]) ||
      r < 0.0 || r > 1.0 ||
      g < 0.0 || g > 1.0 ||
      b < 0.0 || b > 1.0)
    {
    return;
    }

  this->SelectedPointColorInInteraction[0] = r;
  this->SelectedPointColorInInteraction[1] = g;
  this->SelectedPointColorInInteraction[2] = b;

  this->Modified();

  this->RedrawPoint(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointColorInInteraction(double rgb[3])
{ 
  this->SetSelectedPointColorInInteraction(rgb[0], rgb[1], rgb[2]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointColorStyle(int arg)
{
  if (arg < vtkKWParameterValueFunctionEditor::PointColorStyleFill)
    {
    arg = vtkKWParameterValueFunctionEditor::PointColorStyleFill;
    }
  else if (arg > 
           vtkKWParameterValueFunctionEditor::PointColorStyleOutline)
    {
    arg = vtkKWParameterValueFunctionEditor::PointColorStyleOutline;
    }

  if (this->PointColorStyle == arg)
    {
    return;
    }

  this->PointColorStyle = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointColorStyleToFill()
{ 
  this->SetPointColorStyle(
    vtkKWParameterValueFunctionEditor::PointColorStyleFill); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointColorStyleToOutline()
{ 
  this->SetPointColorStyle(
    vtkKWParameterValueFunctionEditor::PointColorStyleOutline); 
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
void vtkKWParameterValueFunctionEditor::SetPointTextColor(double rgb[3])
{ 
  this->SetPointTextColor(rgb[0], rgb[1], rgb[2]); 
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

  this->RedrawPoint(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointTextColor(
  double rgb[3])
{ 
  this->SetSelectedPointTextColor(rgb[0], rgb[1], rgb[2]); 
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
void vtkKWParameterValueFunctionEditor::SetHistogramPolyLineWidth(int arg)
{
  if (this->HistogramPolyLineWidth == arg)
    {
    return;
    }

  this->HistogramPolyLineWidth = arg;

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

  this->CanvasRemoveTag(
    vtkKWParameterValueFunctionEditor::HistogramTag);
  this->CanvasRemoveTag(
    vtkKWParameterValueFunctionEditor::SecondaryHistogramTag);

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramStyleToBars()
{ 
  this->SetHistogramStyle(
    vtkKWParameterValueFunctionEditor::HistogramStyleBars); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramStyleToDots()
{ 
  this->SetHistogramStyle(
    vtkKWParameterValueFunctionEditor::HistogramStyleDots); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramStyleToPolyLine()
{ 
  this->SetHistogramStyle(
    vtkKWParameterValueFunctionEditor::HistogramStylePolyLine); 
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

  this->CanvasRemoveTag(
    vtkKWParameterValueFunctionEditor::HistogramTag);
  this->CanvasRemoveTag(
    vtkKWParameterValueFunctionEditor::SecondaryHistogramTag);

  this->RedrawHistogram();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogramStyleToBars()
{ 
  this->SetSecondaryHistogramStyle(
    vtkKWParameterValueFunctionEditor::HistogramStyleBars); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogramStyleToDots()
{ 
  this->SetSecondaryHistogramStyle(
    vtkKWParameterValueFunctionEditor::HistogramStyleDots); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogramStyleToPolyLine()
{ 
  this->SetSecondaryHistogramStyle(
    vtkKWParameterValueFunctionEditor::HistogramStylePolyLine); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::DisplayHistogramOnly()
{
  this->ExpandCanvasWidthOff();
  this->LabelVisibilityOff();
  this->ValueRangeVisibilityOff();
  this->ValueRangeLabelVisibilityOff();
  this->ParameterRangeVisibilityOff();
  this->ParameterRangeLabelVisibilityOff();
  this->SetPointMarginToCanvasToNone();
  this->FunctionLineVisibilityOff();
  this->PointVisibilityOff();
  this->PointGuidelineVisibilityOff();
  this->PointEntriesVisibilityOff();
  this->ParameterEntryVisibilityOff();

  double *hist_range = 
    this->Histogram ? this->Histogram->GetRange() : NULL;
  double *hist2_range = 
    this->SecondaryHistogram ? this->SecondaryHistogram->GetRange() : NULL;
  if (hist_range && hist2_range)
    {
    if (hist2_range[0] < hist_range[0])
      {
      hist_range[0] = hist2_range[0];
      }
    if (hist2_range[1] > hist_range[1])
      {
      hist_range[1] = hist2_range[1];
      }
    }
  if (hist_range || hist2_range)
    {
    this->SetWholeParameterRange(hist_range ? hist_range : hist2_range);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointVisibility(int arg)
{
  if (this->PointVisibility == arg)
    {
    return;
    }

  this->PointVisibility = arg;

  this->Modified();

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointIndexVisibility(int arg)
{
  if (this->PointIndexVisibility == arg)
    {
    return;
    }

  this->PointIndexVisibility = arg;

  this->Modified();

  this->RedrawFunction();

  // Since the point label can show the point index, update it too

  this->UpdatePointEntries(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointIndexVisibility(int arg)
{
  if (this->SelectedPointIndexVisibility == arg)
    {
    return;
    }

  this->SelectedPointIndexVisibility = arg;

  this->Modified();

  this->RedrawPoint(this->GetSelectedPoint());

  // Since the point label can show the point index, update it too

  this->UpdatePointEntries(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointGuidelineVisibility(int arg)
{
  if (this->PointGuidelineVisibility == arg)
    {
    return;
    }

  this->PointGuidelineVisibility = arg;

  this->Modified();

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
void vtkKWParameterValueFunctionEditor::SetHistogramLogModeOptionMenuVisibility(int arg)
{
  if (this->HistogramLogModeOptionMenuVisibility == arg)
    {
    return;
    }

  this->HistogramLogModeOptionMenuVisibility = arg;

  // Make sure that if the button has to be shown, we create it on the fly if
  // needed

  if (this->HistogramLogModeOptionMenuVisibility && this->IsCreated())
    {
    this->CreateHistogramLogModeOptionMenu();
    }

  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeObjectMethodCommand(
  const char *command)
{
  if (!this->DisableCommands)
    {
    this->Superclass::InvokeObjectMethodCommand(command);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointCommand(
  const char *command, int id, const char *extra)
{
  if (command && *command && !this->DisableCommands && 
      this->HasFunction() && id >= 0 && id < this->GetFunctionSize())
    {
    this->Script("%s %d %s", command, id, (extra ? extra : ""));
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogramLogModeChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->HistogramLogModeChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeHistogramLogModeChangedCommand(
  int mode)
{
  if (this->HistogramLogModeChangedCommand && 
      *this->HistogramLogModeChangedCommand && 
      this->GetApplication())
    {
    this->Script("%s %d", this->HistogramLogModeChangedCommand, mode);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointAddedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointAddedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointAddedCommand(int id)
{
  this->InvokePointCommand(this->PointAddedCommand, id);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::PointAddedEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointChangingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointChangingCommand(int id)
{
  this->InvokePointCommand(this->PointChangingCommand, id);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::PointChangingEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokePointChangedCommand(int id)
{
  this->InvokePointCommand(this->PointChangedCommand, id);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::PointChangedEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetDoubleClickOnPointCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->DoubleClickOnPointCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeDoubleClickOnPointCommand(int id)
{
  this->InvokePointCommand(this->DoubleClickOnPointCommand, id);

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::DoubleClickOnPointEvent, &id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointRemovedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->PointRemovedCommand, object, method);
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
void vtkKWParameterValueFunctionEditor::SetSelectionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->SelectionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeSelectionChangedCommand()
{
  this->InvokeObjectMethodCommand(this->SelectionChangedCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::SelectionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->FunctionChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeFunctionChangedCommand()
{
  this->InvokeObjectMethodCommand(this->FunctionChangedCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::FunctionChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionChangingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->FunctionChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeFunctionChangingCommand()
{
  this->InvokeObjectMethodCommand(this->FunctionChangingCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::FunctionChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetFunctionStartChangingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(&this->FunctionStartChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeFunctionStartChangingCommand()
{
  this->InvokeObjectMethodCommand(this->FunctionStartChangingCommand);

  this->InvokeEvent(vtkKWParameterValueFunctionEditor::FunctionStartChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleRangeChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VisibleRangeChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeVisibleRangeChangedCommand()
{
  this->InvokeObjectMethodCommand(this->VisibleRangeChangedCommand);

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleRangeChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleRangeChangingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->VisibleRangeChangingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeVisibleRangeChangingCommand()
{
  this->InvokeObjectMethodCommand(this->VisibleRangeChangingCommand);

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleRangeChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterCursorMovingCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ParameterCursorMovingCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeParameterCursorMovingCommand(
  double pos)
{
  this->InvokeObjectMethodCommand(this->ParameterCursorMovingCommand);

  if (this->ParameterCursorMovingCommand && 
      *this->ParameterCursorMovingCommand && 
      this->GetApplication())
    {
    this->Script("%s %lf", this->ParameterCursorMovingCommand, pos);
    }

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::ParameterCursorMovingEvent, &pos);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetParameterCursorMovedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ParameterCursorMovedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::InvokeParameterCursorMovedCommand(
  double pos)
{
  if (this->ParameterCursorMovedCommand && 
      *this->ParameterCursorMovedCommand && 
      this->GetApplication())
    {
    this->Script("%s %lf", this->ParameterCursorMovedCommand, pos);
    }

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::ParameterCursorMovedEvent, &pos);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Canvas);
  this->PropagateEnableState(this->ParameterRange);
  this->PropagateEnableState(this->ValueRange);
  this->PropagateEnableState(this->TopLeftContainer);
  this->PropagateEnableState(this->TopLeftFrame);
  this->PropagateEnableState(this->UserFrame);
  this->PropagateEnableState(this->PointEntriesFrame);
  this->PropagateEnableState(this->RangeLabel);
  this->PropagateEnableState(this->ParameterEntry);
  this->PropagateEnableState(this->HistogramLogModeOptionMenu);

  if (this->GetEnabled())
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
}

// ---------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHistogram(vtkKWHistogram *arg)
{
  if (this->Histogram != arg)
    {
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
    }

  if (this->Histogram && 
      this->Histogram->GetMTime() > this->LastHistogramBuildTime)
    {
    this->UpdateHistogramLogModeOptionMenu();
    this->RedrawHistogram();
    if (this->ComputeValueTicksFromHistogram)
      {
      this->RedrawRangeTicks();
      }
    }
}

// ---------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSecondaryHistogram(
  vtkKWHistogram *arg)
{
  if (this->SecondaryHistogram != arg)
    {
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
    }

  if (this->SecondaryHistogram && 
      this->SecondaryHistogram->GetMTime() > this->LastHistogramBuildTime)
    {
    this->UpdateHistogramLogModeOptionMenu();
    this->RedrawHistogram();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetCanvasItemCenter(int item_id, 
                                                            int *x, int *y)
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
    *x = vtkMath::Round((c_x1 + c_x2) * 0.5);
    *y = vtkMath::Round((c_y1 + c_y2) * 0.5);
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
    factors[0] = 
      (double)(this->CurrentCanvasWidth - 1 - margin_left - margin_right)
      / (p_v_range[1] - p_v_range[0]);
    }
  else
    {
    factors[0] = 0.0;
    }

  double *v_v_range = this->GetVisibleValueRange();
  if (v_v_range[1] != v_v_range[0])
    {
    factors[1] = 
      (double)(this->CurrentCanvasHeight - 1 - margin_top - margin_bottom)
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

  int max_point_radiusx = this->PointRadiusX;
  if (this->SelectedPointRadius > 1.0)
    {
    max_point_radiusx = 
      (int)ceil((double)max_point_radiusx * this->SelectedPointRadius);
    }
  int max_point_radiusy = this->PointRadiusY;
  if (this->SelectedPointRadius > 1.0)
    {
    max_point_radiusy = 
      (int)ceil((double)max_point_radiusy * this->SelectedPointRadius);
    }

  int point_marginx = (int)floor(
    (double)max_point_radiusx + (double)this->PointOutlineWidth * 0.5);

  int point_marginy = (int)floor(
    (double)max_point_radiusy + (double)this->PointOutlineWidth * 0.5);
  
  if (margin_left)
    {
    *margin_left = (this->PointMarginToCanvas & 
                    vtkKWParameterValueFunctionEditor::PointMarginLeftSide 
                    ? point_marginx : 0);
    }

  if (margin_right)
    {
    *margin_right = (this->PointMarginToCanvas & 
                     vtkKWParameterValueFunctionEditor::PointMarginRightSide 
                     ? point_marginx : 0);
    }

  if (margin_top)
    {
    *margin_top = (this->PointMarginToCanvas & 
                   vtkKWParameterValueFunctionEditor::PointMarginTopSide 
                   ? point_marginy : 0);
    }

  if (margin_bottom)
    {
    *margin_bottom = (this->PointMarginToCanvas & 
                      vtkKWParameterValueFunctionEditor::PointMarginBottomSide 
                      ? point_marginy : 0);
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
    *x2 = (c_x + (double)(this->CurrentCanvasWidth - 0));
    }

  if (y2)
    {
    *y2 = (c_y + (double)(this->CurrentCanvasHeight - 0));
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
unsigned long vtkKWParameterValueFunctionEditor::GetRedrawFunctionTime()
{
  return this->GetFunctionMTime();
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

  // Get the new canvas size

  int old_c_width = this->Canvas->GetWidth();
  int old_c_height = this->Canvas->GetHeight();

  if (this->ExpandCanvasWidth)
    {
    if (this->CanvasVisibility)
      {
      vtkKWTkUtilities::GetWidgetSize(
        this->Canvas, &this->CurrentCanvasWidth, NULL);
      }
    else
      {
      vtkKWTkUtilities::GetWidgetSize(
        this, &this->CurrentCanvasWidth, NULL);
      this->CurrentCanvasWidth -= 
        (this->GetBorderWidth() + this->GetPadX())* 2;
      }
    if (this->CurrentCanvasWidth < VTK_KW_PVFE_CANVAS_WIDTH_MIN)
      {
      this->CurrentCanvasWidth = VTK_KW_PVFE_CANVAS_WIDTH_MIN;
      }
    }
  else
    {
    this->CurrentCanvasWidth = this->RequestedCanvasWidth;
    }
  this->CurrentCanvasHeight = this->RequestedCanvasHeight;

  this->Canvas->SetWidth(this->CurrentCanvasWidth);
  this->Canvas->SetHeight(this->CurrentCanvasHeight);

  if (this->ValueTicksVisibility)
    {
    this->ValueTicksCanvas->SetHeight(this->CurrentCanvasHeight);
    }

  if (this->ParameterTicksVisibility)
    {
    this->ParameterTicksCanvas->SetWidth(this->CurrentCanvasWidth);
    }

  if (this->IsGuidelineValueCanvasUsed())
    {
    this->GuidelineValueCanvas->SetWidth(this->CurrentCanvasWidth);
    }

  // In that visible area, we must fit the visible parameter in the
  // width dimension, and the visible value range in the height dimension.
  // Get the corresponding scaling factors.

  double c_x, c_y, c_x2, c_y2;
  this->GetCanvasScrollRegion(&c_x, &c_y, &c_x2, &c_y2);

  char buffer[256];
  sprintf(buffer, "%lf %lf %lf %lf", c_x, c_y, c_x2, c_y2);
  this->Canvas->SetConfigurationOption("-scrollregion", buffer);

  if (this->ValueTicksVisibility)
    {
    this->ValueTicksCanvas->SetWidth(this->ValueTicksCanvasWidth);
    sprintf(buffer, "0 %lf %d %lf", c_y, this->ValueTicksCanvasWidth, c_y2);
    this->ValueTicksCanvas->SetConfigurationOption("-scrollregion", buffer);
    }

  if (this->ParameterTicksVisibility)
    {
    sprintf(buffer, "%lf 0 %lf %d", 
            c_x, c_x2, VTK_KW_PVFE_TICKS_PARAMETER_CANVAS_HEIGHT);
    this->ParameterTicksCanvas->SetConfigurationOption("-scrollregion",buffer);
    }

  if (this->IsGuidelineValueCanvasUsed())
    {
    sprintf(buffer, "%lf 0 %lf %d", 
            c_x, c_x2, VTK_KW_PVFE_GUIDELINE_VALUE_CANVAS_HEIGHT);
    this->GuidelineValueCanvas->SetConfigurationOption("-scrollregion",buffer);
    }

  // If the canvas has been resized,
  // or if the visible range has changed (i.e. if the relative size of the
  // visible range compared to the whole range has changed significantly)
  // or if the function has changed, etc.
 
  vtkKWParameterValueFunctionEditor::Ranges ranges;
  ranges.GetRangesFrom(this);

  if (old_c_width != this->CurrentCanvasWidth || 
      old_c_height != this->CurrentCanvasHeight ||
      ranges.NeedResizeComparedTo(&this->LastRanges))
    {
    this->RedrawSizeDependentElements();
    }

  if (ranges.NeedPanOnlyComparedTo(&this->LastRanges))
    {
    this->RedrawPanOnlyDependentElements();
    }

  if (!this->HasFunction() ||
      this->GetRedrawFunctionTime() > this->LastRedrawFunctionTime)
    {
    this->RedrawFunctionDependentElements();
    }

  this->LastRanges.GetRangesFrom(this);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawSizeDependentElements()
{
  this->RedrawHistogram();
  this->RedrawRangeTicks();
  this->RedrawParameterCursor();
  this->RedrawFunction();
  this->RedrawRangeFrame();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawPanOnlyDependentElements()
{
  this->RedrawHistogram();
  this->RedrawRangeTicks();

  // We used to draw the entire function for the whole canvas, even if
  // a small portion of it was displayed (in zoom mode). This would
  // use a lot of resources of lines were to be sampled betweem each points
  // every n-pixel. Now we only redraw the points and line that are
  // indeed visible. So if we pan, we need to redraw.

  this->RedrawFunction();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawFunctionDependentElements()
{
  this->RedrawFunction();
  this->RedrawRangeFrame();

  this->UpdatePointEntries(this->GetSelectedPoint());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawSinglePointDependentElements(
  int id)
{
  if (id < 0 || id >= this->GetFunctionSize())
    {
    return;
    }

  this->RedrawPoint(id);

  this->RedrawLine(id - 1, id);
  this->RedrawLine(id, id + 1);

  if (id == this->GetSelectedPoint())
    {
    this->UpdatePointEntries(id);
    }

  // The range frame uses the two extreme points
  
  if ((id == 0 || id == this->GetFunctionSize() - 1))
    {
    this->RedrawRangeFrame(); 
    }
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

  int has_tag = this->CanvasHasTag(
    vtkKWParameterValueFunctionEditor::FrameForegroundTag);
  if (!has_tag)
    {
    if (this->CanvasOutlineVisibility && this->CanvasVisibility)
      {
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleLeftSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_l " 
               << vtkKWParameterValueFunctionEditor::FrameForegroundTag 
               << "}\n";
        }
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleRightSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_r " 
               << vtkKWParameterValueFunctionEditor::FrameForegroundTag 
               << "}\n";
        }
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleTopSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_t " 
               << vtkKWParameterValueFunctionEditor::FrameForegroundTag 
               << "}\n";
        }
      if (this->CanvasOutlineStyle & 
          vtkKWParameterValueFunctionEditor::CanvasOutlineStyleBottomSide)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << "-tags {framefg_b " 
               << vtkKWParameterValueFunctionEditor::FrameForegroundTag 
               << "}\n";
        }
      }
    }
  else 
    {
    if (!this->CanvasOutlineVisibility || !this->CanvasVisibility)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::FrameForegroundTag << endl;
      }
    }

  // The background

  has_tag = this->CanvasHasTag(
    vtkKWParameterValueFunctionEditor::FrameBackgroundTag);
  if (!has_tag)
    {
    if (this->CanvasBackgroundVisibility && this->CanvasVisibility)
      {
      tk_cmd << canv << " create rectangle 0 0 0 0 "
             << " -tags {" 
             << vtkKWParameterValueFunctionEditor::FrameBackgroundTag << "}" 
             << endl;
      tk_cmd << canv << " lower " 
             << vtkKWParameterValueFunctionEditor::FrameBackgroundTag 
             << " all" << endl;
      }
    }
  else 
    {
    if (!this->CanvasBackgroundVisibility || !this->CanvasVisibility)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::FrameBackgroundTag << endl;
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

  if (this->CanvasOutlineVisibility && this->CanvasVisibility)
    {
    double c1_x = p_w_range[0] * factors[0];
    double c1_y = v_w_range[0] * factors[1];
    double c2_x = p_w_range[1] * factors[0];
    double c2_y = v_w_range[1] * factors[1];

    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleLeftSide)
      {
      tk_cmd << canv << " coords framefg_l " 
             << c1_x << " " << c2_y + 1 - LSTRANGE << " " 
             << c1_x << " " << c1_y - LSTRANGE << endl;
      if (this->PointMarginToCanvas &
          vtkKWParameterValueFunctionEditor::PointMarginLeftSide)
        {
        tk_cmd << canv << " lower framefg_l " 
               << " {" <<vtkKWParameterValueFunctionEditor::FunctionTag <<"}"
               << endl;
        }
      else
        {
        tk_cmd << canv << " raise framefg_l all" << endl;
        }
      }
    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleRightSide)
      {
      tk_cmd << canv << " coords framefg_r " 
             << c2_x << " " << c2_y + 1 - LSTRANGE << " " 
             << c2_x << " " << c1_y - LSTRANGE << endl;
      if (this->PointMarginToCanvas &
          vtkKWParameterValueFunctionEditor::PointMarginRightSide)
        {
        tk_cmd << canv << " lower framefg_r " 
               << " {" <<vtkKWParameterValueFunctionEditor::FunctionTag <<"}"
               << endl;
        }
      else
        {
        tk_cmd << canv << " raise framefg_r all" << endl;
        }
      }
    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleTopSide)
      {
      tk_cmd << canv << " coords framefg_t " 
             << c2_x + 1 - LSTRANGE << " " << c1_y << " " 
             << c1_x - LSTRANGE << " " << c1_y << endl;
      if (this->PointMarginToCanvas &
          vtkKWParameterValueFunctionEditor::PointMarginTopSide)
        {
        tk_cmd << canv << " lower framefg_t " 
               << " {" <<vtkKWParameterValueFunctionEditor::FunctionTag <<"}"
               << endl;
        }
      else
        {
        tk_cmd << canv << " raise framefg_t all" << endl;
        }
      }
    if (this->CanvasOutlineStyle & 
        vtkKWParameterValueFunctionEditor::CanvasOutlineStyleBottomSide)
      {
      tk_cmd << canv << " coords framefg_b " 
             << c2_x + 1 - LSTRANGE << " " << c2_y << " " 
             << c1_x - LSTRANGE << " " << c2_y << endl;
      if (this->PointMarginToCanvas &
          vtkKWParameterValueFunctionEditor::PointMarginBottomSide)
        {
        tk_cmd << canv << " lower framefg_b " 
               << " {" <<vtkKWParameterValueFunctionEditor::FunctionTag <<"}"
               << endl;
        }
      else
        {
        tk_cmd << canv << " raise framefg_b all" << endl;
        }
      }
    }

  if (this->CanvasBackgroundVisibility && this->CanvasVisibility)
    {
    tk_cmd << canv << " coords " 
           << vtkKWParameterValueFunctionEditor::FrameBackgroundTag 
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
    tk_cmd << canv << " itemconfigure " << vtkKWParameterValueFunctionEditor::FrameBackgroundTag 
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
  if (this->ValueTicksVisibility)
    {
    v_t_canv = this->ValueTicksCanvas->GetWidgetName();
    }

  const char *p_t_canv = NULL;
  if (this->ParameterTicksVisibility)
    {
    p_t_canv = this->ParameterTicksCanvas->GetWidgetName();
    }

  ostrstream tk_cmd;

  // Create the ticks if not created already

  int has_p_tag = 
    this->CanvasHasTag(vtkKWParameterValueFunctionEditor::ParameterTicksTag);
  if (!has_p_tag)
    {
    if (this->ParameterTicksVisibility)
      {
      for (int i = 0; i < this->NumberOfParameterTicks; i++)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {p_tick_t" << i << " " 
               << vtkKWParameterValueFunctionEditor::ParameterTicksTag << "}" 
               << endl;
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {p_tick_b" << i << " " 
               << vtkKWParameterValueFunctionEditor::ParameterTicksTag << "}" 
               << endl;
        if (this->ParameterTicksFormat)
          {
          tk_cmd << p_t_canv << " create text 0 0 -text {} -anchor n " 
                 << "-font {{fixed} " << VTK_KW_PVFE_TICKS_TEXT_SIZE << "} "
                 << "-tags {p_tick_b_t" << i << " " 
                 << vtkKWParameterValueFunctionEditor::ParameterTicksTag 
                 << "}" 
                 << endl;
          }
        }
      }
    }
  else 
    {
    if (!this->ParameterTicksVisibility)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::ParameterTicksTag << endl;
      tk_cmd << p_t_canv << " delete " 
             << vtkKWParameterValueFunctionEditor::ParameterTicksTag << endl;
      }
    }

  int has_v_tag = 
    this->CanvasHasTag(vtkKWParameterValueFunctionEditor::ValueTicksTag);
  if (!has_v_tag)
    {
    if (this->ValueTicksVisibility)
      {
      for (int i = 0; i < this->NumberOfValueTicks; i++)
        {
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {v_tick_l" << i << " " 
               << vtkKWParameterValueFunctionEditor::ValueTicksTag << "}" 
               << endl;
        tk_cmd << canv << " create line 0 0 0 0 "
               << " -tags {v_tick_r" << i << " " 
               << vtkKWParameterValueFunctionEditor::ValueTicksTag << "}" 
               << endl;
        tk_cmd << v_t_canv << " create text 0 0 -text {} -anchor e " 
               << "-font {{fixed} " << VTK_KW_PVFE_TICKS_TEXT_SIZE << "} "
               << "-tags {v_tick_l_t" << i << " " 
               << vtkKWParameterValueFunctionEditor::ValueTicksTag << "}" 
               << endl;
        }
      }
    }
  else 
    {
    if (!this->ValueTicksVisibility)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::ValueTicksTag << endl;
      tk_cmd << v_t_canv << " delete " 
             << vtkKWParameterValueFunctionEditor::ValueTicksTag << endl;
      }
    }

  // Update coordinates and colors

  if (this->ParameterTicksVisibility || this->ValueTicksVisibility)
    {
    double factors[2] = {0.0, 0.0};
    this->GetCanvasScalingFactors(factors);

    double *p_v_range = this->GetVisibleParameterRange();
    double *v_v_range = this->GetVisibleValueRange();
    double *v_w_range = this->GetWholeValueRange();

    char buffer[256];

    if (this->ParameterTicksVisibility)
      {
      double y_t = (v_w_range[1] - v_v_range[1]) * factors[1];
      double y_b = (v_w_range[1] - v_v_range[0]) * factors[1];

      double p_v_step = (p_v_range[1] - p_v_range[0]) / 
        (double)(this->NumberOfParameterTicks + 1);
      double displayed_p, p_v_pos = p_v_range[0] + p_v_step;

      for (int i = 0; i < this->NumberOfParameterTicks; i++)
        {
        double x = p_v_pos * factors[0];
        tk_cmd << canv << " coords p_tick_t" <<  i << " " 
               << x << " " << y_t << " " << x << " " << y_t + this->TicksLength
               << endl;
        tk_cmd << canv << " coords p_tick_b" <<  i << " " 
               << x << " " << y_b << " " << x << " " << y_b - this->TicksLength
               << endl;
        if (this->ParameterTicksFormat)
          {
          tk_cmd << p_t_canv << " coords p_tick_b_t" <<  i << " " 
                 << x << " " << -1 << endl;
          this->MapParameterToDisplayedParameter(p_v_pos, &displayed_p);
          sprintf(buffer, this->ParameterTicksFormat, displayed_p);
          tk_cmd << p_t_canv << " itemconfigure p_tick_b_t" <<  i << " " 
                 << " -text {" << buffer << "}" << endl;
          }
        p_v_pos += p_v_step;
        }
      }

    if (this->ValueTicksVisibility)
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

  int has_tag = 
    this->CanvasHasTag(vtkKWParameterValueFunctionEditor::ParameterCursorTag);
  if (!has_tag)
    {
    if (this->ParameterCursorVisibility && this->CanvasVisibility)
      {
      tk_cmd << canv << " create line 0 0 0 0 "
             << " -tags {" 
             << vtkKWParameterValueFunctionEditor::ParameterCursorTag << "}" 
             << endl;
      tk_cmd << canv << " lower " 
             << vtkKWParameterValueFunctionEditor::ParameterCursorTag
             << " {" << vtkKWParameterValueFunctionEditor::FunctionTag << "}" 
             << endl;
      }
    }
  else 
    {
    if (!this->ParameterCursorVisibility || !this->CanvasVisibility)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::ParameterCursorTag<< endl;
      }
    }

  // Update the cursor position and style
  
  if (this->ParameterCursorVisibility && this->CanvasVisibility)
    {
    double v_v_range[2];
    this->GetWholeValueRange(v_v_range);

    double factors[2] = {0.0, 0.0};
    this->GetCanvasScalingFactors(factors);

    tk_cmd << canv << " coords " 
           << vtkKWParameterValueFunctionEditor::ParameterCursorTag
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
    
    tk_cmd << canv << " itemconfigure " 
           << vtkKWParameterValueFunctionEditor::ParameterCursorTag
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
  if (!this->IsCreated() || !this->HasFunction() || this->DisableRedraw)
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

  int x, y, rx = 0, ry = 0;
  int is_not_visible = 0, is_not_visible_h = 0;
  int is_not_valid = (id < 0 || id >= this->GetFunctionSize());

  // Point style

  int func_size = this->GetFunctionSize();

  int style = this->PointStyle;
  if (id == 0 && this->FirstPointStyle != 
      vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    style = this->FirstPointStyle;
    }
  if (id == func_size - 1 && this->LastPointStyle != 
      vtkKWParameterValueFunctionEditor::PointStyleDefault)
    {
    style = this->LastPointStyle;
    }
  
  // Get the point coords, radius, check if the point is visible

  if (!is_not_valid)
    {
    this->GetFunctionPointCanvasCoordinates(id, &x, &y);

    rx = this->PointRadiusX;
    if (id == this->GetSelectedPoint())
      {
      rx = (int)ceil((double)rx * this->SelectedPointRadius);
      }
    ry = this->PointRadiusY;
    if (id == this->GetSelectedPoint())
      {
      ry = (int)ceil((double)ry * this->SelectedPointRadius);
      }

    // If the point is not in the visible range, hide it

    double c_x, c_y, c_x2, c_y2;
    this->GetCanvasScrollRegion(&c_x, &c_y, &c_x2, &c_y2);

    int visible_marginx = rx + this->PointOutlineWidth + 5;
    int visible_marginy = ry + this->PointOutlineWidth + 5;

    if (x + visible_marginx < c_x || c_x2 < x - visible_marginx)
      {
      is_not_visible_h = 1;
      }
    
    if (is_not_visible_h || 
        y + visible_marginy < c_y || c_y2 < y - visible_marginy)
      {
      is_not_visible = 1;
      }
    }


  double rgb[3];
  char color[10];

  // Create the text item (index at each point)
  
  int text_exists = this->CanvasHasTag("t", &id);
  if (is_not_valid)
    {
    if (text_exists)
      {
      *tk_cmd << canv << " delete t" << id << endl;
      }
    }
  else
    {
    if (is_not_visible  ||
        !this->CanvasVisibility ||
        !this->PointVisibility || 
        !(this->PointIndexVisibility ||
          (this->SelectedPointIndexVisibility && 
           id == this->GetSelectedPoint())))
      {
      if (text_exists)
        {
        *tk_cmd << canv << " itemconfigure t" << id << " -state hidden\n"; 
        }
      }
    else
      {
      if (!text_exists)
        {
        *tk_cmd << canv << " create text 0 0 -text {} " 
                << "-tags {t" << id 
                << " " << vtkKWParameterValueFunctionEditor::PointTextTag
                << " " << vtkKWParameterValueFunctionEditor::FunctionTag << "}"
                << endl;
        }
    
      // Update the text coordinates
      
      *tk_cmd << canv << " coords t" << id << " " << x << " " << y << endl;

      // Update the text color and contents
      
      if (this->GetFunctionPointTextColorInCanvas(id, rgb))
        {
        int text_size = 7 - (id > 8 ? 1 : 0);
        sprintf(color, "#%02x%02x%02x", 
                (int)(rgb[0]*255.0), (int)(rgb[1]*255.0), (int)(rgb[2]*255.0));
        *tk_cmd << canv << " itemconfigure t" << id
                << " -state normal -font {{fixed} " << text_size << "} -fill " 
                << color << " -text {";
        if (this->SelectedPointText && id == this->GetSelectedPoint())
          {
          *tk_cmd << this->SelectedPointText;
          }
        else
          {
          *tk_cmd << id + 1;
          }
        *tk_cmd  << "}" << endl;
        }
      }
    }

  // Create the point

  int point_exists = this->CanvasHasTag("p", &id);
  if (is_not_valid)
    {
    if (point_exists)
      {
      *tk_cmd << canv << " delete p" << id << endl;
      }
    }
  else
    {
    if (is_not_visible || 
        !this->PointVisibility || 
        !this->CanvasVisibility)
      {
      if (point_exists)
        {
        *tk_cmd << canv << " itemconfigure p" << id << " -state hidden\n";
        }
      }
    else
      {
      // Get the point style
      // Points are reused. Since each point is of a specific type, it is OK
      // as long as the point styles are not mixed. If they are (i.e., a 
      // special style for the first or last point for example), we have to
      // check for the point type. If it is not the right type, the point can
      // not be reused as the coordinates spec would not match. In that case,
      // delete the point right now.

      int must_check_for_type = 0;

      if (this->FirstPointStyle != 
          vtkKWParameterValueFunctionEditor::PointStyleDefault ||
          this->LastPointStyle != 
          vtkKWParameterValueFunctionEditor::PointStyleDefault)
        {
        must_check_for_type = 1;
        }

      // Check if we need to recreate it

      if (point_exists && must_check_for_type)
        {
        int must_delete_point = 0;
        switch (style)
          {
          case vtkKWParameterValueFunctionEditor::PointStyleDefault:
          case vtkKWParameterValueFunctionEditor::PointStyleDisc:
            must_delete_point = 
              !this->CanvasCheckTagType("p", id, "oval");
            break;

          case vtkKWParameterValueFunctionEditor::PointStyleRectangle:
            must_delete_point = 
              !this->CanvasCheckTagType("p", id, "rectangle");
            break;

          case vtkKWParameterValueFunctionEditor::PointStyleCursorDown:
          case vtkKWParameterValueFunctionEditor::PointStyleCursorUp:
          case vtkKWParameterValueFunctionEditor::PointStyleCursorLeft:
          case vtkKWParameterValueFunctionEditor::PointStyleCursorRight:
            must_delete_point = 
              !this->CanvasCheckTagType("p", id, "polygon");
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
                << " " << vtkKWParameterValueFunctionEditor::PointTag 
                << " " << vtkKWParameterValueFunctionEditor::FunctionTag 
                << "}" << endl;
        *tk_cmd << canv << " lower p" << id << " t" << id << endl;
        }

      // Update the point coordinates and style

      switch (style)
        {
        case vtkKWParameterValueFunctionEditor::PointStyleDefault:
        case vtkKWParameterValueFunctionEditor::PointStyleDisc:
          *tk_cmd << canv << " coords p" << id 
                  << " " << x - rx << " " << y - ry 
                  << " " << x + rx << " " << y + ry << endl;
          break;
      
        case vtkKWParameterValueFunctionEditor::PointStyleRectangle:
          *tk_cmd << canv << " coords p" << id 
                  << " " << x - rx << " " << y - ry 
                  << " " << x + rx + LSTRANGE << " " << y + ry + LSTRANGE 
                  << endl;
          break;
      
        case vtkKWParameterValueFunctionEditor::PointStyleCursorDown:
          *tk_cmd << canv << " coords p" << id 
                  << " " << x - rx << " " << y 
                  << " " << x      << " " << y + ry
                  << " " << x + rx << " " << y 
                  << " " << x + rx << " " << y - ry + 1 
                  << " " << x - rx << " " << y - ry + 1 
                  << endl;
          break;

        case vtkKWParameterValueFunctionEditor::PointStyleCursorUp:
          *tk_cmd << canv << " coords p" << id 
                  << " " << x - rx << " " << y 
                  << " " << x      << " " << y - ry
                  << " " << x + rx << " " << y 
                  << " " << x + rx << " " << y + ry - 1 
                  << " " << x - rx << " " << y + ry - 1 
                  << endl;
          break;

        case vtkKWParameterValueFunctionEditor::PointStyleCursorLeft:
          *tk_cmd << canv << " coords p" << id 
                  << " " << x          << " " << y + ry
                  << " " << x - rx     << " " << y
                  << " " << x          << " " << y - ry
                  << " " << x + rx - 1 << " " << y - ry 
                  << " " << x + rx - 1 << " " << y + ry
                  << endl;
          break;

        case vtkKWParameterValueFunctionEditor::PointStyleCursorRight:
          *tk_cmd << canv << " coords p" << id 
                  << " " << x          << " " << y + ry
                  << " " << x + rx     << " " << y
                  << " " << x          << " " << y - ry
                  << " " << x - rx + 1 << " " << y - ry 
                  << " " << x - rx + 1 << " " << y + ry
                  << endl;
          break;
        }

      *tk_cmd << canv << " itemconfigure p" << id 
              << " -state normal  -width " << this->PointOutlineWidth << endl;

      // Update the point color

      if (this->GetFunctionPointColorInCanvas(id, rgb))
        {
        sprintf(color, "#%02x%02x%02x", 
                (int)(rgb[0]*255.0), (int)(rgb[1]*255.0), (int)(rgb[2]*255.0));
        *tk_cmd << canv << " itemconfigure p" << id;
        if (this->PointColorStyle == 
            vtkKWParameterValueFunctionEditor::PointColorStyleFill)
          {
          *tk_cmd << " -outline black -fill " << color << endl;
          }
        else
          {
          *tk_cmd << " -fill {} -outline " << color << endl;
          }
        }
      }
    }

  // Create and/or update the point guideline

  int guide_exists = this->CanvasHasTag("g", &id);
  if (is_not_valid)
    {
    if (guide_exists)
      {
      *tk_cmd << canv << " delete g" << id << endl;
      }
    }
  else
    {
    if (is_not_visible_h || 
        !this->PointGuidelineVisibility || 
        !this->CanvasVisibility)
      {
      if (guide_exists)
        {
        *tk_cmd << canv << " itemconfigure g" << id << " -state hidden\n";
        }
      }
    else
      {
      if (!guide_exists)
        {
        *tk_cmd << canv << " create line 0 0 0 0 -fill black -width 1 " 
                << " -tags {g" << id << " " 
                << vtkKWParameterValueFunctionEditor::PointGuidelineTag 
                << " " << vtkKWParameterValueFunctionEditor::FunctionTag
                << "}" << endl;
        *tk_cmd << canv << " lower g" << id << " p" << id << endl;
        }
  
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
      *tk_cmd << " -state normal" << endl;
      }
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
int vtkKWParameterValueFunctionEditor::FunctionLineIsInVisibleRangeBetweenPoints(
  int id1, int id2)
{
  if (id1 < 0 || id1 >= this->GetFunctionSize() ||
      id2 < 0 || id2 >= this->GetFunctionSize())
    {
    return 0;
    }

  // Get the line coordinates

  int x1, y1, x2, y2;
  this->GetFunctionPointCanvasCoordinates(id1, &x1, &y1);
  this->GetFunctionPointCanvasCoordinates(id2, &x2, &y2);

  // Reorder so that it matches the canvas coords

  if (x1 > x2)
    {
    int temp = x1;
    x1 = x2;
    x2 = temp;
    }
  if (y1 > y2)
    {
    int temp = y1;
    y1 = y2;
    y2 = temp;
    }

  // Get the current canvas visible coords

  double c_x1, c_y1, c_x2, c_y2;
  this->GetCanvasScrollRegion(&c_x1, &c_y1, &c_x2, &c_y2);

  int margin = this->FunctionLineWidth + 5;

  // Check

  return (x2 + margin < c_x1 || 
          c_x2 < x1 - margin || 
          y2 + margin < c_y1 || 
          c_y2 < y1 - margin) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawLine(
  int id1, int id2, ostrstream *tk_cmd)
{
  if (!this->IsCreated() || !this->HasFunction() || this->DisableRedraw)
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

  int is_not_valid = (id1 == id2 ||
                      id1 < 0 || id1 >= this->GetFunctionSize() ||
                      id2 < 0 || id2 >= this->GetFunctionSize());

  // Switch boths id's if they were not specified in order
  
  if (id1 > id2)
    {
    int temp = id1;
    id1 = id2;
    id2 = temp;
    }
  
  // Create the line item
  
  int line_exists = this->CanvasHasTag("l", &id2);
  if (is_not_valid)
    {
    if (line_exists)
      {
      *tk_cmd << canv << " delete l" << id2 << endl;
      }
    }
  else
    {
    // Hide the line if not visible
    // Actually let's just destroy it to keep memory low in case
    // many segments were generated for a sampled line

    if (!this->FunctionLineVisibility || 
        !this->CanvasVisibility ||
        !this->FunctionLineIsInVisibleRangeBetweenPoints(id1, id2))
      {
      if (line_exists)
        {
        //*tk_cmd << canv << " itemconfigure l" << id2 << " -state hidden\n";
        *tk_cmd << canv << " delete l" << id2 << endl;
        }
      }
    else
      {
      // Create the poly-line between the points
      // The line id is the id of the second end-point (id2)
  
      if (!line_exists)
        {
        *tk_cmd << canv << " create line 0 0 0 0 -fill black " 
                << " -tags {l" << id2 
                << " " << vtkKWParameterValueFunctionEditor::LineTag
                << " " << vtkKWParameterValueFunctionEditor::FunctionTag 
                << "}" << endl;
        *tk_cmd << canv << " lower l" << id2 
                << " {p" << id1 << "||p" << id2 << "||m_p" << id1
                << "}" << endl;
        }
  
      // Get the point coords
      // Use either a straight line, or sample points

      *tk_cmd << canv << " coords l" << id2;
      this->GetLineCoordinates(id1, id2, tk_cmd);
      *tk_cmd << endl;

      // Configure style
      
      *tk_cmd << canv << " itemconfigure l" << id2 
              << " -state normal -width " << this->FunctionLineWidth;
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
void vtkKWParameterValueFunctionEditor::GetLineCoordinates(
  int id1, int id2, ostrstream *tk_cmd)
{
  // We assume all parameters are OK, they were checked by RedrawLine

  // Use either a straight line, or sample points

  int x1, y1, x2, y2;
  this->GetFunctionPointCanvasCoordinates(id1, &x1, &y1);
  this->GetFunctionPointCanvasCoordinates(id2, &x2, &y2);

  *tk_cmd << " " << x1 << " " << y1;

  if (this->FunctionLineIsSampledBetweenPoints(id1, id2))
    {
    double id1_p, id2_p;
    if (this->GetFunctionPointParameter(id1, &id1_p) &&
        this->GetFunctionPointParameter(id2, &id2_p))
      {
      // we want segments no longer than 2 pixels
      // Also, check that we do not exceed, say 2000 pixels total, that
      // would be a flag that something not reasonable is happenin, i.e.
      // a function has just been set but the visible does not match
      // at all and extreme zooming is happening before the user had the
      // time to reset the range.

      int max_segment_length = 2;
          
      int x, y;
      int nb_steps = 
        (int)ceil((double)(x2 - x1) / (double)max_segment_length);
      if (nb_steps > 1000)
        {
        nb_steps = 1000;
        }
      for (int i = 1; i < nb_steps; i++)
        {
        double p = id1_p + (id2_p - id1_p)*((double)i / (double)nb_steps);
        if (this->GetFunctionPointCanvasCoordinatesAtParameter(p, &x, &y))
          {
          *tk_cmd << " " << x << " " << y;
          }
        }
      }
    }

  *tk_cmd << " " << x2 << " " << y2;
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
    this->CanvasRemoveTag(vtkKWParameterValueFunctionEditor::FunctionTag);
    return;
    }

  // Are we going to create or delete points ?

  int nb_points_changed = 
    (this->LastRedrawFunctionSize != this->GetFunctionSize());

  // Try to save the selection before (eventually) creating new points

  int s_x = 0, s_y = 0;
  if (nb_points_changed && this->HasSelection())
    {
    int item_id = atoi(
      this->Script("lindex [%s find withtag %s] 0",
                   canv, vtkKWParameterValueFunctionEditor::SelectedTag));
    this->GetCanvasItemCenter(item_id, &s_x, &s_y);
    }

  // Create the points 
  
  ostrstream tk_cmd;

  int i, nb_potential_points = this->GetFunctionSize();
  if (nb_potential_points < this->LastRedrawFunctionSize)
    {
    nb_potential_points = this->LastRedrawFunctionSize;
    }

  // Note that we redraw the points that are out of the function id range
  // this is OK since this will delete them automatically

  if (nb_potential_points)
    {
    this->RedrawPoint(0, &tk_cmd);
    for (i = 1; i < nb_potential_points; i++)
      {
      this->RedrawPoint(i, &tk_cmd);
      this->RedrawLine(i - 1, i, &tk_cmd);
      }
    }

  // Execute all of this

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  this->LastRedrawFunctionSize = this->GetFunctionSize();
  this->LastRedrawFunctionTime = this->GetFunctionMTime();

  // Try to restore the selection

  if (nb_points_changed && this->HasSelection())
    {
    int p_x = 0, p_y = 0;
    for (i = 0; i < this->LastRedrawFunctionSize; i++)
      {
      this->GetFunctionPointCanvasCoordinates(i, &p_x, &p_y);
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
    this->CurrentCanvasHeight - margin_top - margin_bottom);

  desc->SetBackgroundColor(this->FrameBackgroundColor);
  desc->DrawGrid = 0;
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

  // Main histogram descriptor

  int hist_is_image = 
    this->HistogramStyle == 
    vtkKWParameterValueFunctionEditor::HistogramStyleBars || 
    this->HistogramStyle == 
    vtkKWParameterValueFunctionEditor::HistogramStyleDots;

  if (!this->HistogramImageDescriptor)
    {
    this->HistogramImageDescriptor = 
      new vtkKWHistogram::ImageDescriptor; 
    }
  this->HistogramImageDescriptor->SetColor(
    this->HistogramColor);
  this->HistogramImageDescriptor->Style = 
    this->HistogramStyle;
  this->HistogramImageDescriptor->DrawForeground = 
    hist_is_image ? 1 : 0;
  this->HistogramImageDescriptor->DrawBackground = 1;
  this->UpdateHistogramImageDescriptor(
    this->HistogramImageDescriptor);

  int has_hist_tag = 
    this->CanvasHasTag(
      vtkKWParameterValueFunctionEditor::HistogramTag);

  vtksys_stl::string hist_img_name;
  if (this->Histogram && hist_is_image)
    {
    hist_img_name = canv;
    hist_img_name += '.';
    hist_img_name += 
      vtkKWParameterValueFunctionEditor::HistogramTag;
    if (!vtkKWTkUtilities::FindPhoto(
          this->GetApplication(), hist_img_name.c_str()))
      {
      this->Script("image create photo %s -width 0 -height 0", 
                   hist_img_name.c_str());
      }
    }

  // Secondary histogram

  int secondary_hist_is_image = 
    this->SecondaryHistogramStyle == 
    vtkKWParameterValueFunctionEditor::HistogramStyleBars || 
    this->SecondaryHistogramStyle == 
    vtkKWParameterValueFunctionEditor::HistogramStyleDots;

  if (!this->SecondaryHistogramImageDescriptor)
    {
    this->SecondaryHistogramImageDescriptor = 
      new vtkKWHistogram::ImageDescriptor;
    }

  this->SecondaryHistogramImageDescriptor->SetColor(
    this->SecondaryHistogramColor);
  this->SecondaryHistogramImageDescriptor->Style = 
    this->SecondaryHistogramStyle;
  this->SecondaryHistogramImageDescriptor->DrawForeground = 
    secondary_hist_is_image ? 1 : 0;
  this->SecondaryHistogramImageDescriptor->DrawBackground = 
    hist_is_image ? 0 : 1; // if both are images, need alpha to blend them
  this->UpdateHistogramImageDescriptor(
    this->SecondaryHistogramImageDescriptor);

  int has_secondary_hist_tag = 
    this->CanvasHasTag(
      vtkKWParameterValueFunctionEditor::SecondaryHistogramTag);

  vtksys_stl::string secondary_hist_img_name;
  if (this->SecondaryHistogram && secondary_hist_is_image)
    {
    secondary_hist_img_name = canv;
    secondary_hist_img_name += '.';
    secondary_hist_img_name += 
      vtkKWParameterValueFunctionEditor::SecondaryHistogramTag;
    if (!vtkKWTkUtilities::FindPhoto(
          this->GetApplication(), secondary_hist_img_name.c_str()))
      {
      this->Script("image create photo %s -width 0 -height 0", 
                   secondary_hist_img_name.c_str());
      }
    }
  
  // Create the images
  
  double *p_v_range = this->GetVisibleParameterRange();

  vtkImageData *hist_image = NULL;
  vtkIntArray *hist_image_coords = NULL;

  vtkImageData *secondary_hist_image = NULL;
  vtkIntArray *secondary_hist_image_coords = NULL;

  vtksys_stl::string blend_hist_img_name;
  if (this->Histogram && hist_is_image &&
      this->SecondaryHistogram && secondary_hist_is_image)
    {
    blend_hist_img_name = hist_img_name;
    blend_hist_img_name += "_blend_";
    blend_hist_img_name += 
      vtkKWParameterValueFunctionEditor::SecondaryHistogramTag;
    }
      
  if ((this->Histogram || this->SecondaryHistogram) && 
      p_v_range[0] != p_v_range[1])
    {
    // Main histogram

    unsigned long hist_image_mtime = 0;
    if (this->Histogram)
      {
      if (hist_is_image)
        {
        hist_image = this->Histogram->GetImage(
          this->HistogramImageDescriptor);
        }
      hist_image_coords = 
        this->Histogram->GetImageCoordinates(
          this->HistogramImageDescriptor);
      hist_image_mtime = hist_image_coords->GetMTime();
      }

    // Secondary histogram

    if (this->Histogram)
      {
      this->SecondaryHistogramImageDescriptor->DefaultMaximumOccurence = 
        this->HistogramImageDescriptor->LastMaximumOccurence;
      }

    unsigned long secondary_hist_image_mtime = 0;
    if (this->SecondaryHistogram)
      {
      if (secondary_hist_is_image)
        {
        secondary_hist_image = this->SecondaryHistogram->GetImage(
          this->SecondaryHistogramImageDescriptor);
        }
      secondary_hist_image_coords = 
        this->SecondaryHistogram->GetImageCoordinates(
          this->SecondaryHistogramImageDescriptor);
      secondary_hist_image_mtime = secondary_hist_image_coords->GetMTime();
      }

    // Update the image in the canvas
    // Blend main and secondary histograms if needed

    if (hist_image_mtime > this->LastHistogramBuildTime ||
        secondary_hist_image_mtime > this->LastHistogramBuildTime)
      {
      if (hist_image)
        {
        int *wext = hist_image->GetWholeExtent();
        int width = wext[1] - wext[0] + 1;
        int height = wext[3] - wext[2] + 1;
        int pixel_size = hist_image->GetNumberOfScalarComponents();
        unsigned char *pixels = 
          static_cast<unsigned char*>(hist_image->GetScalarPointer());
        unsigned long buffer_length =  width * height * pixel_size;
        vtkKWTkUtilities::UpdatePhoto(
          this->GetApplication(), hist_img_name.c_str(), 
          pixels, width, height, pixel_size, buffer_length,
          vtkKWTkUtilities::UpdatePhotoOptionFlipVertical);
        }
      if (secondary_hist_image)
        {
        int *wext = secondary_hist_image->GetWholeExtent();
        int width = wext[1] - wext[0] + 1;
        int height = wext[3] - wext[2] + 1;
        int pixel_size = secondary_hist_image->GetNumberOfScalarComponents();
        unsigned char *pixels = 
         static_cast<unsigned char*>(secondary_hist_image->GetScalarPointer());
        unsigned long buffer_length =  width * height * pixel_size;
        vtkKWTkUtilities::UpdatePhoto(
          this->GetApplication(), secondary_hist_img_name.c_str(), 
          pixels, width, height, pixel_size, buffer_length,
          vtkKWTkUtilities::UpdatePhotoOptionFlipVertical);
        }
      if (hist_image && secondary_hist_image)
        {
        vtkImageBlend *blend = vtkImageBlend::New();
        blend->AddInput(hist_image);
        blend->AddInput(secondary_hist_image);
        vtkImageData *img_data = blend->GetOutput();
        img_data->Update();
        int *wext = img_data->GetWholeExtent();
        int width = wext[1] - wext[0] + 1;
        int height = wext[3] - wext[2] + 1;
        int pixel_size = img_data->GetNumberOfScalarComponents();
        unsigned char *pixels = 
          static_cast<unsigned char*>(img_data->GetScalarPointer());
        unsigned long buffer_length =  width * height * pixel_size;
        vtkKWTkUtilities::UpdatePhoto(
          this->GetApplication(), blend_hist_img_name.c_str(), 
          pixels, width, height, pixel_size, buffer_length,
          vtkKWTkUtilities::UpdatePhotoOptionFlipVertical);
        blend->Delete();
        }
      
      this->LastHistogramBuildTime = 
        (hist_image_mtime > secondary_hist_image_mtime 
         ? hist_image_mtime : secondary_hist_image_mtime);
      }
    }

  ostrstream tk_cmd;

  char color[10];
  double *v_w_range = this->GetWholeValueRange();
  double *v_v_range = this->GetVisibleValueRange();
  double factors[2] = {0.0, 0.0};

  // Update the histogram position (or delete it if not needed anymore)

  if (this->Histogram && 
      (!hist_is_image || hist_image))
    {
    if (!has_hist_tag)
      {
      if (hist_is_image)
        {
        tk_cmd << canv << " create image 0 0 -anchor nw";
        }
      else
        {
        tk_cmd << canv << " create line 0 0 0 0";
        }
      tk_cmd << " -tags {" 
             << vtkKWParameterValueFunctionEditor::HistogramTag
             << "}" << endl;
      if (this->CanvasBackgroundVisibility && this->CanvasVisibility)
        {
        tk_cmd << canv << " raise " 
               << vtkKWParameterValueFunctionEditor::HistogramTag 
               << " " << vtkKWParameterValueFunctionEditor::FrameBackgroundTag
               << endl;
        }
      }
    
    this->GetCanvasScalingFactors(factors);
    double c_x = 
      this->HistogramImageDescriptor->Range[0] * factors[0];

    tk_cmd << canv << " coords " 
           << vtkKWParameterValueFunctionEditor::HistogramTag
           << " ";
    if (hist_is_image)
      {
      double c_y = factors[1] * (v_w_range[1] - v_v_range[1]);
      tk_cmd << c_x << " " << c_y << endl;
      tk_cmd << canv << " itemconfigure "
             << vtkKWParameterValueFunctionEditor::HistogramTag
             << " -image " 
             << (secondary_hist_image ? blend_hist_img_name.c_str() 
                 : hist_img_name.c_str()) << endl;
      }
    else
      {
      double c_y = (v_w_range[1] - v_v_range[0]) * factors[1];
      double factor = 
        c_y / (double)this->HistogramImageDescriptor->Height;
      int *y_ptr = hist_image_coords->GetPointer(0);
      int *y_end = y_ptr + hist_image_coords->GetNumberOfTuples();
      for (; y_ptr < y_end; ++c_x, ++y_ptr)
        {
        tk_cmd << c_x << " " << (c_y - factor * (*y_ptr)) << " ";
        }
      tk_cmd << endl;
      sprintf(color, "#%02x%02x%02x", 
              (int)(this->HistogramColor[0] * 255.0),
              (int)(this->HistogramColor[1] * 255.0),
              (int)(this->HistogramColor[2] * 255.0));
      tk_cmd << canv << " itemconfigure " 
             << vtkKWParameterValueFunctionEditor::HistogramTag 
             << " -fill " << color 
             << " -width " << this->HistogramPolyLineWidth << endl;
      }
    }
  else
    {
    if (has_hist_tag)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::HistogramTag 
             << endl;
      }
    }

  // Update the secondary histogram position (or delete if not needed anymore)

  if (this->SecondaryHistogram && 
      (!secondary_hist_is_image || secondary_hist_image))
    {
    if (!has_secondary_hist_tag)
      {
      if (secondary_hist_is_image)
        {
        tk_cmd << canv << " create image 0 0 -anchor nw -image " 
               << secondary_hist_img_name.c_str();
        }
      else
        {
        tk_cmd << canv << " create line 0 0 0 0";
        }
      tk_cmd << " -tags {" 
             << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
             << "}" << endl;
      if (this->CanvasBackgroundVisibility && this->CanvasVisibility)
        {
        tk_cmd << canv << " raise " 
               << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
               << " " << vtkKWParameterValueFunctionEditor::FrameBackgroundTag
               << endl;
        }
      }
    
    this->GetCanvasScalingFactors(factors);
    double c_x = 
      this->SecondaryHistogramImageDescriptor->Range[0] * factors[0];

    tk_cmd << canv << " coords " 
           << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag
           << " ";
    if (secondary_hist_is_image)
      {
      double c_y = factors[1] * (v_w_range[1] - v_v_range[1]);
      tk_cmd << c_x << " " << c_y << endl;
      }
    else
      {
      double c_y = (v_w_range[1] - v_v_range[0]) * factors[1];
      double factor = 
        c_y / (double)this->SecondaryHistogramImageDescriptor->Height;
      int *y_ptr = secondary_hist_image_coords->GetPointer(0);
      int *y_end = y_ptr + secondary_hist_image_coords->GetNumberOfTuples();
      for (; y_ptr < y_end; ++c_x, ++y_ptr)
        {
        tk_cmd << c_x << " " << (c_y - factor * (*y_ptr)) << " ";
        }
      tk_cmd << endl;
      sprintf(color, "#%02x%02x%02x", 
              (int)(this->SecondaryHistogramColor[0] * 255.0),
              (int)(this->SecondaryHistogramColor[1] * 255.0),
              (int)(this->SecondaryHistogramColor[2] * 255.0));
      tk_cmd << canv << " itemconfigure " 
             << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
             << " -fill " << color
             << " -width " << this->HistogramPolyLineWidth << endl;
      }
    }
  else
    {
    if (has_secondary_hist_tag)
      {
      tk_cmd << canv << " delete " 
             << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
             << endl;
      }
    }

  // Some re-ordering may be needed to show the polyline on top of the image

  if (!has_hist_tag && hist_is_image)
    {
    tk_cmd << canv << " lower " 
           << vtkKWParameterValueFunctionEditor::HistogramTag << " " 
           << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag << endl;
    }
  if (!has_secondary_hist_tag && secondary_hist_is_image)
    {
    tk_cmd << canv << " lower " 
           << vtkKWParameterValueFunctionEditor::SecondaryHistogramTag 
           << " " << vtkKWParameterValueFunctionEditor::HistogramTag << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::HasSelection()
{
  return (this->GetSelectedPoint() >= 0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectPoint(int id)
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize() ||
      this->GetSelectedPoint() == id)
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

    tk_cmd << canv << " addtag " 
           << vtkKWParameterValueFunctionEditor::SelectedTag 
           << " withtag p" <<  this->GetSelectedPoint() << endl;
    tk_cmd << canv << " addtag " 
           << vtkKWParameterValueFunctionEditor::SelectedTag 
           << " withtag t" <<  this->GetSelectedPoint() << endl;
    tk_cmd << "catch {" << canv << " raise " 
           << vtkKWParameterValueFunctionEditor::SelectedTag << " all}" 
           << endl;

    tk_cmd << ends;
    this->Script(tk_cmd.str());
    tk_cmd.rdbuf()->freeze(0);
    }

  // Draw the selected point accordingly and update its aspect
  
  this->RedrawSinglePointDependentElements(this->GetSelectedPoint());
  this->PackPointEntries();

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

    tk_cmd << canv << " dtag p" <<  this->GetSelectedPoint()
           << " " << vtkKWParameterValueFunctionEditor::SelectedTag << endl;
    tk_cmd << canv << " dtag t" <<  this->GetSelectedPoint()
           << " " << vtkKWParameterValueFunctionEditor::SelectedTag << endl;

    tk_cmd << ends;
    this->Script(tk_cmd.str());
    tk_cmd.rdbuf()->freeze(0);
    }

  // Deselect

  int old_selection = this->GetSelectedPoint();
  this->SelectedPoint = -1;

  // Redraw the point that used to be selected and update its aspect

  this->RedrawSinglePointDependentElements(old_selection);

  // Show the selected point description in the point label
  // Since nothing is selected, the expected side effect is to clear the
  // point label

  this->UpdatePointEntries(this->GetSelectedPoint());
  this->PackPointEntries();

  this->InvokeSelectionChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectNextPoint()
{
  if (this->HasSelection())
    {
    this->SelectPoint(this->GetSelectedPoint() == this->GetFunctionSize() - 1 
                      ? 0 : this->GetSelectedPoint() + 1);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SelectPreviousPoint()
{
  if (this->HasSelection())
    {
    this->SelectPoint(this->GetSelectedPoint() == 0  
                      ? this->GetFunctionSize() - 1 : this->GetSelectedPoint() - 1);
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

  return this->RemovePoint(this->GetSelectedPoint());
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
    else if (id < this->GetSelectedPoint())
      {
      this->SelectPoint(this->GetSelectedPoint() - 1);
      }
    else if (this->GetSelectedPoint() >= this->GetFunctionSize())
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
  int x, int y, int *id)
{
  if (!this->AddFunctionPointAtCanvasCoordinates(x, y, id))
    {
    return 0;
    }

  // Draw the points (or all the points if the index have changed)

  this->RedrawFunctionDependentElements();

  // If the point was inserted before the selection, shift the selection

  if (this->HasSelection() && *id <= this->GetSelectedPoint())
    {
    this->SelectPoint(this->GetSelectedPoint() + 1);
    }

  this->InvokePointAddedCommand(*id);
  this->InvokeFunctionChangedCommand();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::AddPointAtParameter(
  double parameter, int *id)
{
  if (!this->AddFunctionPointAtParameter(parameter, id))
    {
    return 0;
    }

  // Draw the points (or all the points if the index have changed)

  this->RedrawFunctionDependentElements();

  // If we the point was inserted before the selection, shift the selection

  if (this->HasSelection() && *id <= this->GetSelectedPoint())
    {
    this->SelectPoint(this->GetSelectedPoint() + 1);
    }

  this->InvokePointAddedCommand(*id);
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

  // Browse all editor's point, get their parameters, add them to our own
  // function (the values will be interpolated automatically)

  int new_id;
  for (int editor_id = 0; editor_id < editor_size; editor_id++)
    {
    this->MergePointFromEditor(editor, editor_id, &new_id);
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
int vtkKWParameterValueFunctionEditor::MergePointFromEditor(
  vtkKWParameterValueFunctionEditor *editor, int editor_id, int *new_id)
{
  double parameter, editor_parameter;
  if (editor && 
      editor->GetFunctionPointParameter(editor_id, &editor_parameter) &&
      (!this->GetFunctionPointParameter(editor_id, &parameter) ||
       editor_parameter != parameter))
    {
    return this->AddPointAtParameter(editor_parameter, new_id);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::CopyPointFromEditor(
  vtkKWParameterValueFunctionEditor *editor, int id)
{
  double parameter, editor_parameter;
  if (editor && editor->GetFunctionPointParameter(id, &editor_parameter) &&
      this->GetFunctionPointParameter(id, &parameter))
    {
    if (editor_parameter != parameter)
      {
      this->MoveFunctionPointToParameter(id, editor_parameter);
      }
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::CanvasHasTag(const char *tag, 
                                                    int *suffix,
                                                    vtkKWCanvas *canv)
{
  if (!canv)
    {
    canv = this->Canvas;
    }

  if (!canv->IsCreated())
    {
    return 0;
    }

  if (suffix)
    {
    return atoi(canv->Script(
                  "llength [%s find withtag %s%d]",
                  canv->GetWidgetName(), tag, *suffix));
    }

  return atoi(canv->Script(
                "llength [%s find withtag %s]",
                canv->GetWidgetName(), tag));
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
  this->UpdatePointEntries(this->GetSelectedPoint());

  this->RedrawSizeDependentElements();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetDisplayedWholeParameterRange(
  double range[2]) 
{ 
  this->SetDisplayedWholeParameterRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetDisplayedVisibleParameterRange(
  double range[2])
{ 
  this->GetDisplayedVisibleParameterRange(range[0], range[1]); 
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::MapParameterToDisplayedParameter(
  double p, double *displayed_p)
{
  if (this->DisplayedWholeParameterRange[0] !=
      this->DisplayedWholeParameterRange[1])
    {
    double d_p_w_delta = (this->DisplayedWholeParameterRange[1] - 
                          this->DisplayedWholeParameterRange[0]);
    double *p_w_range = this->GetWholeParameterRange();
    double p_w_delta = p_w_range[1] - p_w_range[0];
    double rel_p = (p - p_w_range[0]) / p_w_delta;
    *displayed_p = this->DisplayedWholeParameterRange[0] + rel_p * d_p_w_delta;
    }
  else
    {
    *displayed_p = p;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::MapDisplayedParameterToParameter(
  double displayed_p, double *p)
{
  if (this->DisplayedWholeParameterRange[0] !=
      this->DisplayedWholeParameterRange[1])
    {
    double d_p_w_delta = (this->DisplayedWholeParameterRange[1] - 
                          this->DisplayedWholeParameterRange[0]);
    double *p_w_range = this->GetWholeParameterRange();
    double p_w_delta = p_w_range[1] - p_w_range[0];
    double rel_displayed_p = 
      (displayed_p - this->DisplayedWholeParameterRange[0]) / d_p_w_delta;
    *p = p_w_range[0] + rel_displayed_p * p_w_delta;
    }
  else
    {
    *p = displayed_p;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetDisplayedVisibleParameterRange(
  double &r0, double &r1)
{
  this->MapParameterToDisplayedParameter(
    this->GetVisibleParameterRange()[0], &r0);
  this->MapParameterToDisplayedParameter(
    this->GetVisibleParameterRange()[1], &r1);
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::GetFunctionPointDisplayedParameter(
  int id, double *displayed_p)
{
  double parameter;
  if (!this->GetFunctionPointParameter(id, &parameter))
    {
    return 0;
    }

  this->MapParameterToDisplayedParameter(parameter, displayed_p);
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateRangeLabel()
{
  if (!this->IsCreated() || 
      !this->RangeLabel || 
      !this->RangeLabel->IsAlive() ||
      !(this->ParameterRangeLabelVisibility || 
        this->ValueRangeLabelVisibility))
    {
    return;
    }

  ostrstream ranges;
  int nb_ranges = 0;

  if (this->ParameterRangeLabelVisibility)
    {
    double param[2];
    this->GetDisplayedVisibleParameterRange(param[0], param[1]);
    char buffer[1024];
    sprintf(buffer, "[%g, %g]", param[0], param[1]);
    ranges << buffer;
    nb_ranges++;
    }

  double *value = GetVisibleValueRange();
  if (value && this->ValueRangeLabelVisibility)
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
  this->RangeLabel->SetText(ranges.str());
  ranges.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateParameterEntry(int id)
{
  if (!this->ParameterEntry || !this->HasFunction())
    {
    return;
    }

  double parameter;

  if (id < 0 || id >= this->GetFunctionSize() ||
      !this->GetFunctionPointParameter(id, &parameter))
    {
    this->ParameterEntry->SetEnabled(0);
    if (this->ParameterEntry->GetWidget())
      {
      this->ParameterEntry->GetWidget()->SetValue("");
      }
    return;
    }

  this->ParameterEntry->SetEnabled(
    this->FunctionPointParameterIsLocked(id) ? 0 : this->GetEnabled());

  this->MapParameterToDisplayedParameter(parameter, &parameter);

  if (this->ParameterEntryFormat)
    {
    char buffer[256];
    sprintf(buffer, this->ParameterEntryFormat, parameter);
    this->ParameterEntry->GetWidget()->SetValue(buffer);
    }
  else
    {
    this->ParameterEntry->GetWidget()->SetValueAsDouble(parameter);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ParameterEntryCallback(const char*)
{
  if (!this->ParameterEntry || !this->HasSelection())
    {
    return;
    }

  unsigned long mtime = this->GetFunctionMTime();

  double parameter = this->ParameterEntry->GetWidget()->GetValueAsDouble();

  this->MapDisplayedParameterToParameter(parameter, &parameter);

  this->MoveFunctionPointToParameter(this->GetSelectedPoint(), parameter);

  if (this->GetFunctionMTime() > mtime)
    {
    this->InvokePointChangedCommand(this->GetSelectedPoint());
    this->InvokeFunctionChangedCommand();
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::UpdateHistogramLogModeOptionMenu()
{
  if (this->HistogramLogModeOptionMenu && 
      this->HistogramLogModeOptionMenu->IsCreated())
    {
    vtkKWHistogram *hist = 
      this->Histogram ? this->Histogram : this->SecondaryHistogram;
    int log_mode = 1;
    if (hist)
      {
      log_mode = hist->GetLogMode();
      }
    vtkKWMenu *menu = this->HistogramLogModeOptionMenu->GetMenu();
    const char* img_opt = menu->GetItemOption(
      menu->GetIndexOfItem("Log."), "-image");
    if (img_opt && *img_opt)
      {
      ostrstream img_name;
      img_name << this->HistogramLogModeOptionMenu->GetWidgetName() 
               << ".img" << log_mode << ends;
      this->HistogramLogModeOptionMenu->SetValue(img_name.str());
      img_name.rdbuf()->freeze(0);
      }
    else
      {
      this->HistogramLogModeOptionMenu->SetValue(
        log_mode 
        ? ks_("Transfer Function Editor|Histogram|Logarithmic|Log.") 
        : ks_("Transfer Function Editor|Histogram|Linear|Lin."));
      }
    this->HistogramLogModeOptionMenu->SetEnabled(
      !hist ? 0 : this->GetEnabled());
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
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }
  
  // Make sure both editors have the same visible range from now

  b->SetVisibleParameterRange(this->GetVisibleParameterRange());

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent,
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent
    };

  b->AddObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizeVisibleParameterRange(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent,
      vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent
    };

  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizePoints(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }

  // Make sure they share the same points in the parameter space from now

  this->MergePointsFromEditor(b);
  b->MergePointsFromEditor(this);

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::PointChangingEvent,
      vtkKWParameterValueFunctionEditor::PointChangedEvent,
      vtkKWParameterValueFunctionEditor::PointRemovedEvent,
      vtkKWParameterValueFunctionEditor::FunctionChangedEvent
    };

  b->AddObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizePoints(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }

  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::PointChangingEvent,
      vtkKWParameterValueFunctionEditor::PointChangedEvent,
      vtkKWParameterValueFunctionEditor::PointRemovedEvent,
      vtkKWParameterValueFunctionEditor::FunctionChangedEvent
    };

  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizeSingleSelection(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }
  
  // Make sure only one of those editors has a selected point from now
  
  if (this->HasSelection())
    {
    b->ClearSelection();
    }
  else if (b->HasSelection())
    {
    this->ClearSelection();
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->AddObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizeSingleSelection(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand);

  this->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::SynchronizeSameSelection(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }
  
  // Make sure those editors have the same selected point from now
  
  if (this->HasSelection())
    {
    b->SelectPoint(this->GetSelectedPoint());
    }
  else if (b->HasSelection())
    {
    this->SelectPoint(b->GetSelectedPoint());
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->AddObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand2);

  this->AddObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand2);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::DoNotSynchronizeSameSelection(
  vtkKWParameterValueFunctionEditor *b)
{
  if (!b)
    {
    return 0;
    }
  
  int events[] = 
    {
      vtkKWParameterValueFunctionEditor::SelectionChangedEvent
    };
  
  b->RemoveObserversList(
    sizeof(events) / sizeof(int), events, this->SynchronizeCallbackCommand2);
  
  this->RemoveObserversList(
    sizeof(events) / sizeof(int), events, b->SynchronizeCallbackCommand2);
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ProcessSynchronizationEventsFunction(
  vtkObject *object,
  unsigned long event,
  void *clientdata,
  void *calldata)
{
  vtkKWParameterValueFunctionEditor *self =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(clientdata);
  if (self)
    {
    self->ProcessSynchronizationEvents(object, event, calldata);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents(
  vtkObject *caller,
  unsigned long event,
  void *calldata)
{
  vtkKWParameterValueFunctionEditor *pvfe =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(caller);
  
  int *point_id = reinterpret_cast<int *>(calldata);
  double *dargs = reinterpret_cast<double *>(calldata);
  double range[2];

  switch (event)
    {
    // Synchronize visible range
    
    case vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent:
    case vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent:
      pvfe->GetRelativeVisibleParameterRange(range);
      this->SetRelativeVisibleParameterRange(range);
      break;
      
    // Synchronize points
      
    case vtkKWParameterValueFunctionEditor::PointChangingEvent:
    case vtkKWParameterValueFunctionEditor::PointChangedEvent:
      this->CopyPointFromEditor(pvfe, *point_id);
      break;

    case vtkKWParameterValueFunctionEditor::PointRemovedEvent:
      this->RemovePointAtParameter(dargs[1]);
      break;

    case vtkKWParameterValueFunctionEditor::FunctionChangedEvent:
      this->MergePointsFromEditor(pvfe);
      break;

    // Synchronize Single selection

    case vtkKWParameterValueFunctionEditor::SelectionChangedEvent:
      if (pvfe->HasSelection())
        {
        this->ClearSelection();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ProcessSynchronizationEventsFunction2(
  vtkObject *object,
  unsigned long event,
  void *clientdata,
  void *calldata)
{
  vtkKWParameterValueFunctionEditor *self =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(clientdata);
  if (self)
    {
    self->ProcessSynchronizationEvents2(object, event, calldata);
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents2(
  vtkObject *caller,
  unsigned long event,
  void *vtkNotUsed(calldata))
{
  vtkKWParameterValueFunctionEditor *pvfe =
    reinterpret_cast<vtkKWParameterValueFunctionEditor *>(caller);
  
  switch (event)
    {
    // Synchronize Same selection

    case vtkKWParameterValueFunctionEditor::SelectionChangedEvent:
      if (pvfe->HasSelection())
        {
        this->SelectPoint(pvfe->GetSelectedPoint());
        }
      else
        {
        this->ClearSelection();
        }
      break;
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ConfigureCallback()
{
  static int in_configure_callback = 0;

  if (in_configure_callback)
    {
    return;
    }

  in_configure_callback = 1;

  this->Redraw();

  in_configure_callback = 0;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingCallback(
  double, double)
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangingCommand();

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedCallback(
  double, double)
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangedCommand();

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleValueRangeChangingCallback(
  double, double)
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleValueRangeChangedCallback(
  double, double)
{
  this->UpdateRangeLabel();
  this->Redraw();

  this->InvokeVisibleRangeChangedCommand();
}

//----------------------------------------------------------------------------
int
vtkKWParameterValueFunctionEditor::FindClosestItemWithTagAtCanvasCoordinates(
  int x, int y, int halo, const char *tag, int *c_x, int *c_y, char *found)
{
  if (!this->IsCreated() || halo < 0 || !tag || !c_x || !c_y)
    {
    return 0;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If we are out of the canvas, clamp the coordinates

  if (x < 0)
    {
    x = 0;
    }
  else if (x > this->CurrentCanvasWidth - 1)
    {
    x = this->CurrentCanvasWidth - 1;
    }

  if (y < 0)
    {
    y = 0;
    }
  else if (y > this->CurrentCanvasHeight - 1)
    {
    y = this->CurrentCanvasHeight - 1;
    }

  // Get the real canvas coordinates

  *c_x = atoi(this->Script("%s canvasx %d", canv, x));
  *c_y = atoi(this->Script("%s canvasy %d", canv, y));

  // Find the overlapping items

  const char *res = 
    this->Script(
      "%s find overlapping %d %d %d %d", 
      canv, *c_x - halo, *c_y - halo, *c_x + halo, *c_y + halo);

  vtksys_stl::vector<vtksys_stl::string> items;
  vtksys::SystemTools::Split(res, items, ' ');
  
  // For each item, check the tags, and see if we have a match

  vtksys_stl::vector<vtksys_stl::string>::iterator it = items.begin();
  vtksys_stl::vector<vtksys_stl::string>::iterator end = items.end();
  for (; it != end; it++)
    {
    res = this->Script("%s itemcget %s -tags", canv, (*it).c_str());
    vtksys_stl::vector<vtksys_stl::string> tags;
    vtksys::SystemTools::Split(res, tags, ' ');
    vtksys_stl::vector<vtksys_stl::string>::iterator match =
      vtksys_stl::find(tags.begin(), tags.end(), tag);
    if (match != tags.end())
      {
      strcpy(found, (*it).c_str());
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkKWParameterValueFunctionEditor::FindFunctionPointAtCanvasCoordinates(
  int x, int y, int *id, int *c_x, int *c_y)
{
  if (!this->IsCreated() || !this->HasFunction())
    {
    return 0;
    }

  char item[256];
  if (!this->FindClosestItemWithTagAtCanvasCoordinates(
        x, y, 3, vtkKWParameterValueFunctionEditor::PointTag, c_x, c_y, item))
    {
    return 0;
    }

  *id = -1;

  // Get its first tag, which should be a point tag (in
  // the form of ppid or tpid, ex: p0 or t1)

  const char *canv = this->Canvas->GetWidgetName();
  const char *tag = 
    this->Script("lindex [%s itemcget %s -tags] 0", canv, item);
  if (tag && *tag && (tag[0] == 't' || tag[0] == 'p') && isdigit(tag[1]))
    {
    *id = atoi(tag + 1);
    }

  return (*id < 0 || *id >= this->GetFunctionSize()) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::DoubleClickOnPointCallback(
  int x, int y)
{
  int id, c_x, c_y;

  // No point found

  if (!this->FindFunctionPointAtCanvasCoordinates(x, y, &id, &c_x, &c_y))
    {
    return;
    }

  // Select the point

  this->SelectPoint(id);

  // The first click in that double-click will trigger
  // StartInteractionCallback. At this point, weird behaviours have been
  // noticed. For example, if the DoubleClickOnPointCommand below
  // popups up a Color Chooser dialog, even if that dialog is modal, selecting
  // a color will trigger the MovePointCallback just as if the user was
  // still dragging the point it double-clicked on. To avoid that,
  // StartInteractionCallback sets InUserInteraction to 1 and
  // MovePointCallback does not anything if it is not set to 1. Therefore
  // set it to 0 right now to avoid triggering any user interaction involving
  // moving the point.

  this->InUserInteraction = 0;

  this->InvokeDoubleClickOnPointCommand(id);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::StartInteractionCallback(int x, int y)
{
  int id, c_x, c_y;

  // No point found, then let's add that point

  if (!this->FindFunctionPointAtCanvasCoordinates(x, y, &id, &c_x, &c_y))
    {
    if (!this->AddPointAtCanvasCoordinates(c_x, c_y, &id))
      {
      return;
      }
    }

  // Select the point (that was found or just added)

  this->SelectPoint(id);
  this->LastSelectionCanvasCoordinateX = c_x;
  this->LastSelectionCanvasCoordinateY = c_y;

  this->InUserInteraction = 1;

  // Invoke the commands/callbacks

  this->InvokeFunctionStartChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::MovePointCallback(
  int x, int y, int shift)
{
  if (!this->IsCreated() || !this->HasSelection() || !this->InUserInteraction)
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If we are out of the canvas by a given "delete" margin, warn that 
  // the point is going to be deleted (do not delete here now to give
  // the user a chance to recover)

  int warn_delete = 
    (this->FunctionPointCanBeRemoved(this->GetSelectedPoint()) &&
     (x < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
      x > this->CurrentCanvasWidth - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
      y < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
      y > this->CurrentCanvasHeight - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN));

  // If we are out of the canvas, clamp the coordinates

  if (x < 0)
    {
    x = 0;
    }
  else if (x > this->CurrentCanvasWidth - 1)
    {
    x = this->CurrentCanvasWidth - 1;
    }

  if (y < 0)
    {
    y = 0;
    }
  else if (y > this->CurrentCanvasHeight - 1)
    {
    y = this->CurrentCanvasHeight - 1;
    }

  // Get the real canvas coordinates

  int c_x = atoi(this->Script("%s canvasx %d", canv, x));
  int c_y = atoi(this->Script("%s canvasy %d", canv, y));

  // We assume we can not go before or beyond the previous or next point

  if (this->GetSelectedPoint() > 0)
    {
    int prev_x, prev_y;
    this->GetFunctionPointCanvasCoordinates(this->GetSelectedPoint() - 1, 
                                            &prev_x, &prev_y);
    if (c_x <= prev_x)
      {
      c_x = prev_x + 1;
      }
    }

  if (this->GetSelectedPoint() < this->GetFunctionSize() - 1)
    {
    int next_x, next_y;
    this->GetFunctionPointCanvasCoordinates(this->GetSelectedPoint() + 1,
                                            &next_x, &next_y);
    if (c_x >= next_x)
      {
      c_x = next_x - 1;
      }
    }

  // Are we constrained vertically or horizontally ?

  int move_h_only = this->FunctionPointValueIsLocked(this->GetSelectedPoint());
  int move_v_only = this->FunctionPointParameterIsLocked(this->GetSelectedPoint());

  if (shift)
    {
    if (this->LastConstrainedMove == 
        vtkKWParameterValueFunctionEditor::ConstrainedMoveFree)
      {
      if (fabs((double)(c_x - this->LastSelectionCanvasCoordinateX)) >
          fabs((double)(c_y - this->LastSelectionCanvasCoordinateY)))
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
      c_y = this->LastSelectionCanvasCoordinateY;
      }
    else if (this->LastConstrainedMove == 
             vtkKWParameterValueFunctionEditor::ConstrainedMoveVertical)
      {
      move_v_only = 1;
      c_x = this->LastSelectionCanvasCoordinateX;
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
    this->Canvas->SetConfigurationOption("-cursor", cursor);
    }

  // Now update the point given those coords, and update the info label

  this->MoveFunctionPointToCanvasCoordinates(
    this->GetSelectedPoint(), c_x, c_y);

  // Invoke the commands/callbacks

  this->InvokePointChangingCommand(this->GetSelectedPoint());
  this->InvokeFunctionChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::EndInteractionCallback(int x, int y)
{
  if (!this->HasSelection() || !this->InUserInteraction)
    {
    return;
    }

  this->InUserInteraction = 0;

  // Invoke the commands/callbacks
  // If we are out of the canvas by a given margin, delete the point

  if (this->FunctionPointCanBeRemoved(this->GetSelectedPoint()) &&
      (x < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
       x > this->CurrentCanvasWidth - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
       y < -VTK_KW_PVFE_CANVAS_DELETE_MARGIN ||
       y > this->CurrentCanvasHeight - 1 + VTK_KW_PVFE_CANVAS_DELETE_MARGIN))
    {
    this->RemovePoint(this->GetSelectedPoint());
    }
  else
    {
    this->InvokePointChangedCommand(this->GetSelectedPoint());
    this->InvokeFunctionChangedCommand();
    }

  // Remove any interaction icon

  if (this->Canvas && this->ChangeMouseCursor)
    {
    this->Canvas->SetConfigurationOption("-cursor", NULL);
    }

  // Redraw the selection in case it had a special interaction color

  if (this->SelectedPointColorInInteraction[0] >= 0.0 &&
      this->SelectedPointColorInInteraction[1] >= 0.0 &&
      this->SelectedPointColorInInteraction[2] >= 0.0)
    {
    this->RedrawPoint(this->GetSelectedPoint());
    }
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor
::ParameterCursorStartInteractionCallback( int vtkNotUsed(x) )
{
  if (this->Canvas && this->ChangeMouseCursor)
    {
    this->Canvas->SetConfigurationOption("-cursor", "hand2");
    }
}

//----------------------------------------------------------------------------
void 
vtkKWParameterValueFunctionEditor::ParameterCursorEndInteractionCallback()
{
  if (this->Canvas && this->ChangeMouseCursor)
    {
    this->Canvas->SetConfigurationOption("-cursor", NULL);
    }

  this->InvokeParameterCursorMovedCommand(this->GetParameterCursorPosition());
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::ParameterCursorMoveCallback(int x)
{
  if (!this->IsCreated())
    {
    return;
    }

  // If we are out of the canvas, clamp the coordinates

  if (x < 0)
    {
    x = 0;
    }
  else if (x > this->CurrentCanvasWidth - 1)
    {
    x = this->CurrentCanvasWidth - 1;
    }

  // Get the real canvas coordinates

  int c_x = atoi(
    this->Script("%s canvasx %d", this->Canvas->GetWidgetName(), x));

  // Get the corresponding parameter and move

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);
  if (factors[0])
    {
    this->SetParameterCursorPosition((double)c_x / factors[0]);
    }

  // Invoke the commands/callbacks

  this->InvokeParameterCursorMovingCommand(this->GetParameterCursorPosition());
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
  this->InvokeHistogramLogModeChangedCommand(mode);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ParameterRangeVisibility: "
     << (this->ParameterRangeVisibility ? "On" : "Off") << endl;
  os << indent << "ValueRangeVisibility: "
     << (this->ValueRangeVisibility ? "On" : "Off") << endl;
  os << indent << "ParameterRangeLabelVisibility: "
     << (this->ParameterRangeLabelVisibility ? "On" : "Off") << endl;
  os << indent << "ValueRangeLabelVisibility: "
     << (this->ValueRangeLabelVisibility ? "On" : "Off") << endl;
  os << indent << "RangeLabelPosition: " << this->RangeLabelPosition << endl;
  os << indent << "PointEntriesPosition: " << this->PointEntriesPosition << endl;
  os << indent << "ParameterEntryVisibility: "
     << (this->ParameterEntryVisibility ? "On" : "Off") << endl;
  os << indent << "PointEntriesVisibility: "
     << (this->PointEntriesVisibility ? "On" : "Off") << endl;
  os << indent << "UserFrameVisibility: "
     << (this->UserFrameVisibility ? "On" : "Off") << endl;
  os << indent << "CanvasHeight: "<< this->RequestedCanvasHeight << endl;
  os << indent << "CurrentCanvasHeight: "<< this->CurrentCanvasHeight << endl;
  os << indent << "CanvasWidth: "<< this->RequestedCanvasWidth << endl;
  os << indent << "CurrentCanvasWidth: "<< this->CurrentCanvasWidth << endl;
  os << indent << "ExpandCanvasWidth: "
     << (this->ExpandCanvasWidth ? "On" : "Off") << endl;
  os << indent << "PointRadiusX: "<< this->PointRadiusX << endl;
  os << indent << "PointRadiusY: "<< this->PointRadiusY << endl;
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
  os << indent << "SelectedPointText: "
     << (this->SelectedPointText ? this->SelectedPointText : "(None)") << endl;
  os << indent << "SelectedPointRadius: " 
     << this->SelectedPointRadius << endl;
  os << indent << "DisableCommands: "
     << (this->DisableCommands ? "On" : "Off") << endl;
  os << indent << "LockEndPointsParameter: "
     << (this->LockEndPointsParameter ? "On" : "Off") << endl;
  os << indent << "LockPointsParameter: "
     << (this->LockPointsParameter ? "On" : "Off") << endl;
  os << indent << "LockPointsValue: "
     << (this->LockPointsValue ? "On" : "Off") << endl;
  os << indent << "RescaleBetweenEndPoints: "
     << (this->RescaleBetweenEndPoints ? "On" : "Off") << endl;
  os << indent << "PointMarginToCanvas: " << this->PointMarginToCanvas << endl;
  os << indent << "CanvasOutlineStyle: " << this->CanvasOutlineStyle << endl;
  os << indent << "ParameterCursorInteractionStyle: " << this->ParameterCursorInteractionStyle << endl;
  os << indent << "DisableAddAndRemove: "
     << (this->DisableAddAndRemove ? "On" : "Off") << endl;
  os << indent << "ChangeMouseCursor: "
     << (this->ChangeMouseCursor ? "On" : "Off") << endl;
  os << indent << "SelectedPoint: "<< this->GetSelectedPoint() << endl;
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
  os << indent << "SelectedPointColorInInteraction: ("
     << this->SelectedPointColorInInteraction[0] << ", " 
     << this->SelectedPointColorInInteraction[1] << ", " 
     << this->SelectedPointColorInInteraction[2] << ")" << endl;
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
  os << indent << "FunctionLineVisibility: "
     << (this->FunctionLineVisibility ? "On" : "Off") << endl;
  os << indent << "CanvasVisibility: "
     << (this->CanvasVisibility ? "On" : "Off") << endl;
  os << indent << "PointIndexVisibility: "
     << (this->PointIndexVisibility ? "On" : "Off") << endl;
  os << indent << "PointVisibility: "
     << (this->PointVisibility ? "On" : "Off") << endl;
  os << indent << "PointGuidelineVisibility: "
     << (this->PointGuidelineVisibility ? "On" : "Off") << endl;
  os << indent << "SelectedPointIndexVisibility: "
     << (this->SelectedPointIndexVisibility ? "On" : "Off") << endl;
  os << indent << "HistogramLogModeOptionMenuVisibility: "
     << (this->HistogramLogModeOptionMenuVisibility ? "On" : "Off") << endl;
  os << indent << "ParameterCursorVisibility: "
     << (this->ParameterCursorVisibility ? "On" : "Off") << endl;
  os << indent << "DisplayedWholeParameterRange: ("
     << this->DisplayedWholeParameterRange[0] << ", " 
     << this->DisplayedWholeParameterRange[1] << ")" << endl;
  os << indent << "PointStyle: " << this->PointStyle << endl;
  os << indent << "FirstPointStyle: " << this->FirstPointStyle << endl;
  os << indent << "LastPointStyle: " << this->LastPointStyle << endl;
  os << indent << "FunctionLineStyle: " << this->FunctionLineStyle << endl;
  os << indent << "FunctionLineWidth: " << this->FunctionLineWidth << endl;
  os << indent << "HistogramPolyLineWidth: " << this->HistogramPolyLineWidth << endl;
  os << indent << "ParameterCursorPosition: " 
     << this->ParameterCursorPosition << endl;
  os << indent << "PointGuidelineStyle: " << this->PointGuidelineStyle << endl;
  os << indent << "PointOutlineWidth: " << this->PointOutlineWidth << endl;
  os << indent << "PointPositionInValueRange: " << this->PointPositionInValueRange << endl;
  os << indent << "ParameterRangePosition: " << this->ParameterRangePosition << endl;
  os << indent << "PointColorStyle: " << this->PointColorStyle << endl;
  os << indent << "CanvasOutlineVisibility: "
     << (this->CanvasOutlineVisibility ? "On" : "Off") << endl;
  os << indent << "CanvasBackgroundVisibility: "
     << (this->CanvasBackgroundVisibility ? "On" : "Off") << endl;
  os << indent << "ParameterTicksVisibility: "
     << (this->ParameterTicksVisibility ? "On" : "Off") << endl;
  os << indent << "ValueTicksVisibility: "
     << (this->ValueTicksVisibility ? "On" : "Off") << endl;
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
