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

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataSet.h"
#include "vtkEventForwarderCommand.h"
#include "vtkFieldData.h"
#include "vtkDataArray.h"
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

vtkStandardNewMacro(vtkTransferFunctionViewer);

//----------------------------------------------------------------------------
static void vtkTransferFunctionViewer_UpdateDisplaySize(
  vtkObject *caller,
  unsigned long vtkNotUsed(eventId),
  void *tfv, void*)
{
  vtkTransferFunctionViewer *viewer =
    reinterpret_cast<vtkTransferFunctionViewer*>(tfv);
  vtkRenderWindow *win = reinterpret_cast<vtkRenderWindow*>(caller);
  int *rwSize = win->GetSize();
  int *dispSize = viewer->GetSize();
  if (!dispSize ||
      (rwSize[0] > 0 && rwSize[1] > 0 &&
       (rwSize[0] != dispSize[0] || rwSize[1] != dispSize[1])))
    {
    viewer->SetSize(rwSize);
    }
}

//----------------------------------------------------------------------------
vtkTransferFunctionViewer::vtkTransferFunctionViewer()
{
  this->RenderWindow = NULL;
  this->Renderer = vtkRenderer::New();
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  this->Interactor = NULL;
  this->InteractorStyle = vtkInteractorStyleTransferFunctionEditor::New();
  this->EditorWidget = NULL;
  this->EventForwarder = vtkEventForwarderCommand::New();
  this->Histogram = NULL;  

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
  this->SetHistogram(NULL);
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
    vtkCallbackCommand *cb = vtkCallbackCommand::New();
    cb->SetCallback(vtkTransferFunctionViewer_UpdateDisplaySize);
    cb->SetClientData(this);
    this->RenderWindow->AddObserver(vtkCommand::ModifiedEvent, cb);
    cb->Delete();
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
void vtkTransferFunctionViewer::SetShowColorFunctionInBackground(int visibility)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set transfer function editor type before setting color function visibility.");
    return;
    }

  static_cast<vtkTransferFunctionEditorRepresentation*>(
    this->EditorWidget->GetRepresentation())->
    SetShowColorFunctionInBackground(visibility);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetShowColorFunctionInHistogram(int visibility)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set transfer function editor type before setting color function visibility.");
    return;
    }

  static_cast<vtkTransferFunctionEditorRepresentation*>(
    this->EditorWidget->GetRepresentation())->
    SetShowColorFunctionInHistogram(visibility);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetShowColorFunctionOnLines(int visibility)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set transfer function editor type before setting color function visibility.");
    return;
    }

  static_cast<vtkTransferFunctionEditorRepresentation*>(
    this->EditorWidget->GetRepresentation())->
    SetColorLinesByScalar(visibility);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetLinesColor(double r, double g, double b)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set the transfer function editor type before setting the lines color.");
    return;
    }

  static_cast<vtkTransferFunctionEditorRepresentation*>(
    this->EditorWidget->GetRepresentation())->
    SetLinesColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetColorElementsByColorFunction(int color)
{
  if (this->EditorWidget)
    {
    static_cast<vtkTransferFunctionEditorRepresentation*>(
      this->EditorWidget->GetRepresentation())->
      SetColorElementsByColorFunction(color);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetElementsColor(double r, double g, double b)
{
  if (this->EditorWidget)
    {
    static_cast<vtkTransferFunctionEditorRepresentation*>(
      this->EditorWidget->GetRepresentation())->SetElementsColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetElementLighting(
  double ambient, double diffuse, double specular, double specularPower)
{
  if (!this->EditorWidget)
    {
    vtkErrorMacro("Set the transfer function editor type before setting the element lighting parameters.");
    return;
    }

  static_cast<vtkTransferFunctionEditorRepresentation*>(
    this->EditorWidget->GetRepresentation())->SetElementLighting(
      ambient, diffuse, specular, specularPower);
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
    this->EditorWidget->AddObserver(vtkCommand::PickEvent,
                                    this->EventForwarder);
    this->EditorWidget->AddObserver(vtkCommand::PlacePointEvent,
                                    this->EventForwarder);
    this->EditorWidget->AddObserver(vtkCommand::EndInteractionEvent,
                                    this->EventForwarder);
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      rep->AddObserver(vtkCommand::WidgetValueChangedEvent,
                       this->EventForwarder);
      rep->AddObserver(vtkCommand::WidgetModifiedEvent,
                       this->EventForwarder);
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
void vtkTransferFunctionViewer::SetHistogramColor(double r, double g, double b)
{
  if (this->EditorWidget)
    {
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      rep->SetHistogramColor(r, g, b);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetSize(int x, int y)
{
  if (this->EditorWidget)
    {
    int size[2];
    size[0] = x;
    size[1] = y;
    if (this->RenderWindow)
      {
      this->RenderWindow->SetSize(size);
      }
    this->EditorWidget->Configure(size);
    this->Render();
    }
}

//----------------------------------------------------------------------------
int* vtkTransferFunctionViewer::GetSize()
{
  if (this->EditorWidget)
    {
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      return rep->GetDisplaySize();
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetBorderWidth(int width)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetBorderWidth(width);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::Render()
{
  if (this->EditorWidget && this->EditorWidget->GetRepresentation())
    {
    if (this->Histogram)
      {
      vtkTransferFunctionEditorRepresentation *rep =
        vtkTransferFunctionEditorRepresentation::SafeDownCast(
          this->EditorWidget->GetRepresentation());
      if (
          (rep && this->Histogram->GetMTime() > rep->GetHistogramMTime()) ||
          !this->EditorWidget->GetHistogram())
        {
        this->EditorWidget->SetHistogram(this->Histogram);
        vtkDataArray *hist = this->Histogram->GetXCoordinates();
        if (hist)
          {
          double range[2];
          hist->GetRange(range);
          this->SetWholeScalarRange(range);
          this->SetVisibleScalarRange(this->GetWholeScalarRange());
          }
        }
      }
    else
      {
      double range[2];
      this->GetVisibleScalarRange(range);
      if (range[0] > range[1])
        {
        this->SetVisibleScalarRange(this->GetWholeScalarRange());
        }
      }
    vtkColorTransferFunction *cFunc = this->EditorWidget->GetColorFunction();
    vtkPiecewiseFunction *oFunc =
      this->EditorWidget->GetOpacityFunction();
    if ((cFunc && cFunc->GetMTime() > this->EditorWidget->GetColorMTime()) ||
        (oFunc && oFunc->GetMTime() > this->EditorWidget->GetOpacityMTime()))
      {
      this->EditorWidget->UpdateFromTransferFunctions();
      }
    this->EditorWidget->GetRepresentation()->BuildRepresentation();
    int size[2];
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    rep->GetDisplaySize(size);
    if (size[0] > 0 && size[1] > 0)
      {
      vtkCamera *cam = this->Renderer->GetActiveCamera();
      cam->SetPosition(size[0]*0.5, size[1]*0.5, 1);
      cam->SetFocalPoint(size[0]*0.5, size[1]*0.5, 0);
      cam->SetParallelScale(size[1]*0.5);
      }
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
void vtkTransferFunctionViewer::SetLockEndPoints(int lock)
{
  vtkTransferFunctionEditorWidgetSimple1D *tfe =
    vtkTransferFunctionEditorWidgetSimple1D::SafeDownCast(this->EditorWidget);
  if (tfe)
    {
    tfe->SetLockEndPoints(lock);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetOpacityFunction(
  vtkPiecewiseFunction *function)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetOpacityFunction(function);
    }
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
void vtkTransferFunctionViewer::SetColorFunction(
  vtkColorTransferFunction *function)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetColorFunction(function);
    }
}

//----------------------------------------------------------------------------
vtkColorTransferFunction* vtkTransferFunctionViewer::GetColorFunction()
{
  if (this->EditorWidget)
    {
    return this->EditorWidget->GetColorFunction();
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetAllowInteriorElements(int allow)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetAllowInteriorElements(allow);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetElementOpacity(unsigned int idx,
                                                  double opacity)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetElementOpacity(idx, opacity);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetElementRGBColor(
  unsigned int idx, double r, double g, double b)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetElementRGBColor(idx, r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetElementHSVColor(
  unsigned int idx, double h, double s, double v)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetElementHSVColor(idx, h, s, v);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetElementScalar(
  unsigned int idx, double scalar)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetElementScalar(idx, scalar);
    }
}

//----------------------------------------------------------------------------
double vtkTransferFunctionViewer::GetElementOpacity(unsigned int idx)
{
  if (this->EditorWidget)
    {
    return this->EditorWidget->GetElementOpacity(idx);
    }
  return 0.0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionViewer::GetElementRGBColor(unsigned int idx,
                                                  double color[3])
{
  if (this->EditorWidget)
    {
    return this->EditorWidget->GetElementRGBColor(idx, color);
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkTransferFunctionViewer::GetElementHSVColor(unsigned int idx,
                                                  double color[3])
{
  if (this->EditorWidget)
    {
    return this->EditorWidget->GetElementHSVColor(idx, color);
    }
  return 0;
}

//----------------------------------------------------------------------------
double vtkTransferFunctionViewer::GetElementScalar(unsigned int idx)
{
  if (this->EditorWidget)
    {
    return this->EditorWidget->GetElementScalar(idx);
    }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetColorSpace(int space)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetColorSpace(space);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetCurrentElementId(unsigned int idx)
{
  if (this->EditorWidget)
    {
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      if (idx < rep->GetNumberOfHandles())
        {
        rep->SetActiveHandle(idx);
        }
      }
    }
}

//----------------------------------------------------------------------------
unsigned int vtkTransferFunctionViewer::GetCurrentElementId()
{
  if (this->EditorWidget)
    {
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      return rep->GetActiveHandle();
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::MoveToPreviousElement()
{
  if (this->EditorWidget)
    {
    this->EditorWidget->MoveToPreviousElement();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::MoveToNextElement()
{
  if (this->EditorWidget)
    {
    this->EditorWidget->MoveToNextElement();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetHistogram(vtkRectilinearGrid *histogram)
{
  if (this->Histogram != histogram)
    {
    if (this->EditorWidget)
      {
      this->EditorWidget->SetHistogram(histogram);
      }
    vtkRectilinearGrid *tmpHist = this->Histogram;
    this->Histogram = histogram;
    if (this->Histogram)
      {
      this->Histogram->Register(this);
      }
    if (tmpHist)
      {
      tmpHist->UnRegister(this);
      }
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "RenderWindow:";
  if (this->RenderWindow)
    {
    os << "\n";
    this->RenderWindow->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << " none\n";
    }
  os << indent << "Renderer:\n";
  this->Renderer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Interactor:";
  if (this->Interactor)
    {
    os << "\n";
    this->Interactor->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << " none\n";
    }
  os << indent << "EditorWidget:";
  if (this->EditorWidget)
    {
    os << "\n";
    this->EditorWidget->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << " none\n";
    }
}
