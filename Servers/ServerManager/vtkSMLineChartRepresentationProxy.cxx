/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLineChartRepresentationProxy.h"

#include "vtkObjectFactory.h"

#include "vtkSMLineChartViewProxy.h"
#include "vtkQtChartTableRepresentation.h"
#include "vtkQtLineChartView.h"
#include "vtkDataObject.h"

vtkStandardNewMacro(vtkSMLineChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMLineChartRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMLineChartRepresentationProxy::vtkSMLineChartRepresentationProxy()
{
  this->VTKRepresentation = vtkQtChartTableRepresentation::New();
  this->Visibility = 0;
}

//----------------------------------------------------------------------------
vtkSMLineChartRepresentationProxy::~vtkSMLineChartRepresentationProxy()
{
  this->VTKRepresentation->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMLineChartRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }
  vtkSMLineChartViewProxy* barChartView = vtkSMLineChartViewProxy::SafeDownCast(
    view);
  if (!barChartView)
    {
    return false;
    }

  this->ChartViewProxy = barChartView;
  if (this->Visibility)
    {
    this->ChartViewProxy->GetLineChartView()->AddRepresentation(
      this->VTKRepresentation);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLineChartRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMLineChartViewProxy* barChartView = vtkSMLineChartViewProxy::SafeDownCast(
    view);
  if (!barChartView || barChartView != this->ChartViewProxy)
    {
    return false;
    }

  if (this->Visibility && this->ChartViewProxy)
    {
    this->ChartViewProxy->GetLineChartView()->RemoveRepresentation(
      this->VTKRepresentation);
    }
  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMLineChartRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    if (this->ChartViewProxy)
      {
      if (this->Visibility)
        {
        this->ChartViewProxy->GetLineChartView()->AddRepresentation(
          this->VTKRepresentation);
        }
      else
        {
        this->ChartViewProxy->GetLineChartView()->RemoveRepresentation(
          this->VTKRepresentation);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMLineChartRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  this->VTKRepresentation->SetInputConnection(
    this->GetOutput()->GetProducerPort());
}

//----------------------------------------------------------------------------
void vtkSMLineChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


