/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqObjectHistogramWidget.h"
#include "pqServer.h"
#include "pqVTKEventAdaptor.h"

#include <pqChartValue.h>
#include <pqHistogramChart.h>
#include <pqHistogramWidget.h>

#include <QVBoxLayout>

#include <vtkCommand.h>
#include <vtkDataSet.h>
#include <vtkSMClientSideDataProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSphereSource.h>
#include <vtkPointSet.h>

#include <vtkstd/map>

//////////////////////////////////////////////////////////////////////////////
// pqObjectHistogramWidget::pqImplementation

struct pqObjectHistogramWidget::pqImplementation :
  public pqHistogramWidget
{
  pqImplementation() :
    ClientSideData(0)
  {
  }
  
  ~pqImplementation()
  {
    if(this->ClientSideData)
      this->ClientSideData->Delete();
  }
  
  void setServer(pqServer* Server)
  {
    if(this->ClientSideData)
      this->ClientSideData->Delete();
    
    if(Server)
      this->ClientSideData = vtkSMClientSideDataProxy::SafeDownCast(Server->GetProxyManager()->NewProxy("displays", "ClientSideData"));
  }
  
  vtkSMClientSideDataProxy* ClientSideData;
  pqVTKEventAdaptor EventAdaptor;
};

pqObjectHistogramWidget::pqObjectHistogramWidget(QWidget *parent) :
  QWidget(parent),
  Implementation(new pqImplementation())
{
  QVBoxLayout* const layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->addWidget(this->Implementation);
  
  connect(&this->Implementation->EventAdaptor, SIGNAL(vtkEvent()), this, SLOT(onDisplayData()));
}

pqObjectHistogramWidget::~pqObjectHistogramWidget()
{
  delete this->Implementation;
}

void pqObjectHistogramWidget::onServerChanged(pqServer* server)
{
  this->Implementation->setServer(server);
}

void pqObjectHistogramWidget::onSetProxy(vtkSMSourceProxy* proxy)
{
  if(!proxy)
    return;

  if(!this->Implementation->ClientSideData)
    return;
    
  proxy->AddObserver(vtkCommand::UpdateEvent, &this->Implementation->EventAdaptor);
    
  vtkSMInputProperty* const input = vtkSMInputProperty::SafeDownCast(
    this->Implementation->ClientSideData->GetProperty("Input"));
  input->AddProxy(proxy);
  
  this->onDisplayData();
}

void pqObjectHistogramWidget::onDisplayData()
{
  if(!this->Implementation->ClientSideData)
    return;
    
  this->Implementation->ClientSideData->UpdateVTKObjects();
  this->Implementation->ClientSideData->Update();

  vtkDataSet* const data = this->Implementation->ClientSideData->GetCollectedData();
  if(!data)
    return;

  vtkPointSet* const point_set = vtkPointSet::SafeDownCast(data);
  if(!point_set)
    return;

  // For grins, create a histogram of Z-coordinates
  vtkstd::map<int, int> bins;
  for(vtkIdType i = 0; i != point_set->GetNumberOfPoints(); ++i)
    {
    double point[3];
    point_set->GetPoint(i, point);
    const int bin = point[2];
    bins.insert(vtkstd::make_pair(bin, 0));
    bins[bin]++;
    }

  pqChartValueList list;
  for(vtkstd::map<int, int>::iterator bin = bins.begin(); bin != bins.end(); ++bin)
    {
    list.pushBack(static_cast<float>(bin->second));
    }

  if(bins.size() > 1)
    {
    this->Implementation->getHistogram()->setData(
      list,
      pqChartValue(static_cast<double>(bins.begin()->first)), static_cast<double>(pqChartValue(bins.rbegin()->first)));
    }
}
