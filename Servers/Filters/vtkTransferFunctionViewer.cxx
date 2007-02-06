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
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTransferFunctionEditorRepresentation.h"
#include "vtkTransferFunctionEditorWidgetSimple1D.h"
#include "vtkTransferFunctionEditorWidgetShapes1D.h"
#include "vtkTransferFunctionEditorWidgetShapes2D.h"

vtkCxxRevisionMacro(vtkTransferFunctionViewer, "1.1");
vtkStandardNewMacro(vtkTransferFunctionViewer);

//----------------------------------------------------------------------------
vtkTransferFunctionViewer::vtkTransferFunctionViewer()
{
  this->RenderWindow = NULL;
  this->Renderer = vtkRenderer::New();
  this->Interactor = NULL;
  this->InteractorStyle = vtkInteractorStyleTransferFunctionEditor::New();
  this->EditorWidget = NULL;
  this->Input = NULL;
  this->ArrayName = NULL;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
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
  this->SetInput(NULL);
  this->SetArrayName(NULL);
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
void vtkTransferFunctionViewer::SetInput(vtkDataSet *input)
{
  if (this->Input != input)
    {
    vtkDataSet *tmpInput = this->Input;
    this->Input = input;
    if (this->Input != NULL)
      {
      this->Input->Register(this);
      }
    if (tmpInput != NULL)
      {
      tmpInput->UnRegister(this);
      }
    this->Modified();
    }

  if (this->EditorWidget)
    {
    this->EditorWidget->SetInput(input);
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetArrayName(const char* name)
{
  if (this->ArrayName == NULL && name == NULL)
    {
    return;
    }
  if (this->ArrayName && name && !strcmp(this->ArrayName, name))
    {
    return;
    }
  if (this->ArrayName)
    {
    delete [] this->ArrayName;
    }
  if (name)
    {
    size_t len = strlen(name) + 1;
    char *str1 = new char[len];
    const char *str2 = name;
    this->ArrayName = str1;
    do
      {
      *str1++ = *str2++;
      }
    while (--len);
    }
  else
    {
    this->ArrayName = NULL;
    }
  if (this->EditorWidget)
    {
    this->EditorWidget->SetArrayName(this->ArrayName);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetFieldAssociation(int assoc)
{
  int clampedVal =
    (assoc < vtkDataObject::FIELD_ASSOCIATION_POINTS) ?
    vtkDataObject::FIELD_ASSOCIATION_POINTS : assoc;
  clampedVal =
    (assoc > vtkDataObject::FIELD_ASSOCIATION_CELLS) ?
    vtkDataObject::FIELD_ASSOCIATION_CELLS : assoc;
  
  if (this->FieldAssociation != clampedVal)
    {
    this->FieldAssociation = clampedVal;
    this->Modified();
    if (this->EditorWidget)
      {
      this->EditorWidget->SetFieldAssociation(clampedVal);
      }
    }
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetFieldAssociationToPoints()
{
  this->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::SetFieldAssociationToCells()
{
  this->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
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
    this->EditorWidget->SetInput(this->Input);
    this->EditorWidget->SetFieldAssociation(this->FieldAssociation);
    this->EditorWidget->SetArrayName(this->ArrayName);
    this->EditorWidget->SetEnabled(1);
    vtkTransferFunctionEditorRepresentation *rep =
      vtkTransferFunctionEditorRepresentation::SafeDownCast(
        this->EditorWidget->GetRepresentation());
    if (rep)
      {
      int *size = this->RenderWindow->GetSize();
      if (size[0] == 0 && size[1] == 0)
        {
        size[0] = size[0] = 300;
        }
      rep->SetDisplaySize(size);
      }
    this->InteractorStyle->SetWidget(this->EditorWidget);
    }
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
void vtkTransferFunctionViewer::SetScalarRange(double min, double max)
{
  if (this->EditorWidget)
    {
    this->EditorWidget->SetScalarRange(min, max);
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
void vtkTransferFunctionViewer::GetScalarRange(double range[2])
{
  if (this->EditorWidget)
    {
    this->EditorWidget->GetScalarRange(range);
    }
}

//----------------------------------------------------------------------------
double* vtkTransferFunctionViewer::GetScalarRange()
{
  if (!this->EditorWidget)
    {
    return NULL;
    }
  return this->EditorWidget->GetScalarRange();
}

//----------------------------------------------------------------------------
void vtkTransferFunctionViewer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
