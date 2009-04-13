/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartTableRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartTableRepresentationProxy.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkQtChartNamedSeriesOptionsModel.h"
#include "vtkQtChartRepresentation.h"
#include "vtkQtChartView.h"
#include "vtkSMChartNamedOptionsModelProxy.h"
#include "vtkSMChartViewProxy.h"

vtkStandardNewMacro(vtkSMChartTableRepresentationProxy);
vtkCxxRevisionMacro(vtkSMChartTableRepresentationProxy, "1.2");
//----------------------------------------------------------------------------
vtkSMChartTableRepresentationProxy::vtkSMChartTableRepresentationProxy()
{
  this->VTKRepresentation = vtkQtChartRepresentation::New();
  this->Visibility = 0;

  this->UseIndexForXAxis = true;
  this->XSeriesName = 0;
}

//----------------------------------------------------------------------------
vtkSMChartTableRepresentationProxy::~vtkSMChartTableRepresentationProxy()
{
  this->SetXSeriesName(0);
  this->VTKRepresentation->Delete();
}

//----------------------------------------------------------------------------
void vtkSMChartTableRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMChartTableRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  this->OptionsProxy = vtkSMChartNamedOptionsModelProxy::SafeDownCast(
    this->GetSubProxy("SeriesOptions"));
  this->OptionsProxy->CreateObjects(this->VTKRepresentation);
  this->VTKRepresentation->SetOptionsModel(
    this->OptionsProxy->GetOptionsModel());
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMChartTableRepresentationProxy::AddToView(vtkSMViewProxy* view)
{
  if (!this->Superclass::AddToView(view))
    {
    return false;
    }
  vtkSMChartViewProxy* chartView = vtkSMChartViewProxy::SafeDownCast(
    view);
  if (!chartView)
    {
    return false;
    }

  this->ChartViewProxy = chartView;
  if (this->Visibility)
    {
    this->ChartViewProxy->GetChartView()->AddRepresentation(
      this->VTKRepresentation);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMChartTableRepresentationProxy::RemoveFromView(vtkSMViewProxy* view)
{
  vtkSMChartViewProxy* barChartView = vtkSMChartViewProxy::SafeDownCast(
    view);
  if (!barChartView || barChartView != this->ChartViewProxy)
    {
    return false;
    }

  if (this->Visibility && this->ChartViewProxy)
    {
    this->ChartViewProxy->GetChartView()->RemoveRepresentation(
      this->VTKRepresentation);
    }
  this->ChartViewProxy = 0;
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkSMChartTableRepresentationProxy::SetVisibility(int visible)
{
  if (this->Visibility != visible)
    {
    this->Visibility = visible;
    if (this->ChartViewProxy)
      {
      if (this->Visibility)
        {
        this->ChartViewProxy->GetChartView()->AddRepresentation(
          this->VTKRepresentation);
        }
      else
        {
        this->ChartViewProxy->GetChartView()->RemoveRepresentation(
          this->VTKRepresentation);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMChartTableRepresentationProxy::Update(vtkSMViewProxy* view)
{
  this->Superclass::Update(view);
  this->VTKRepresentation->SetInputConnection(
    this->GetOutput()->GetProducerPort());
  this->VTKRepresentation->Update();
  this->UpdatePropertyInformation();
}

//----------------------------------------------------------------------------
int vtkSMChartTableRepresentationProxy::GetNumberOfSeries()
{
  return this->VTKRepresentation->GetNumberOfSeries();
}

//----------------------------------------------------------------------------
const char* vtkSMChartTableRepresentationProxy::GetSeriesName(int series)
{
  return this->VTKRepresentation->GetSeriesName(series);
}

//----------------------------------------------------------------------------
void vtkSMChartTableRepresentationProxy::SetUseIndexForXAxis(bool use_index)
{
  this->UseIndexForXAxis = use_index;
  this->UpdateXSeriesName();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMChartTableRepresentationProxy::SetXAxisSeriesName(const char* name)
{
  this->SetXSeriesName(name);
  this->UpdateXSeriesName();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMChartTableRepresentationProxy::UpdateXSeriesName()
{
  if (!this->UseIndexForXAxis && this->XSeriesName && this->XSeriesName[0])
    {
    this->VTKRepresentation->SetKeyColumn(this->XSeriesName);
    }
  else
    {
    this->VTKRepresentation->SetKeyColumn(0);
    }
}

