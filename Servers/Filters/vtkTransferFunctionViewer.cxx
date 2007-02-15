/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTransferFunctionViewer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTransferFunctionViewer.h"

#include "vtkDataSet.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFieldData.h"
#include "vtkInteractorStyleTransferFunctionEditor.h"
#include "vtkObjectFactory.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentation.h"
#include "vtkTransferFunctionEditorWidgetSimple1D.h"
#include "vtkTransferFunctionEditorWidgetShapes1D.h"
#include "vtkTransferFunctionEditorWidgetShapes2D.h"

vtkCxxRevisionMacro(vtkTransferFunctionViewer, "1.6");
vtkStandardNewMacro(vtkTransferFunctionViewer);

//----------------------------------------------------------------------------
vtkTransferFunctionViewer::vtkTransferFunctionViewer()
{
  this->RenderWindow = NULL;
  this->Renderer = vtkRenderer::New();
  this->Interactor = NULL;
  this->InteractorStyle = vtkInteractorStyleTransferFunctionEditor::New();
  this->EditorWidget = NULL;
  this->EventForwarder = vtkEventForwarderCommand::New();

  this->EventForwarder->SetTarget(this);

  vtkRenderWindow *win = vtkRenderWindow::New();
  this->SetRenderWindow(win);
  win->Delete();

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  this->SetInteractor(iren);
  iren->Delete();

  this->InteractorStyle->AddObserver(vtkCommand::InteractionEvent,
                                     this->EventForwarder);

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
vtkTransferFunctionViewer::~vtkTransferFunctionViewer()
{
  this->RenderWindow->Delete();
  this->Renderer->Delete();
  this->Interactor->Delete();
  this->InteractorStyle->Delete();
  if (this->EditorWidget)
    {
    this->EditorWidget->Delete();
    this->EditorWidget = NULL;
    }
  this->EventForwarder->Delete();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetRenderWindow(vtkRenderWindow *win)
{
  if (this->RenderWindow == win)
    {
    return;
    }

  this->UnInstallPipeline();

  if (this->RenderWindow)
    {
    this->RenderWindow->UnRegister(this);
    }

  this->RenderWindow = win;

  if (this->RenderWindow)
    {
    this->RenderWindow->Register(this);
    }

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetInteractor(vtkRenderWindowInteractor *iren)
{
  if (this->Interactor == iren)
    {
    return;
    }

  this->UnInstallPipeline();

  if (this->Interactor)
    {
    this->Interactor->UnRegister(this);
    }

  this->Interactor = iren;

  if (this->Interactor)
    {
    this->Interactor->Register(this);
    }

  this->InstallPipeline();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetHistogramVisibility(int visibility)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set transfer function editor type before setting histogram visibility.");
    return;
    }

  static_cast<vtkTransferFunctionEditorRepresentation*>(
    this->EditorWidget->GetRepresentation())->SetHistogramVisibility(visibility);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetTransferFunctionEditorType(int type)
{
  switch (type)
    {
    case SIMPLE_1D:
      if (this->EditorWidget)
        {
        if (this->EditorWidget->IsA("vtkTransferFunctionEditorWidgetSimple1D"))
          {
          return;
          }
        this->EditorWidget->Delete();
        }
      this->EditorWidget = vtkTransferFunctionEditorWidgetSimple1D::New();
      break;
    case SHAPES_1D:
      if (this->EditorWidget)
        {
        if (this->EditorWidget->IsA("vtkTransferFunctionEditorWidgetShapes1D"))
          {
          return;
          }
        this->EditorWidget->Delete();
        }
      this->EditorWidget = vtkTransferFunctionEditorWidgetShapes1D::New();
      break;
    case SHAPES_2D:
      if (this->EditorWidget)
        {
        if (this->EditorWidget->IsA("vtkTransferFunctionEditorWidgetShapes2D"))
          {
          return;
          }
        this->EditorWidget->Delete();
        }
      this->EditorWidget = vtkTransferFunctionEditorWidgetShapes2D::New();
      break;
    default:
      vtkErrorMacro("Unknown transfer function editor type.");
    }

  if (this->EditorWidget)
    {
    this->EditorWidget->SetInteractor(this->Interactor);
    this->EditorWidget->SetDefaultRenderer(this->Renderer);
    this->EditorWidget->SetEnabled(1);
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      int *size = this->RenderWindow->GetSize();
      if (size[0] == 0 && size[1] == 0)
        {
        size[0] = size[1] = 300;
        }
      rep->SetDisplaySize(size);
      }
    this->InteractorStyle->SetWidget(this->EditorWidget);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetModificationType(int type)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetModificationType(type);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetModificationTypeToColor()
{
  this->SetModificationType(vtkTransferFunctionEditorWidget::COLOR);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetModificationTypeToOpacity()
{
  this->SetModificationType(vtkTransferFunctionEditorWidget::OPACITY);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetModificationTypeToColorAndOpacity()
{
  this->SetModificationType(
    vtkTransferFunctionEditorWidget::COLOR_AND_OPACITY);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetBackgroundColor(double r, double g,
                                                   double b)
{
  this->Renderer->SetBackground(r, g, b);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetSize(int x, int y)
{
  this->RenderWindow->SetSize(x, y);
  if (this->EditorWidget)
    {
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      rep->SetDisplaySize(x, y);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::Render()
{
  if (this->EditorWidget && this->EditorWidget->GetRepresentation())
    {
    this->EditorWidget->GetRepresentation()->BuildRepresentation();
    }

  this->RenderWindow->Render();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::InstallPipeline()
{
  if (this->Interactor)
    {
    this->Interactor->SetInteractorStyle(this->InteractorStyle);
    this->Interactor->SetRenderWindow(this->RenderWindow);
    }

  if (this->RenderWindow)
    {
    this->RenderWindow->AddRenderer(this->Renderer);
    }

  if (this->EditorWidget)
    {
    this->EditorWidget->SetInteractor(this->Interactor);
    this->EditorWidget->SetEnabled(1);
    }

  if (this->RenderWindow != NULL)
    {
    if (this->EditorWidget)
      {
      vtkTransferFunctionEditorRepresentation *rep =
        vtkTransferFunctionEditorRepresentation::SafeDownCast(
          this->EditorWidget->GetRepresentation());
      if (rep)
        {
        int *size = this->RenderWindow->GetSize();
        if (size[0] == 0 && size[1] == 0)
          {
          // to match the defaults for vtkRenderWindow
          size[0] = size[1] = 300;
          }
        rep->SetDisplaySize(size);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::UnInstallPipeline()
{
  if (this->Interactor)
    {
    this->Interactor->SetInteractorStyle(NULL);
    this->Interactor->SetRenderWindow(NULL);
    }

  if (this->RenderWindow)
    {
    this->RenderWindow->RemoveRenderer(this->Renderer);
    }

  if (this->EditorWidget)
    {
    this->EditorWidget->SetInteractor(NULL);
    this->EditorWidget->SetEnabled(0);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetVisibleScalarRange(double min, double max)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetVisibleScalarRange(min, max);
    }

  vtkTransferFunctionEditorRepresentation *rep =
    vtkTransferFunctionEditorRepresentation::SafeDownCast(
      this->EditorWidget->GetRepresentation());
  if (rep)
    {
    rep->BuildRepresentation();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::GetVisibleScalarRange(double range[2])
{
  if (this->EditorWidget)
    {
    this->EditorWidget->GetVisibleScalarRange(range);
    }
}

//----------------------------------------------------------------------------
double* vtkTransferFunctionViewer::GetVisibleScalarRange()
{
  if (!this->EditorWidget)
    {
    return NULL;
    }
  return this->EditorWidget->GetVisibleScalarRange();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetWholeScalarRange(double min, double max)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetWholeScalarRange(min, max);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::GetWholeScalarRange(double range[2])
{
  if (this->EditorWidget)
    {
    this->EditorWidget->GetWholeScalarRange(range);
    }
}

//----------------------------------------------------------------------------
double* vtkTransferFunctionViewer::GetWholeScalarRange()
{
  if (!this->EditorWidget)
    {
    return NULL;
    }
  return this->EditorWidget->GetWholeScalarRange();
}

//----------------------------------------------------------------------------
vtkPiecewiseFunction* vtkTransferFunctionViewer::GetOpacityFunction()
{
  if (this->EditorWidget)
    {
    return this->EditorWidget->GetOpacityFunction();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetHistogram(vtkRectilinearGrid *histogram)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set the transfer function editor type before setting the histogram.");
    return;
    }

  this->EditorWidget->SetHistogram(histogram);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
