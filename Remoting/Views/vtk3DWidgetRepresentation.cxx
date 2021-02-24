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
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkWidgetRepresentation.h"

#include <algorithm>

vtkStandardNewMacro(vtk3DWidgetRepresentation);
vtkCxxSetObjectMacro(vtk3DWidgetRepresentation, Widget, vtkAbstractWidget);
//----------------------------------------------------------------------------
vtk3DWidgetRepresentation::vtk3DWidgetRepresentation()
  : ReferenceBounds{ 0, 0, 0, 0, 0, 0 }
  , UseReferenceBounds(false)
  , PlaceWidgetBounds{ 0, 0, 0, 0, 0, 0 }
{
  this->SetNumberOfInputPorts(0);
  this->Widget = nullptr;
  this->Representation = nullptr;
  this->UseNonCompositedRenderer = false;
  this->Enabled = false;

  this->RepresentationObserverTag = 0;
  this->ViewObserverTag = 0;
}

//----------------------------------------------------------------------------
vtk3DWidgetRepresentation::~vtk3DWidgetRepresentation()
{
  this->SetWidget(nullptr);
  this->SetRepresentation(nullptr);
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
      vtkCommand::ModifiedEvent, this, &vtk3DWidgetRepresentation::OnRepresentationModified);
  }

  this->UpdateEnabled();
}

//----------------------------------------------------------------------------
bool vtk3DWidgetRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
  {
    vtkRenderer* activeRenderer =
      this->UseNonCompositedRenderer ? pvview->GetNonCompositedRenderer() : pvview->GetRenderer();
    if (this->Widget)
    {
      // If DefaultRenderer is non-nullptr, SetCurrentRenderer() will have no
      // effect.
      this->Widget->SetDefaultRenderer(nullptr);
      this->Widget->SetCurrentRenderer(activeRenderer);
      // Set the DefaultRenderer to ensure that it doesn't get overridden by the
      // Widget. The Widget should use the specified renderer. Period.
      this->Widget->SetDefaultRenderer(activeRenderer);
    }
    if (this->Representation)
    {
      this->Representation->SetRenderer(activeRenderer);
      activeRenderer->AddActor(this->Representation);
    }
    if (this->View)
    {
      this->View->RemoveObserver(this->ViewObserverTag);
      this->ViewObserverTag = 0;
    }
    this->View = pvview;
    // observer the view so we know when it gets a new interactor.
    this->ViewObserverTag = this->View->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtk3DWidgetRepresentation::OnViewModified);
    this->UpdateEnabled();
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
  if (this->Widget == nullptr)
  {
    return;
  }

  bool enable_widget = (this->View != nullptr) ? this->Enabled : false;

  // BUG #14913: Don't enable widget when the representation is missing or not
  // visible.
  if (this->Representation == nullptr || this->Representation->GetVisibility() == 0)
  {
    enable_widget = false;
  }

  // Not all processes have the interactor setup. Enable 3D widgets only on
  // those processes that have an interactor.
  if (this->View == nullptr || this->View->GetInteractor() == nullptr)
  {
    enable_widget = false;
  }

  if (this->Widget->GetEnabled() != (enable_widget ? 1 : 0))
  {
    // We do this here, instead of AddToView() since
    // the View may not have the interactor setup when
    // AddToView() is called (which happens when loading state files).
    this->Widget->SetInteractor(this->View->GetInteractor());
    this->Widget->SetEnabled(enable_widget ? 1 : 0);
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
void vtk3DWidgetRepresentation::OnViewModified()
{
  if (this->View && this->Widget && this->View->GetInteractor() != this->Widget->GetInteractor())
  {
    this->UpdateEnabled();
  }
}

//----------------------------------------------------------------------------
bool vtk3DWidgetRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
  {
    this->View = nullptr;
    if (this->Widget)
    {
      this->Widget->SetEnabled(0);
      this->Widget->SetDefaultRenderer(nullptr);
      this->Widget->SetCurrentRenderer(nullptr);
      this->Widget->SetInteractor(nullptr);
    }
    if (this->Representation)
    {
      vtkRenderer* renderer = this->Representation->GetRenderer();
      if (renderer)
      {
        renderer->RemoveActor(this->Representation);
        // NOTE: this will modify the Representation and call
        // this->OnRepresentationModified().
        this->Representation->SetRenderer(nullptr);
      }
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::SetReferenceBounds(const double bds[6])
{
  if (!std::equal(bds, bds + 6, this->ReferenceBounds))
  {
    std::copy(bds, bds + 6, this->ReferenceBounds);
    this->PlaceWidget();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::SetUseReferenceBounds(bool use_ref_bds)
{
  if (this->UseReferenceBounds != use_ref_bds)
  {
    this->UseReferenceBounds = use_ref_bds;
    this->PlaceWidget();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::PlaceWidget(const double bds[6])
{
  if (!std::equal(bds, bds + 6, this->PlaceWidgetBounds))
  {
    std::copy(bds, bds + 6, this->PlaceWidgetBounds);
    this->PlaceWidget();
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::PlaceWidget()
{
  if (this->UseReferenceBounds && this->Representation)
  {
    this->Representation->PlaceWidget(this->ReferenceBounds);
  }
  else if (this->Representation)
  {
    this->Representation->PlaceWidget(this->PlaceWidgetBounds);
  }
}

//----------------------------------------------------------------------------
void vtk3DWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseNonCompositedRenderer: " << this->UseNonCompositedRenderer << endl;
  os << indent << "Widget: " << this->Widget << endl;
  os << indent << "Representation: " << this->Representation << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
}
