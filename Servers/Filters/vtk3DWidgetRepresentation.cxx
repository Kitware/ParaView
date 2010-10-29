/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtk3DWidgetRepresentation.h"

#include "vtkAbstractWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkWidgetRepresentation.h"

vtkStandardNewMacro(vtk3DWidgetRepresentation);
vtkCxxSetObjectMacro(vtk3DWidgetRepresentation, Widget, vtkAbstractWidget);
vtkCxxSetObjectMacro(vtk3DWidgetRepresentation, Representation,
  vtkWidgetRepresentation);
//----------------------------------------------------------------------------
vtk3DWidgetRepresentation::vtk3DWidgetRepresentation()
{
  this->SetNumberOfInputPorts(0);
  this->Widget = 0;
  this->Representation = 0;
  this->UseNonCompositedRenderer = false;
  this->Enabled = false;
}

//----------------------------------------------------------------------------
vtk3DWidgetRepresentation::~vtk3DWidgetRepresentation()
{
  this->SetWidget(0);
  this->SetRepresentation(0);
}

//----------------------------------------------------------------------------
bool vtk3DWidgetRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    if (this->Widget)
      {
      this->Widget->SetInteractor(pvview->GetInteractor());
      this->Widget->SetCurrentRenderer(
        this->UseNonCompositedRenderer?
        pvview->GetNonCompositedRenderer() :
        pvview->GetRenderer());
      }
    if (this->Representation)
      {
      if (this->UseNonCompositedRenderer)
        {
        this->Representation->SetRenderer(pvview->GetNonCompositedRenderer());
        pvview->GetNonCompositedRenderer()->AddActor(this->Representation);
        }
      else
        {
        this->Representation->SetRenderer(pvview->GetRenderer());
        pvview->GetRenderer()->AddActor(this->Representation);
        }
      }
    this->View = pvview;
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
  if (this->View && this->Widget)
    {
    // Ensure that correct current renderer otherwise the widget may locate the
    // wrong renderer.
    if (this->Enabled)
      {
      if (this->UseNonCompositedRenderer)
        {
        this->Widget->SetCurrentRenderer(this->View->GetNonCompositedRenderer());
        }
      else
        {
        this->Widget->SetCurrentRenderer(this->View->GetRenderer());
        }
      }

    this->Widget->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
bool vtk3DWidgetRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(view);
  if (pvview)
    {
    if (this->Widget)
      {
      this->Widget->SetEnabled(0);
      this->Widget->SetCurrentRenderer(0);
      this->Widget->SetInteractor(0);
      }
    if (this->Representation)
      {
      if (this->UseNonCompositedRenderer)
        {
        pvview->GetNonCompositedRenderer()->RemoveActor(this->Representation);
        }
      else
        {
        pvview->GetRenderer()->RemoveActor(this->Representation);
        }
      this->Representation->SetRenderer(0);
      }
    return true;
    }
  return false;
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
}
