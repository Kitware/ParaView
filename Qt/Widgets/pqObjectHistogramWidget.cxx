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

#include <pqChartValue.h>
#include <pqHistogramChart.h>
#include <pqHistogramWidget.h>

#include <QComboBox>
#include <QVBoxLayout>

#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkSMClientSideDataProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSphereSource.h>

//////////////////////////////////////////////////////////////////////////////
// pqObjectHistogramWidget::pqImplementation

struct pqObjectHistogramWidget::pqImplementation
{
  pqImplementation() :
    ClientSideData(0),
    EventAdaptor(vtkEventQtSlotConnect::New())
  {
  }
  
  ~pqImplementation()
  {
    this->EventAdaptor->Delete();
    
    if(this->ClientSideData)
      {
      this->ClientSideData->Delete();
      this->ClientSideData = 0;
      }
  }
  
  void setProxy(vtkSMSourceProxy* Proxy)
  {
    if(this->ClientSideData)
      {
      this->ClientSideData->Delete();
      this->ClientSideData = 0;
      }  
    
    this->CurrentVariable = QString();
    
    if(!Proxy)
      return;
    
    // Connect a client side data object to the input source
    this->ClientSideData = vtkSMClientSideDataProxy::SafeDownCast(
      Proxy->GetProxyManager()->NewProxy("displays", "ClientSideData"));
    vtkSMInputProperty* const input = vtkSMInputProperty::SafeDownCast(
      this->ClientSideData->GetProperty("Input"));
    input->AddProxy(Proxy);
    
    this->onInputChanged();
  }
  
  void setCurrentVariable(const QString& CurrentVariable)
  {
    this->CurrentVariable = CurrentVariable;
    this->updateChart();
  }
  
  void onInputChanged()
  {
    if(!this->ClientSideData)
      return;
    this->ClientSideData->UpdateVTKObjects();
    this->ClientSideData->Update();

    vtkDataSet* const data = this->ClientSideData->GetCollectedData();
    if(!data)
      return;

    // Populate the current-variable combo-box with available variables
    // (we do this here because the set of available variables may change as properties
    // are modified in e.g: a source or a reader)
    this->Variables.clear();
    this->Variables.addItem(QString(), QString());
    
    if(vtkPointData* const point_data = data->GetPointData())
      {
      for(int i = 0; i != point_data->GetNumberOfArrays(); ++i)
        {
        vtkDataArray* const array = point_data->GetArray(i);
        if(array->GetNumberOfComponents() != 1)
          continue;
        const char* const array_name = point_data->GetArrayName(i);
        this->Variables.addItem(QString(array_name) + " (point)", QString(array_name));
        }
      }
      
    if(vtkCellData* const cell_data = data->GetCellData())
      {
      for(int i = 0; i != cell_data->GetNumberOfArrays(); ++i)
        {
        vtkDataArray* const array = cell_data->GetArray(i);
        if(array->GetNumberOfComponents() != 1)
          continue;
        const char* const array_name = cell_data->GetArrayName(i);
        this->Variables.addItem(QString(array_name) + " (cell)", QString(array_name));
        }
      }

    this->updateChart();
  }
  
  static inline double lerp(double A, double B, double Amount)
  {
    return ((1 - Amount) * A) + (Amount * B);
  }
  
  void updateChart()
  {
    if(!this->ClientSideData)
      return;

    vtkDataSet* const data = this->ClientSideData->GetCollectedData();
    if(!data)
      return;

    vtkDataArray* array = 0;
    if(vtkPointData* const point_data = data->GetPointData())
      {
      array = point_data->GetArray(this->CurrentVariable.toAscii().data());
      }
      
    if(!array)
      {
      if(vtkCellData* const cell_data = data->GetCellData())
        {
        array = cell_data->GetArray(this->CurrentVariable.toAscii().data());
        }
      }
      
    if(!array)
      return;

    double value_min = VTK_DOUBLE_MAX;
    double value_max = -VTK_DOUBLE_MAX;
    const unsigned int bin_count = 10;
    typedef vtkstd::vector<int> bins_t;
    bins_t bins(bin_count, 0);

    for(vtkIdType i = 0; i != array->GetNumberOfTuples(); ++i)
      {
      const double value = array->GetTuple1(i);
      value_min = vtkstd::min(value_min, value);
      value_max = vtkstd::max(value_max, value);
      }
      
    for(unsigned long bin = 0; bin != bin_count; ++bin)
      {
      const double bin_min = lerp(value_min, value_max, static_cast<double>(bin) / static_cast<double>(bin_count));
      const double bin_max = lerp(value_min, value_max, static_cast<double>(bin+1) / static_cast<double>(bin_count));
      
      for(vtkIdType i = 0; i != array->GetNumberOfTuples(); ++i)
        {
        const double value = array->GetTuple1(i);
        if(bin_min <= value && value <= bin_max)
          {
          ++bins[bin];
          }
        }
      }

    pqChartValueList list;
    for(bins_t::const_iterator bin = bins.begin(); bin != bins.end(); ++bin)
      {
      list.pushBack(static_cast<double>(*bin));
      }

    if(!bins.empty())
      {
      this->HistogramWidget.getHistogram()->setData(
        list,
        pqChartValue(value_min), pqChartValue(value_max));
      }
  }
  
  vtkSMClientSideDataProxy* ClientSideData;
  vtkEventQtSlotConnect* EventAdaptor;
  QComboBox Variables;
  pqHistogramWidget HistogramWidget;
  QString CurrentVariable;
};

/////////////////////////////////////////////////////////////////////////////////
// pqObjectHistogramWidget

pqObjectHistogramWidget::pqObjectHistogramWidget(QWidget *parent) :
  QWidget(parent),
  Implementation(new pqImplementation())
{
  QVBoxLayout* const layout = new QVBoxLayout(this);
  layout->setMargin(0);
  layout->addWidget(&this->Implementation->Variables);
  layout->addWidget(&this->Implementation->HistogramWidget);
  
  QObject::connect(
    &this->Implementation->Variables,
    SIGNAL(activated(int)),
    this,
    SLOT(onCurrentVariableChanged(int)));
}

pqObjectHistogramWidget::~pqObjectHistogramWidget()
{
  delete this->Implementation;
}

void pqObjectHistogramWidget::onSetProxy(vtkSMSourceProxy* proxy)
{
  this->Implementation->setProxy(proxy);
  
  if(proxy)
    {
    this->Implementation->EventAdaptor->Connect(
      proxy,
      vtkCommand::UpdateEvent,
      this,
      SLOT(onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)));    
    }
}

void pqObjectHistogramWidget::onSetCurrentVariable(const QString& CurrentVariable)
{
  this->Implementation->setCurrentVariable(CurrentVariable);
}

void pqObjectHistogramWidget::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}

void pqObjectHistogramWidget::onCurrentVariableChanged(int Row)
{
  this->Implementation->setCurrentVariable(this->Implementation->Variables.itemData(Row).toString());
}
