/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "ChartAdapter.h"

#include <pqChartAxis.h>
#include <pqChartLabel.h>
#include <pqHistogramChart.h>
#include <pqHistogramColor.h>
#include <pqHistogramListModel.h>
#include <pqHistogramWidget.h>

#include <vtkAlgorithm.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkProcessModule.h>
#include <vtkRectilinearGrid.h>
#include <vtkSMCompoundProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSmartPointer.h>
#include <vtkUnsignedLongArray.h>

class pqHistogramColorRange : public pqHistogramColor
{
public:
  pqHistogramColorRange(const QColor& begin, const QColor& end) :
    Begin(begin),
    End(end)
  {
  }
  
private:
  static const QColor lerp(const QColor& A, const QColor& B, const double Mix)
  {
    return QColor::fromRgbF(
      (A.redF() * (1.0 - Mix)) + (B.redF() * Mix),
      (A.greenF() * (1.0 - Mix)) + (B.greenF() * Mix),
      (A.blueF() * (1.0 - Mix)) + (B.blueF() * Mix),
      (A.alphaF() * (1.0 - Mix)) + (B.alphaF() * Mix));
  }

  QColor getColor(int index, int total) const
  {
    if(!total)
      return Begin;
      
    return lerp(Begin, End, static_cast<double>(index) / static_cast<double>(total));
  }
  
  const QColor Begin;
  const QColor End;
};

//////////////////////////////////////////////////////////////////////////////
// ChartAdapter::pqImplementation

struct ChartAdapter::pqImplementation
{
  pqImplementation(pqHistogramWidget& chart) :
    Chart(chart),
    Model(new pqHistogramListModel()),
    EventAdaptor(vtkSmartPointer<vtkEventQtSlotConnect>::New()),
    SourceProxy(0)
  {
    QFont h1;
    h1.setBold(true);
    h1.setPointSize(12);
    h1.setStyleStrategy(QFont::PreferAntialias);
  
    QFont bold;
    bold.setBold(true);
    bold.setStyleStrategy(QFont::PreferAntialias);
  
    QFont italic;
    italic.setItalic(true);
    italic.setStyleStrategy(QFont::PreferAntialias);

    this->Chart.setBackgroundColor(Qt::white);
    
    this->Chart.getTitle().setFont(h1);
    this->Chart.getTitle().setColor(Qt::black);
    
    this->Chart.getHistogram().setBinColorScheme(new pqHistogramColorRange(Qt::blue, Qt::red));
    this->Chart.getHistogram().setBinOutlineStyle(pqHistogramChart::Black);
    
    this->Chart.getHorizontalAxis().setGridColor(Qt::lightGray);
    this->Chart.getHorizontalAxis().setAxisColor(Qt::darkGray);
    this->Chart.getHorizontalAxis().setTickLabelColor(Qt::darkGray);
    this->Chart.getHorizontalAxis().setTickLabelFont(italic);
    
    this->Chart.getHorizontalAxis().getLabel().setText("Value");
    this->Chart.getHorizontalAxis().getLabel().setFont(bold);

    this->Chart.getHistogramAxis().setGridColor(Qt::lightGray);
    this->Chart.getHistogramAxis().setAxisColor(Qt::darkGray);
    this->Chart.getHistogramAxis().setTickLabelColor(Qt::darkGray);
    this->Chart.getHistogramAxis().setTickLabelFont(italic);
    this->Chart.getHistogramAxis().setPrecision(0);

    this->Chart.getHistogramAxis().getLabel().setText("Count");
    this->Chart.getHistogramAxis().getLabel().setFont(bold);
    this->Chart.getHistogramAxis().getLabel().setOrientation(pqChartLabel::VERTICAL);

    this->Chart.getHistogram().setModel(this->Model);
    
    this->updateChart();
  }
  
  ~pqImplementation()
  {
    delete this->Model;
  }
  
  void setSource(vtkSMProxy* Proxy)
  {
    this->SourceProxy = Proxy;

    // TODO: hack -- figure out how compound proxies really fit in
    vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(Proxy);
    if(cp)
      {
        Proxy = NULL;
        for(int i=cp->GetNumberOfProxies(); Proxy == NULL && i>0; i--)
          {
          Proxy = vtkSMSourceProxy::SafeDownCast(cp->GetProxy(i-1));
          }
      }

    this->onInputChanged();
  }
  
  void onInputChanged()
  {
    this->updateChart();
  }
  
  void updateChart()
  {
    // Set the default (no data) appearance of the chart
    this->Model->clearBinValues();
    this->Chart.getTitle().setText("Histogram (no data)");
    this->Chart.getHistogramAxis().setVisible(true);
    this->Chart.getHistogramAxis().setValueRange(0.0, 100.0);
    this->Chart.getHorizontalAxis().setVisible(true);
    this->Chart.getHorizontalAxis().setValueRange(0.0, 100.0);
    
    if(!this->SourceProxy)
      return;

    vtkAlgorithm* const algorithm = vtkAlgorithm::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetObjectFromID(this->SourceProxy->GetID(0)));
      
    if(!algorithm)
      return;
    algorithm->Update();
      
    vtkRectilinearGrid* const histogram = vtkRectilinearGrid::SafeDownCast(algorithm->GetOutputDataObject(0));
    if(!histogram)
      return;
    

    histogram->Print(cout);

    vtkDoubleArray* const bin_extents = vtkDoubleArray::SafeDownCast(histogram->GetXCoordinates());
    if(!bin_extents)
      return;
    if(bin_extents->GetNumberOfComponents() != 1)
      return;
    if(bin_extents->GetNumberOfTuples() < 2)
      return;
    
    const double value_min = bin_extents->GetValue(0);
    const double value_max = bin_extents->GetValue(bin_extents->GetNumberOfTuples() - 1);
    
    vtkUnsignedLongArray* const bin_values = vtkUnsignedLongArray::SafeDownCast(histogram->GetCellData()->GetArray("bin_values"));
    if(!bin_values)
      return;
    if(bin_values->GetNumberOfComponents() != 1)
      return;
      
    pqChartValueList list;
    for(int i = 0; i != bin_values->GetNumberOfTuples(); ++i)
      {
      list.pushBack(static_cast<double>(bin_values->GetValue(i)));
      }

    this->Chart.getTitle().setText("Histogram");
    this->Chart.getHistogramAxis().setVisible(true);
    this->Chart.getHorizontalAxis().setVisible(true);
    this->Model->setRangeX(pqChartValue(value_min), pqChartValue(value_max));
    this->Model->setBinValues(list);
  }
  
  pqHistogramWidget& Chart;
  pqHistogramListModel *Model;
  vtkSmartPointer<vtkEventQtSlotConnect> EventAdaptor;
  vtkSMProxy* SourceProxy;
};

/////////////////////////////////////////////////////////////////////////////////
// ChartAdapter

ChartAdapter::ChartAdapter(pqHistogramWidget& chart) :
  Implementation(new pqImplementation(chart))
{
}

ChartAdapter::~ChartAdapter()
{
  delete this->Implementation;
}

void ChartAdapter::setSource(vtkSMProxy* proxy)
{
  this->Implementation->setSource(proxy);
  
  if(proxy)
    {
    this->Implementation->EventAdaptor->Connect(
      proxy,
      vtkCommand::UpdateEvent,
      this,
      SLOT(onInputChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*)));    
    }
}

void ChartAdapter::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}
