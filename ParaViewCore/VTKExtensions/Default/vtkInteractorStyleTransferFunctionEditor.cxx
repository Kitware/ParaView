/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTransferFunctionEditor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInteractorStyleTransferFunctionEditor.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentation.h"
#include "vtkTransferFunctionEditorWidget.h"

vtkStandardNewMacro(vtkInteractorStyleTransferFunctionEditor);

vtkCxxSetObjectMacro(vtkInteractorStyleTransferFunctionEditor, Widget,
                     vtkTransferFunctionEditorWidget);

//----------------------------------------------------------------------------
vtkInteractorStyleTransferFunctionEditor::vtkInteractorStyleTransferFunctionEditor()
{
  this->MotionFactor = 10.0;
  this->Widget = NULL;
}

//----------------------------------------------------------------------------
vtkInteractorStyleTransferFunctionEditor::~vtkInteractorStyleTransferFunctionEditor()
{
  this->SetWidget(NULL);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnMouseMove()
{
  switch (this->State)
    {
    case VTKIS_PAN:
      this->Pan();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    case VTKIS_ZOOM:
      this->Zoom();
      this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
      break;
    }
  
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnMiddleButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  if (!this->Widget || !this->CurrentRenderer)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartPan();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnMiddleButtonUp()
{
  switch (this->State)
    {
    case VTKIS_PAN:
      this->EndPan();
      break;
    }

  if (this->Interactor)
    {
    this->ReleaseFocus();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnRightButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  if (!this->Widget || !this->CurrentRenderer)
    {
    return;
    }

  this->GrabFocus(this->EventCallbackCommand);
  this->StartZoom();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnRightButtonUp()
{
  switch (this->State)
    {
    case VTKIS_ZOOM:
      this->EndZoom();
      break;
    }

  if (this->Interactor)
    {
    this->ReleaseFocus();
    }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnConfigure()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  this->FindPokedRenderer(x, y);

  if (!this->Widget || !this->CurrentRenderer)
    {
    return;
    }

  int *size = this->CurrentRenderer->GetSize();
  this->Widget->Configure(size);
  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(
      this->Widget->GetRepresentation());
  if (rep)
    {
    rep->BuildRepresentation();
    }

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::OnChar()
{
  if (!this->Widget)
    {
    return;
    }

  if (strlen(this->Interactor->GetKeySym()) == 1)
    {
    switch (this->Interactor->GetKeyCode())
      {
      case 'r':
      case 'R':
        {
        this->Widget->ShowWholeScalarRange();
        vtkTransferFunctionEditorRepresentation *rep =
          vtkTransferFunctionEditorRepresentation::SafeDownCast(
            this->Widget->GetRepresentation());
        if (rep)
          {
          rep->BuildRepresentation();
          }
        this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
        }
        break;

      case 'Q' :
      case 'q' :
      case 'e' :
      case 'E' :
        this->Interactor->ExitCallback();
        break;
      }
    }

  this->Interactor->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::Pan()
{
  if (!this->Widget)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  int *size = this->CurrentRenderer->GetSize();
  int dx = rwi->GetLastEventPosition()[0] - rwi->GetEventPosition()[0];
  double pctX = dx / (double)(size[0]);

  double range[2];
  this->Widget->GetVisibleScalarRange(range);

  double newRange[2];
  double rangeShift = (range[1] - range[0]) * pctX;
  newRange[0] = range[0] + rangeShift;
  newRange[1] = range[1] + rangeShift;
  this->Widget->SetVisibleScalarRange(newRange);

  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(
      this->Widget->GetRepresentation());
  if (rep)
    {
    rep->BuildRepresentation();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::Zoom()
{
  if (!this->Widget)
    {
    return;
    }

  vtkRenderWindowInteractor *rwi = this->Interactor;
  double *center = this->CurrentRenderer->GetCenter();
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  double dyf = this->MotionFactor * (double)(dy) / (double)(center[1]);
  double scaleFactor = pow((double)1.1, dyf);

  double range[2];
  this->Widget->GetVisibleScalarRange(range);

  double newRange[2];
  double newRangeWidth = (range[1] - range[0]) / scaleFactor;
  newRange[0] = (range[0] + range[1] - newRangeWidth) * 0.5;
  newRange[1] = newRange[0] + newRangeWidth;
  this->Widget->SetVisibleScalarRange(newRange);

  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(
      this->Widget->GetRepresentation());
  if (rep)
    {
    rep->BuildRepresentation();
    }

  rwi->Render();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleTransferFunctionEditor::PrintSelf(ostream& os,
                                                         vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
