/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBarChartRepresentationProxy.h"

#include "vtkObjectFactory.h"

#include "vtkSMBarChartViewProxy.h"
#include "vtkQtChartTableRepresentation.h"
#include "vtkQtBarChartView.h"
#include "vtkDataObject.h"

vtkStandardNewMacro(vtkSMBarChartRepresentationProxy);
vtkCxxRevisionMacro(vtkSMBarChartRepresentationProxy, "1.1");
//----------------------------------------------------------------------------
vtkSMBarChartRepresentationProxy::vtkSMBarChartRepresentationProxy()
{
  this->VTKRepresentation = vtkQtChartTableRepresentation::New();
  this->Visibility = 0;
}

//----------------------------------------------------------------------------
vtkSMBarChartRepresentationProxy::~vtkSMBarChartRepresentationProxy()
{
  this->VTKRepresentation->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMBarChartRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }
  vtkSMBarChartViewProxy* barChartView = vtkSMBarChartViewProxy::SafeDownCast(
    view);
  if (!barChartView)
    {
    return false;
    }

  this->ChartViewProxy = barChartView;
  if (this->Visibility)
    {
    this->ChartViewProxy->GetBarChartView()->AddRepresentation(
      this->VTKRepresentation);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMBarChartRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMBarChartViewProxy* barChartView = vtkSMBarChartViewProxy::SafeDownCast(
    view);
  if (!barChartView || barChartView != this->ChartViewProxy)
    {
    return false;
    }

  if (this->Visibility && this->ChartViewProxy)
    {
    this->ChartViewProxy->GetBarChartView()->RemoveRepresentation(
      this->VTKRepresentation);
    }
  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMBarChartRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    if (this->ChartViewProxy)
      {
      if (this->Visibility)
        {
        this->ChartViewProxy->GetBarChartView()->AddRepresentation(
          this->VTKRepresentation);
        }
      else
        {
        this->ChartViewProxy->GetBarChartView()->RemoveRepresentation(
          this->VTKRepresentation);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMBarChartRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  this->VTKRepresentation->SetInputConnection(
    this->GetOutput()->GetProducerPort());
}

//----------------------------------------------------------------------------
void vtkSMBarChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


