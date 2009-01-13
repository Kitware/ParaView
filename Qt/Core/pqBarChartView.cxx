/*=========================================================================

   Program: ParaView
   Module:    pqBarChartView.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqBarChartView.h"

// Server Manager Includes.
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMChartRepresentationProxy.h"
#include "vtkPVDataInformation.h"

#include "vtkTable.h"
#include "vtkSmartPointer.h"
#include "vtkQtBarChartView.h"
#include "vtkQtChartWidget.h"
#include "vtkQtChartSeriesModelCollection.h"
#include "vtkQtChartTableRepresentation.h"

// Qt Includes.
#include <QPushButton>

// ParaView Includes.
#include "pqDataRepresentation.h"
#include "pqBarChartRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqBarChartView::pqInternal
{
public:
  pqInternal()
    {
    this->BarChartView = vtkSmartPointer<vtkQtBarChartView>::New();
    }

  ~pqInternal()
    {

    }

  vtkSmartPointer<vtkQtBarChartView> BarChartView;
  QMap<pqRepresentation*,
      vtkSmartPointer<vtkQtChartTableRepresentation> > RepresentationMap;

};

//-----------------------------------------------------------------------------
pqBarChartView::pqBarChartView(
 const QString& group, const QString& name, 
    vtkSMViewProxy* viewModule, pqServer* server, 
    QObject* _parent/*=NULL*/):
   pqView(barChartViewType(), group, name, viewModule, server, _parent)
{
  this->Internal = new pqInternal();
  QObject::connect(this, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(onAddRepresentation(pqRepresentation*)));
  QObject::connect(this, SIGNAL(representationRemoved(pqRepresentation*)),
    this, SLOT(onRemoveRepresentation(pqRepresentation*)));
  QObject::connect(
    this, SIGNAL(representationVisibilityChanged(pqRepresentation*, bool)),
    this, SLOT(updateRepresentationVisibility(pqRepresentation*, bool)));

  // Add the current Representations to the chart.
  QList<pqRepresentation*> currentRepresentations = this->getRepresentations();
  foreach(pqRepresentation* rep, currentRepresentations)
    {
    this->onAddRepresentation(rep);
    }
}

//-----------------------------------------------------------------------------
pqBarChartView::~pqBarChartView()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqBarChartView::getWidget()
{
  return this->Internal->BarChartView->GetChartWidget();
}

//-----------------------------------------------------------------------------
void pqBarChartView::onAddRepresentation(pqRepresentation* repr)
{
  if (this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  pqBarChartRepresentation* chartRep =
    qobject_cast<pqBarChartRepresentation*>(repr);

  if(!chartRep)
    {
    return;
    }

  // For some reason the vtkTable is null at this point, this
  // is my attempt to update the proxy so it won't be null,
  // but even after this the table still has no data :(
  vtkSMChartRepresentationProxy::
    SafeDownCast(chartRep->getProxy())->Update(
    vtkSMViewProxy::SafeDownCast(this->getProxy()));

  vtkTable* table = chartRep->getClientSideData();
  if (!table)
    {
    return;
    }

  // Add the representation to the view
  vtkQtChartTableRepresentation* barChartRep =
    vtkQtChartTableRepresentation::SafeDownCast(
    this->Internal->BarChartView->AddRepresentationFromInput(table));

  // Store the created representation in the map
  this->Internal->RepresentationMap.insert(repr, barChartRep);

  // Update the chart view
  this->Internal->BarChartView->Update();
}


//-----------------------------------------------------------------------------
void pqBarChartView::onRemoveRepresentation(pqRepresentation* repr)
{
  if (!this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  vtkQtChartTableRepresentation* barChartRep =
    this->Internal->RepresentationMap.value(repr);

  this->Internal->BarChartView->RemoveRepresentation(barChartRep);
  this->Internal->BarChartView->Update();

  // Remove representation from representation map
  this->Internal->RepresentationMap.remove(repr);
}

//-----------------------------------------------------------------------------
void pqBarChartView::updateRepresentationVisibility(
  pqRepresentation* repr, bool visible)
{
  if (!this->Internal->RepresentationMap.contains(repr))
    {
    return;
    }

  vtkQtChartTableRepresentation* barChartRep =
    this->Internal->RepresentationMap.value(repr);

  if (visible)
    {
    this->Internal->BarChartView->AddRepresentation(barChartRep);
    }
  else
    {
    this->Internal->BarChartView->RemoveRepresentation(barChartRep);
    }

  this->Internal->BarChartView->Update();
}

//-----------------------------------------------------------------------------
bool pqBarChartView::canDisplay(pqOutputPort* opPort) const
{
  pqPipelineSource* source = opPort? opPort->getSource() :0;
  vtkSMSourceProxy* sourceProxy = source ? 
    vtkSMSourceProxy::SafeDownCast(source->getProxy()) : 0;
  if(!opPort || !source ||
     opPort->getServer()->GetConnectionID() !=
     this->getServer()->GetConnectionID() || !sourceProxy ||
     sourceProxy->GetOutputPortsCreated()==0)
    {
    return false;
    }

  vtkPVDataInformation* dataInfo = opPort->getDataInformation(true);
  return (dataInfo && dataInfo->DataSetTypeIsA("vtkDataObject"));
}
