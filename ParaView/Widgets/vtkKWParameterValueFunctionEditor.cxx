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
#include "vtkKWParameterValueFunctionEditor.h"

#include "vtkCallbackCommand.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWIcon.h"
#include "vtkKWImageLabel.h"
#include "vtkKWLabeledLabel.h"
#include "vtkKWRange.h"
#include "vtkKWTkUtilities.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkKWParameterValueFunctionEditor, "1.14");

#define VTK_KW_RANGE_POINT_RADIUS_MIN    2

#define VTK_KW_RANGE_CANVAS_BORDER        1
#define VTK_KW_RANGE_CANVAS_WIDTH_MIN     10
#define VTK_KW_RANGE_CANVAS_HEIGHT_MIN    10
#define VTK_KW_RANGE_CANVAS_DELETE_MARGIN 35

#define VTK_KW_RANGE_POINT_TAG            "point"
#define VTK_KW_RANGE_SELECTED_POINT_TAG   "spoint"
#define VTK_KW_RANGE_LINE_TAG             "line"

#define VTK_KW_RANGE_NB_ICONS             5

// For some reasons, the end-point of a line/rectangle is not drawn on Win32. 
// Comply with that.

#ifndef _WIN32
#define LSTRANGE 0
#else
#define LSTRANGE 1
#endif
#define RSTRANGE 1

//----------------------------------------------------------------------------
vtkKWParameterValueFunctionEditor::vtkKWParameterValueFunctionEditor()
{
  int i;

  this->HideParameterRange      = 0;
  this->HideValueRange          = 0;
  this->CanvasHeight            = 50;
  this->CanvasWidth             = 0;
  this->LockEndPointsParameter  = 0;
  this->DisableAddAndRemove     = 0;
  this->PointRadius             = 4;
  this->SelectedPointRadius     = 1.45;
  this->DisableCommands         = 0;
  this->SelectedPoint           = -1;

  this->PointColor[0]           = 1.0;
  this->PointColor[1]           = 1.0;
  this->PointColor[2]           = 1.0;

  this->SelectedPointColor[0]   = 0.59;
  this->SelectedPointColor[1]   = 0.63;
  this->SelectedPointColor[2]   = 0.82;

  this->ComputePointColorFromValue  = 0;

  this->PointAddedCommand           = NULL;
  this->PointMovingCommand          = NULL;
  this->PointMovedCommand           = NULL;
  this->PointRemovedCommand         = NULL;
  this->SelectionChangedCommand     = NULL;
  this->FunctionChangedCommand      = NULL;
  this->FunctionChangingCommand     = NULL;
  this->VisibleRangeChangedCommand  = NULL;
  this->VisibleRangeChangingCommand = NULL;

  this->Canvas                  = vtkKWWidget::New();
  this->ParameterRange          = vtkKWRange::New();
  this->ValueRange              = vtkKWRange::New();
  this->TitleFrame              = vtkKWFrame::New();
  this->InfoFrame               = vtkKWFrame::New();
  this->RangeLabel              = vtkKWLabel::New();
  this->PointLabel              = vtkKWLabeledLabel::New();

  this->Icons                   = new vtkKWImageLabel* [VTK_KW_RANGE_NB_ICONS];
  for (i = 0; i < VTK_KW_RANGE_NB_ICONS; i++)
    {
    this->Icons[i]              = vtkKWImageLabel::New();
    }

  this->LastRedrawCanvasElementsTime      = 0;
  this->LastRelativeVisibleParameterRange = 0.0;
  this->LastRelativeVisibleValueRange     = 0.0;

  this->LastSelectCanvasCoordinates[0]    = 0;
  this->LastSelectCanvasCoordinates[1]    = 0;
  this->LastConstrainedMove               = CONSTRAINED_MOVE_FREE;

  // Synchronization callbacks
  
  this->SynchronizeCallbackCommand = vtkCallbackCommand::New();
  this->SynchronizeCallbackCommand->SetClientData(this); 
  this->SynchronizeCallbackCommand->SetCallback(
    vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents);

  this->SynchronizeCallbackCommand2 = vtkCallbackCommand::New();
  this->SynchronizeCallbackCommand2->SetClientData(this); 
  this->SynchronizeCallbackCommand2->SetCallback(
    vtkKWParameterValueFunctionEditor::ProcessSynchronizationEvents2);
}

