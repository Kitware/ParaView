/*=========================================================================

  Program:   ParaView
  Module:    vtk3DWidgetRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk3DWidgetRepresentation.h"

#include "vtkAbstractWidget.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVImplicitPlaneRepresentation.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkWidgetRepresentation.h"

vtkStandardNewMacro(vtk3DWidgetRepresentation);
vtkCxxSetObjectMacro(vtk3DWidgetRepresentation, Widget, vtkAbstractWidget);
//----------------------------------------------------------------------------
vtk3DWidgetRepresentation::vtk3DWidgetRepresentation()
{
  this->SetNumberOfInputPorts(0);
  this->Widget = 0;
  this->Representation = 0;
  this->UseNonCompositedRenderer = false;
  this->Enabled = false;

  this->CustomTransform = vtkTransform::New();
  this->CustomTransform->PostMultiply();
  this->CustomTransform->Identity();
  this->RepresentationObserverTag = 0;
}

//----------------------------------------------------------------------------
vtk3DWidgetRepresentation::~vtk3DWidgetRepresentation()
{
  this->SetWidget(0);
  this->SetRepresentation(0);
  this->CustomTransform->Delete();
}

//-----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::SetRepresentation(vtkWidgetRepresentation* repr)
{
  if (this->Representation == repr)
    {
    return;
    }

  if (this->Representation)
    {
    this->Representation->RemoveObserver(this->RepresentationObserverTag);
    this->RepresentationObserverTag = 0;
    }
  vtkSetObjectBodyMacro(Representation, vtkWidgetRepresentation, repr);
  if (this->Representation)
    {
    this->RepresentationObserverTag = this->Representation->AddObserver(
      vtkCommand::ModifiedEvent,
      this, &vtk3DWidgetRepresentation::OnRepresentationModified);
    }

  this->UpdateEnabled();
  this->UpdateTransform();
}

//----------------------------------------------------------------------------
bool vtk3DWidgetRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    vtkRenderer* activeRenderer = this->UseNonCompositedRenderer?
      pvview->GetNonCompositedRenderer() : pvview->GetRenderer();
    if (this->Widget)
      {
      this->Widget->SetInteractor(pvview->GetInteractor());
      this->Widget->SetCurrentRenderer(activeRenderer);
      }
    if (this->Representation)
      {
      this->Representation->SetRenderer(activeRenderer);
      activeRenderer->AddActor(this->Representation);
      }
    this->View = pvview;
    this->UpdateEnabled();
    this->UpdateTransform();
    return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::SetEnabled(bool enable)
{
  if (this->Enabled != enable)
    {
    this->Enabled = enable;
    this->UpdateEnabled();
    }
}

//-----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::UpdateEnabled()
{
  if (this->Widget == NULL)
    {
    return;
    }

  bool enable_widget = (this->View != NULL)? this->Enabled : false;

  // BUG #14913: Don't enable widget when the representation is missing or not
  // visible.
  if (this->Representation == NULL ||
    this->Representation->GetVisibility() == 0)
    {
    enable_widget = false;
    }

  // Not all processes have the interactor setup. Enable 3D widgets only on
  // those processes that have an interactor.
  if (this->View == NULL || this->View->GetInteractor() == NULL)
    {
    enable_widget = false;
    }

  if (this->Widget->GetEnabled() != (enable_widget? 1 : 0))
    {
    this->Widget->SetEnabled(enable_widget? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::OnRepresentationModified()
{
  // This will be called everytime the representation is modified, but since the
  // work done in this->UpdateEnabled() is minimal, we let it do it.
  this->UpdateEnabled();
}

//----------------------------------------------------------------------------
bool vtk3DWidgetRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    this->View = NULL;
    if (this->Widget)
      {
      this->Widget->SetEnabled(0);
      this->Widget->SetCurrentRenderer(0);
      this->Widget->SetInteractor(0);
      }
    if (this->Representation)
      {
      vtkRenderer* renderer = this->Representation->GetRenderer();
      if (renderer)
        {
        renderer->RemoveActor(this->Representation);
        // NOTE: this will modify the Representation and call
        // this->OnRepresentationModified().
        this->Representation->SetRenderer(0);
        }
      }
    this->UpdateTransform();
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::SetCustomWidgetTransform(vtkTransform *transform)
{
  if (this->CustomTransform->GetInput () != transform)
    {
    this->CustomTransform->SetInput(transform);
    if (!transform)
      {
      this->CustomTransform->Identity();
      }
    this->UpdateTransform();
    }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::UpdateTransform()
{
  if (vtkPVImplicitPlaneRepresentation *plane =
    vtkPVImplicitPlaneRepresentation::SafeDownCast(this->Representation))
    {
    if (this->View)
      {
      plane->SetTransform(this->CustomTransform);
      plane->UpdateTransformLocation();
      }
    else
      {
      plane->ClearTransform();
      }
    }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseNonCompositedRenderer: " << this->UseNonCompositedRenderer
    << endl;
  os << indent << "Widget: " << this->Widget << endl;
  os << indent << "Representation: " << this->Representation << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "CustomTransform: ";
  this->CustomTransform->Print(os);
}