//----------------------------------------------------------------------------
vtkKWParameterValueFunctionEditor::~vtkKWParameterValueFunctionEditor()
{
  int i;

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

  if (this->TitleFrame)
    {
    this->TitleFrame->Delete();
    this->TitleFrame = NULL;
    }

  if (this->InfoFrame)
    {
    this->InfoFrame->Delete();
    this->InfoFrame = NULL;
    }

  if (this->PointLabel)
    {
    this->PointLabel->Delete();
    this->PointLabel = NULL;
    }

  if (this->RangeLabel)
    {
    this->RangeLabel->Delete();
    this->RangeLabel = NULL;
    }

  for (i = 0; i < VTK_KW_RANGE_NB_ICONS; i++)
    {
    this->Icons[i]->Delete();
    this->Icons[i] = NULL;
    }
  delete [] this->Icons;

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
          !this->FunctionPointParameterIsLocked(id));
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
int vtkKWParameterValueFunctionEditor::GetFunctionPointColor(
  int id, float rgb[3])
{
  if (!this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
    {
    return 0;
    }

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

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Create(vtkKWApplication *app, 
                                               const char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("ParameterValueFunctionEditor already created");
    return;
    }

  this->SetApplication(app);

  // Create the container frame

  this->Script("frame %s -relief flat -bd 0 -highlightthickness 0 %s", 
               this->GetWidgetName(), args ? args : "");

  // Create the canvas

  this->Canvas->SetParent(this);
  this->Canvas->Create(
    app, 
    "canvas", 
    "-bg #D4D4D4 -highlightthickness 0 -relief solid -width 0");

  this->Script("%s config -height %d -bd %d",
               this->Canvas->GetWidgetName(), 
               this->CanvasHeight, VTK_KW_RANGE_CANVAS_BORDER);

  this->Script("bind %s <Configure> {%s ConfigureCallback}",
               this->Canvas->GetWidgetName(), this->GetTclName());

  // Create the ranges

  this->ParameterRange->SetParent(this);
  this->ParameterRange->Create(app);
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

  this->ValueRange->SetParent(this);
  this->ValueRange->Create(app);
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

  // Create the title frame

  this->TitleFrame->SetParent(this);
  this->TitleFrame->Create(app, "");

  // Create the label (i.e. title)

  this->Label->SetParent(this->TitleFrame);
  this->Label->Create(app, "-anchor w -bd 0");

  // Create the range label

  this->RangeLabel->SetParent(this->TitleFrame);
  this->RangeLabel->Create(app, "-bd 0");

  // Create the info frame

  this->InfoFrame->SetParent(this);
  this->InfoFrame->Create(app, "");

  // Create the point label

  this->PointLabel->SetParent(this->InfoFrame);
  this->PointLabel->Create(app, "");
  this->PointLabel->SetLabelAnchor(vtkKWWidget::ANCHOR_W);
  this->Script("%s config -bd 0", 
               this->PointLabel->GetLabel()->GetWidgetName());
  this->Script("%s config -bd 0", 
               this->PointLabel->GetLabel2()->GetWidgetName());

  // Create some icons

  for (int i = 0; i < VTK_KW_RANGE_NB_ICONS; i++)
    {
    this->Icons[i]->Create(app, "");
    }

  this->Icons[ICON_AXES]->SetImageData(vtkKWIcon::ICON_AXES);
  this->Icons[ICON_MOVE]->SetImageData(vtkKWIcon::ICON_MOVE);
  this->Icons[ICON_MOVE_H]->SetImageData(vtkKWIcon::ICON_MOVE_H);
  this->Icons[ICON_MOVE_V]->SetImageData(vtkKWIcon::ICON_MOVE_V);
  this->Icons[ICON_TRASHCAN]->SetImageData(vtkKWIcon::ICON_TRASHCAN);

  // Set the bindings

  this->Bind();

  // Pack the widget

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::Update()
{
  this->UpdateEnableState();

  this->UpdateRangeLabelWithRange();

  this->RedrawCanvas();
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
               0              1 2
         +-------------------------
        0|     T              I
        1|     [--------------] V
        2|     P      
  */

  // We need a 3x3 grid

  // Title frame (T)

  tk_cmd << "grid " << this->TitleFrame->GetWidgetName() 
         << " -row 0 -column 0 -stick ew -ipady 2" << endl;

  this->TitleFrame->UnpackChildren();

  // Label (i.e. title)

  if (this->ShowLabel)
    {
    tk_cmd << "pack " << this->Label->GetWidgetName() 
           << " -side left -fill x -padx 0" << endl;
    tk_cmd << this->Label->GetWidgetName() << " config -anchor w" << endl;
    }

  // Range label

  tk_cmd << "pack " << this->RangeLabel->GetWidgetName() 
         << " -side left -fill x -padx 0" << endl;
  tk_cmd << this->RangeLabel->GetWidgetName() << " config -anchor w" << endl;
  
  // Canvas ([------])

  tk_cmd << "grid " << this->Canvas->GetWidgetName() 
         << " -row 1 -column 0 -columnspan 2 -sticky news" << endl;

  // Ranges (P, V)

  if (!this->HideParameterRange)
    {
    tk_cmd << "grid " << this->ParameterRange->GetWidgetName() 
           << " -row 2 -column 0 -columnspan 2 -sticky ew -pady 2" << endl;
    }

  if (!this->HideValueRange)
    {
    tk_cmd << "grid " << this->ValueRange->GetWidgetName() 
           << " -row 1 -column 2 -sticky ns -padx 2" << endl;
    }

  // Info frame (I)

  tk_cmd << "grid " << this->InfoFrame->GetWidgetName() 
         << " -row 0 -column 1 -stick ew -ipady 2" << endl;
  
  this->InfoFrame->UnpackChildren();

  // Point label

  tk_cmd << "pack " << this->PointLabel->GetWidgetName() 
         << " -side left -expand y -fill x" << endl;
  
  // Make sure it will resize properly

  tk_cmd << "grid rowconfigure " 
         << this->GetWidgetName() << " 1 -weight 1" << endl;

  tk_cmd << "grid columnconfigure " 
         << this->GetWidgetName() << " 0 -weight 1" << endl;

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

    tk_cmd << canv << " bind " << VTK_KW_RANGE_POINT_TAG
           << " <B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 0}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_RANGE_POINT_TAG
           << " <Shift-B1-Motion> {" << this->GetTclName() 
           << " MovePointCallback %%x %%y 1}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_RANGE_POINT_TAG 
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
    
    tk_cmd << canv << " bind " << VTK_KW_RANGE_POINT_TAG 
           << " <B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_RANGE_POINT_TAG 
           << " <Shift-B1-Motion> {}" << endl;

    tk_cmd << canv << " bind " << VTK_KW_RANGE_POINT_TAG 
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
float* vtkKWParameterValueFunctionEditor::GetWholeParameterRange()
{
  return this->ParameterRange->GetWholeRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeParameterRange(
  float r0, float r1)
{
  this->ParameterRange->SetWholeRange(r0, r1);

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
float* vtkKWParameterValueFunctionEditor::GetVisibleParameterRange()
{
  return this->ParameterRange->GetRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleParameterRange(
  float r0, float r1)
{
  this->ParameterRange->SetRange(r0, r1); 

  // VisibleParameterRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleParameterRange(
  float &r0, float &r1)
{
  this->ParameterRange->GetRelativeRange(r0, r1);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRelativeVisibleParameterRange(
  float r0, float r1)
{
  this->ParameterRange->SetRelativeRange(r0, r1);

  // VisibleParameterRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHideParameterRange(int arg)
{
  if (this->HideParameterRange == arg)
    {
    return;
    }

  this->HideParameterRange = arg;

  this->Modified();

  this->Pack();
  this->UpdateRangeLabelWithRange();
}

//----------------------------------------------------------------------------
float* vtkKWParameterValueFunctionEditor::GetWholeValueRange()
{
  return this->ValueRange->GetWholeRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetWholeValueRange(float r0, float r1)
{
  this->ValueRange->SetWholeRange(r0, r1);

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
float* vtkKWParameterValueFunctionEditor::GetVisibleValueRange()
{
  return this->ValueRange->GetRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetVisibleValueRange(
  float r0, float r1)
{
  this->ValueRange->SetRange(r0, r1);

  // VisibleValueRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::GetRelativeVisibleValueRange(
  float &r0, float &r1)
{
  this->ValueRange->GetRelativeRange(r0, r1);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetRelativeVisibleValueRange(
  float r0, float r1)
{
  this->ValueRange->SetRelativeRange(r0, r1);

  // VisibleValueRangeChangingCallback is invoked automatically 
  // by the line above
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetHideValueRange(int arg)
{
  if (this->HideValueRange == arg)
    {
    return;
    }

  this->HideValueRange = arg;

  this->Modified();

  this->Pack();
  this->UpdateRangeLabelWithRange();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetCanvasHeight(int arg)
{
  if (this->CanvasHeight == arg || arg < VTK_KW_RANGE_CANVAS_HEIGHT_MIN)
    {
    return;
    }

  this->CanvasHeight = arg;

  this->Modified();

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointRadius(int arg)
{
  if (this->PointRadius == arg || arg < VTK_KW_RANGE_POINT_RADIUS_MIN)
    {
    return;
    }

  this->PointRadius = arg;

  this->Modified();

  this->RedrawCanvasElements();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointRadius(float arg)
{
  if (this->SelectedPointRadius == arg || arg < 0.0)
    {
    return;
    }

  this->SelectedPointRadius = arg;

  this->Modified();

  this->RedrawCanvasPoint(this->SelectedPoint);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetPointColor(
  float r, float g, float b)
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

  this->RedrawCanvas();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::SetSelectedPointColor(
  float r, float g, float b)
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

  this->RedrawCanvasPoint(this->SelectedPoint);
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

  this->RedrawCanvasElements();
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
  int id, float parameter)
{
  ostrstream param_str;
  param_str << parameter << ends;
  this->InvokePointCommand(this->PointRemovedCommand, id, param_str.str());
  param_str.rdbuf()->freeze(0);

  float fargs[2];
  fargs[0] = id;
  fargs[1] = parameter;

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::PointRemovedEvent, fargs);
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
void vtkKWParameterValueFunctionEditor::SetObjectMethodCommand(
  char **command, 
  vtkKWObject *object, 
  const char *method)
{
  if (*command)
    {
    delete [] *command;
    *command = NULL;
    }

  if (!object)
    {
    return;
    }

  ostrstream command_str;
  command_str << object->GetTclName() << " " << method << ends;
  *command = command_str.str();
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

  if (this->TitleFrame)
    {
    this->TitleFrame->SetEnabled(this->Enabled);
    }

  if (this->InfoFrame)
    {
    this->InfoFrame->SetEnabled(this->Enabled);
    }

  if (this->RangeLabel)
    {
    this->RangeLabel->SetEnabled(this->Enabled);
    }

  if (this->PointLabel)
    {
    this->PointLabel->SetEnabled(this->Enabled);
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

  if (this->PointLabel)
    {
    this->PointLabel->SetBalloonHelpString(string);
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

  if (this->PointLabel)
    {
    this->PointLabel->SetBalloonHelpJustification(j);
    }
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
    float c_x1, c_y1, c_x2, c_y2;
    if (sscanf(this->Script("%s coords %d", canv, item_id), 
               "%f %f %f %f", 
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
  float *p_v_range = this->GetVisibleParameterRange();
  factors[0] = (double)(this->CanvasWidth - 1.0) / 
    (double)(p_v_range[1] - p_v_range[0]);

  float *v_v_range = this->GetVisibleValueRange();
  factors[1] = (double)(this->CanvasHeight - 1.0) / 
    (double)(v_v_range[1] - v_v_range[0]);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawCanvas()
{
  if (!this->IsCreated() || !this->Canvas || !this->Canvas->IsAlive())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  ostrstream tk_cmd;

  // Get the new canvas size

  int old_c_width = atoi(this->Script("%s cget -width", canv));
  int old_c_height = atoi(this->Script("%s cget -height", canv));

  this->CanvasWidth = atoi(this->Script("winfo width %s", canv)) 
    - 2 * VTK_KW_RANGE_CANVAS_BORDER;
  if (this->CanvasWidth < VTK_KW_RANGE_CANVAS_WIDTH_MIN)
    {
    this->CanvasWidth = VTK_KW_RANGE_CANVAS_WIDTH_MIN;
    }

  tk_cmd << canv << " configure "
         << " -width " << this->CanvasWidth 
         << " -height " << this->CanvasHeight << endl;

  // In that visible area, we must fit the visible parameter in the
  // width dimension, and the visible value range in the height dimension.
  // Get the corresponding scaling factors.

  double factors[2] = {0.0, 0.0};
  this->GetCanvasScalingFactors(factors);

  // Compute the starting point for the scrollregion.
  // (note that the y axis is inverted)

  float *p_v_range = this->GetVisibleParameterRange();
  double c_x = factors[0] * (double)p_v_range[0];

  float *v_w_range = this->GetWholeValueRange();
  float *v_v_range = this->GetVisibleValueRange();
  double c_y = factors[1] * (double)(v_w_range[1] - v_v_range[1]);

  tk_cmd << canv << " configure "
         << " -scrollregion {" 
         << vtkMath::Round(c_x) << " " << vtkMath::Round(c_y) << " " 
         << vtkMath::Round(c_x + this->CanvasWidth - 1) << " " 
         << vtkMath::Round(c_y + this->CanvasHeight - 1) << "}" << endl;

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // If the canvas has been resized,
  // or if the visible range has changed (i.e. if the relative size of the
  // visible range compared to the whole range has changed significantly)
  // or if the function has changed, then update the points

  float *p_w_range = this->GetWholeParameterRange();

  double p_v_rel = fabs((double)(p_v_range[1] - p_v_range[0]) / 
                        (double)(p_w_range[1] - p_w_range[0]));
  double v_v_rel = fabs((double)(v_v_range[1] - v_v_range[0]) / 
                        (double)(v_w_range[1] - v_w_range[0]));

  if (old_c_width != this->CanvasWidth || old_c_height != this->CanvasHeight ||
      fabs(p_v_rel - this->LastRelativeVisibleParameterRange) > 0.001 ||
      fabs(v_v_rel - this->LastRelativeVisibleValueRange) > 0.001 ||
      (!this->HasFunction() ||
       this->GetFunctionMTime() > this->LastRedrawCanvasElementsTime))
    {
    this->RedrawCanvasElements();
    }

  this->LastRelativeVisibleParameterRange = p_v_rel;
  this->LastRelativeVisibleValueRange = v_v_rel;
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::RedrawCanvasPoint(int id, 
                                                          ostrstream *tk_cmd)
{
  if (!this->IsCreated() || 
      !this->HasFunction() || id < 0 || id >= this->GetFunctionSize())
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
    r = (int)ceil((float)r * this->SelectedPointRadius);
    }

  // Eventually create the point and the line to the previous point

  if (!this->CanvasHasTag("p", &id))
    {
    *tk_cmd << canv << " create oval 0 0 0 0 " 
            << "-tag {p" << id << " " << VTK_KW_RANGE_POINT_TAG << "}\n";

    if (id > 0)
      {
      *tk_cmd << canv << " create line 0 0 0 0 " 
              << "-tag {l" << id << " " << VTK_KW_RANGE_LINE_TAG << "}\n";
      *tk_cmd << canv << " lower l" << id << " p" << id - 1 << endl;
      }
    }

  // Update the point and line coordinates

  *tk_cmd << canv << " coords p" << id << " "
          << x - r << " " << y - r << " " << x + r << " " << y + r << endl;

  if (id > 0)
    {
    int prev_x, prev_y;
    this->GetFunctionPointCanvasCoordinates(id - 1, prev_x, prev_y);
    *tk_cmd << canv << " coords l" << id << " "
            << prev_x << " " << prev_y << " " << x << " " << y << endl;
    }
  if (id < this->GetFunctionSize() - 1)
    {
    int next_x, next_y;
    this->GetFunctionPointCanvasCoordinates(id + 1, next_x, next_y);
    *tk_cmd << canv << " coords l" << id + 1 << " "
            << x << " " << y << " " << next_x << " " << next_y << endl;
    }

  // Update the point color

  float rgb[3];
  if (this->GetFunctionPointColor(id, rgb))
    {
    char color[10];
    sprintf(color, "#%02x%02x%02x", 
            (int)(rgb[0]*255.0), (int)(rgb[1]*255.0), (int)(rgb[2]*255.0));
    *tk_cmd << canv << " itemconfigure p" << id
            << " -outline black -fill " << color << endl;
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
void vtkKWParameterValueFunctionEditor::RedrawCanvasElements()
{
  if (!this->IsCreated() || !this->Canvas || !this->Canvas->IsAlive())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If no function, or empty, remove everything

  if (!this->HasFunction() || !this->GetFunctionSize())
    {
    this->Script("%s delete all", canv);
    return;
    }

  // Are we going to create or delete points ?

  int c_nb_points = this->CanvasHasTag(VTK_KW_RANGE_POINT_TAG);
  int nb_points_changed = (c_nb_points != this->GetFunctionSize());

  // Try to save the selection before (eventually) creating new points

  int s_x = 0, s_y = 0;
  if (nb_points_changed && this->HasSelection())
    {
    int item_id = atoi(
      this->Script("%s find withtag %s",canv,VTK_KW_RANGE_SELECTED_POINT_TAG));
    this->GetCanvasItemCenter(item_id, s_x, s_y);
    }

  // Create the points 

  ostrstream tk_cmd;

  int i, nb_points = this->GetFunctionSize();
  for (i = 0; i < nb_points; i++)
    {
    this->RedrawCanvasPoint(i, &tk_cmd);
    }

  // Delete the extra points

  c_nb_points = this->CanvasHasTag(VTK_KW_RANGE_POINT_TAG);
  for (i = nb_points; i < c_nb_points; i++)
    {
    tk_cmd << canv << " delete p" << i << " l" << i << endl;
    }

  // Update the line aspect

  tk_cmd << canv << " itemconfigure " << VTK_KW_RANGE_LINE_TAG
         << " -fill black -width 2 " << endl;

  // Execute all of this

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  this->LastRedrawCanvasElementsTime = this->GetFunctionMTime();

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

    this->Script("%s addtag %s withtag p%d",
                 canv, VTK_KW_RANGE_SELECTED_POINT_TAG, this->SelectedPoint);
    this->Script("%s raise %s all",
                 canv, VTK_KW_RANGE_SELECTED_POINT_TAG);
    }

  // Draw the selected point accordingly and update its aspect
  
  this->RedrawCanvasPoint(this->SelectedPoint);

  // Show the selected point description in the point label

  this->UpdatePointLabelWithFunctionPoint(this->SelectedPoint);

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

    this->Script("%s dtag p%d %s",
                 canv, this->SelectedPoint, VTK_KW_RANGE_SELECTED_POINT_TAG);
    }

  // Deselect

  int old_selection = this->SelectedPoint;
  this->SelectedPoint = -1;

  // Redraw the point that used to be selected and update its aspect

  this->RedrawCanvasPoint(old_selection);

  // Show the selected point description in the point label
  // Since nothing is selected, the expect side effect is to clear the
  // point label

  this->UpdatePointLabelWithFunctionPoint(this->SelectedPoint);

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
  float parameter;
  if (!this->GetFunctionPointParameter(id, parameter) ||
      !this->RemoveFunctionPoint(id))
    {
    return 0;
    }

  // Redraw the points

  this->RedrawCanvasElements();

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
int vtkKWParameterValueFunctionEditor::RemovePointAtParameter(float parameter)
{
  int fsize = this->GetFunctionSize();
  float point_param;
  for (int i = 0; i < fsize; i++)
    {
    if (this->GetFunctionPointParameter(i, point_param) &&
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

  // Redraw the point

  this->RedrawCanvasPoint(id);

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
int vtkKWParameterValueFunctionEditor::AddPointAtParameter(
  float parameter, int &id)
{
  if (!this->AddFunctionPointAtParameter(parameter, id))
    {
    return 0;
    }

  // Redraw the point

  this->RedrawCanvasPoint(id);

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

  float parameter, editor_parameter;
  int new_id;

  // Browse all editor's point, get their parameters, add them to our own
  // function (the values will be interpolated automatically)

  for (int id = 0; id < editor_size; id++)
    {
    if (editor->GetFunctionPointParameter(id, editor_parameter) &&
        (!this->GetFunctionPointParameter(id, parameter) ||
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
void vtkKWParameterValueFunctionEditor::UpdateRangeLabelWithRange()
{
  if (!this->IsCreated() || !this->RangeLabel || !this->RangeLabel->IsAlive())
    {
    return;
    }

  ostrstream ranges;
  int nb_ranges = 0;

  float *param = GetVisibleParameterRange();
  if (param && !this->HideParameterRange)
    {
    char format[1024], range[1024];
    sprintf(format, "[%%.%df, %%.%df]",
            this->ParameterRange->GetEntriesResolution(),
            this->ParameterRange->GetEntriesResolution());
    sprintf(range, format, param[0], param[1]);
    ranges << range;
    nb_ranges++;
    }

  float *value = GetVisibleValueRange();
  if (value && !this->HideValueRange)
    {
    char format[1024], range[1024];
    sprintf(format, "[%%.%df, %%.%df]",
            this->ValueRange->GetEntriesResolution(),
            this->ValueRange->GetEntriesResolution());
    sprintf(range, format, value[0], value[1]);
    if (nb_ranges)
      {
      ranges << " x ";
      }
    ranges << range;
    }

  ranges << ends;
  this->RangeLabel->SetLabel(ranges.str());
  ranges.rdbuf()->freeze(0);
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
  
  float parameter;
  int *point_id = reinterpret_cast<int *>(calldata);
  float *fargs = reinterpret_cast<float *>(calldata);

  switch (event)
    {
    // Synchronize visible range
    
    case vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent:
    case vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent:
      self->SetVisibleParameterRange(pvfe->GetVisibleParameterRange());
      break;
      
    // Synchronize points
      
    case vtkKWParameterValueFunctionEditor::PointMovingEvent:
      if (pvfe->GetFunctionPointParameter(*point_id, parameter))
        {
        self->MoveFunctionPointToParameter(*point_id, parameter);
        }
      break;

    case vtkKWParameterValueFunctionEditor::PointRemovedEvent:
      self->RemovePointAtParameter(fargs[1]);
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
  this->RedrawCanvas();
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
  this->UpdateRangeLabelWithRange();
  this->RedrawCanvas();

  this->InvokeVisibleRangeChangingCommand();

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangingEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedCallback()
{
  this->UpdateRangeLabelWithRange();
  this->RedrawCanvas();

  this->InvokeVisibleRangeChangedCommand();

  this->InvokeEvent(
    vtkKWParameterValueFunctionEditor::VisibleParameterRangeChangedEvent);
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleValueRangeChangingCallback()
{
  this->UpdateRangeLabelWithRange();
  this->RedrawCanvas();

  this->InvokeVisibleRangeChangingCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::VisibleValueRangeChangedCallback()
{
  this->UpdateRangeLabelWithRange();
  this->RedrawCanvas();

  this->InvokeVisibleRangeChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::StartInteractionCallback(int x, int y)
{
  if (!this->IsCreated() || !this->HasFunction() || !this->GetFunctionSize())
    {
    return;
    }

  const char *canv = this->Canvas->GetWidgetName();

  // If we are out of the canvas, clamp the coordinates

  if (x < VTK_KW_RANGE_CANVAS_BORDER)
    {
    x = VTK_KW_RANGE_CANVAS_BORDER;
    }
  else if (x > VTK_KW_RANGE_CANVAS_BORDER + this->CanvasWidth - 1)
    {
    x = VTK_KW_RANGE_CANVAS_BORDER + this->CanvasWidth - 1;
    }

  if (y < VTK_KW_RANGE_CANVAS_BORDER)
    {
    y = VTK_KW_RANGE_CANVAS_BORDER;
    }
  else if (y > VTK_KW_RANGE_CANVAS_BORDER + this->CanvasHeight - 1)
    {
    y = VTK_KW_RANGE_CANVAS_BORDER + this->CanvasHeight - 1;
    }

  // Get the real canvas coordinates

  int c_x = atoi(this->Script("%s canvasx %d", canv, x));
  int c_y = atoi(this->Script("%s canvasy %d", canv, y));

  // Browse the canvas item representing the points and find which 
  // one match the pick

  float c_x1, c_y1, c_x2, c_y2;
  int nb_points = this->GetFunctionSize();

  int id;
  for (id = 0; id < nb_points; id++)
    {
    if (sscanf(this->Script("%s coords p%d", canv, id), 
               "%f %f %f %f", 
               &c_x1, &c_y1, &c_x2, &c_y2) == 4 &&
        c_x >= c_x1 && c_x <= c_x2 && c_y >= c_y1 && c_y <= c_y2)
      {
      break;
      }
    }

  // No point found, then let's add that point

  if (id >= nb_points)
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
     (x < -VTK_KW_RANGE_CANVAS_DELETE_MARGIN ||
      x > this->CanvasWidth - 1 + VTK_KW_RANGE_CANVAS_DELETE_MARGIN ||
      y < -VTK_KW_RANGE_CANVAS_DELETE_MARGIN ||
      y > this->CanvasHeight - 1 + VTK_KW_RANGE_CANVAS_DELETE_MARGIN));

  // If we are out of the canvas, clamp the coordinates

  if (x < VTK_KW_RANGE_CANVAS_BORDER)
    {
    x = VTK_KW_RANGE_CANVAS_BORDER;
    }
  else if (x > VTK_KW_RANGE_CANVAS_BORDER + this->CanvasWidth - 1)
    {
    x = VTK_KW_RANGE_CANVAS_BORDER + this->CanvasWidth - 1;
    }

  if (y < VTK_KW_RANGE_CANVAS_BORDER)
    {
    y = VTK_KW_RANGE_CANVAS_BORDER;
    }
  else if (y > VTK_KW_RANGE_CANVAS_BORDER + this->CanvasHeight - 1)
    {
    y = VTK_KW_RANGE_CANVAS_BORDER + this->CanvasHeight - 1;
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
    if (this->LastConstrainedMove == CONSTRAINED_MOVE_FREE)
      {
      if (fabs((double)(c_x - LastSelectCanvasCoordinates[0])) >
          fabs((double)(c_y - LastSelectCanvasCoordinates[1])))
        {
        this->LastConstrainedMove = CONSTRAINED_MOVE_H;
        }
      else
        {
        this->LastConstrainedMove = CONSTRAINED_MOVE_V;
        }
      }
    if (this->LastConstrainedMove == CONSTRAINED_MOVE_H)
      {
      move_h_only = 1;
      c_y = LastSelectCanvasCoordinates[1];
      }
    else if (this->LastConstrainedMove == CONSTRAINED_MOVE_V)
      {
      move_v_only = 1;
      c_x = LastSelectCanvasCoordinates[0];
      }
    }
  else
    {
    this->LastConstrainedMove = CONSTRAINED_MOVE_FREE;
    }

  // Update the icon to show which interaction is going on

  if (warn_delete)
    {
    this->PointLabel->GetLabel()->SetImageDataName(
      this->Icons[ICON_TRASHCAN]->GetImageDataName());
    }
  else
    {
    if (move_h_only && move_v_only)
      {
      this->PointLabel->GetLabel()->SetImageDataName("");
      }
    else if (move_h_only)
      {
      this->PointLabel->GetLabel()->SetImageDataName(
        this->Icons[ICON_MOVE_H]->GetImageDataName());
      }
    else if (move_v_only)
      {
      this->PointLabel->GetLabel()->SetImageDataName(
        this->Icons[ICON_MOVE_V]->GetImageDataName());
      }
    else
      {
      this->PointLabel->GetLabel()->SetImageDataName(
        this->Icons[ICON_MOVE]->GetImageDataName());
      }
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
      (x < -VTK_KW_RANGE_CANVAS_DELETE_MARGIN ||
       x > this->CanvasWidth - 1 + VTK_KW_RANGE_CANVAS_DELETE_MARGIN ||
       y < -VTK_KW_RANGE_CANVAS_DELETE_MARGIN ||
       y > this->CanvasHeight - 1 + VTK_KW_RANGE_CANVAS_DELETE_MARGIN))
    {
    this->RemovePoint(this->SelectedPoint);
    }
  else
    {
    this->InvokePointMovedCommand(this->SelectedPoint);
    this->InvokeFunctionChangedCommand();
    }

  // Remove any interaction icon

  this->PointLabel->GetLabel()->SetImageDataName("");
}

//----------------------------------------------------------------------------
void vtkKWParameterValueFunctionEditor::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "HideParameterRange: "
     << (this->HideParameterRange ? "On" : "Off") << endl;
  os << indent << "HideValueRange: "
     << (this->HideValueRange ? "On" : "Off") << endl;
  os << indent << "CanvasHeight: "<< this->CanvasHeight << endl;
  os << indent << "CanvasWidth: "<< this->CanvasWidth << endl;
  os << indent << "PointRadius: "<< this->PointRadius << endl;
  os << indent << "SelectedPointRadius: " 
     << this->SelectedPointRadius << endl;
  os << indent << "DisableCommands: "
     << (this->DisableCommands ? "On" : "Off") << endl;
  os << indent << "LockEndPointsParameter: "
     << (this->LockEndPointsParameter ? "On" : "Off") << endl;
  os << indent << "DisableAddAndRemove: "
     << (this->DisableAddAndRemove ? "On" : "Off") << endl;
  os << indent << "Canvas: "<< this->Canvas << endl;
  os << indent << "ParameterRange: "<< this->ParameterRange << endl;
  os << indent << "ValueRange: "<< this->ValueRange << endl;
  os << indent << "TitleFrame: "<< this->TitleFrame << endl;
  os << indent << "InfoFrame: "<< this->InfoFrame << endl;
  os << indent << "RangeLabel: "<< this->RangeLabel << endl;
  os << indent << "PointLabel: "<< this->PointLabel << endl;
  os << indent << "SelectedPoint: "<< this->SelectedPoint << endl;
  os << indent << "PointColor: ("
     << this->PointColor[0] << ", " 
     << this->PointColor[1] << ", " 
     << this->PointColor[2] << ")" << endl;
  os << indent << "SelectedPointColor: ("
     << this->SelectedPointColor[0] << ", " 
     << this->SelectedPointColor[1] << ", " 
     << this->SelectedPointColor[2] << ")" << endl;
  os << indent << "ComputePointColorFromValue: "
     << (this->ComputePointColorFromValue ? "On" : "Off") << endl;
}

