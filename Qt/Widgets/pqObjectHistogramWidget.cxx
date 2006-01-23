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

#include <pqChartAxis.h>
#include <pqChartLabel.h>
#include <pqChartValue.h>
#include <pqHistogramChart.h>
#include <pqHistogramColor.h>
#include <pqHistogramWidget.h>

#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
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
#include <vtkSMCompoundProxy.h>
#include <vtkSphereSource.h>

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
// pqObjectHistogramWidget::pqImplementation

struct pqObjectHistogramWidget::pqImplementation
{
  pqImplementation() :
    ClientSideData(0),
    EventAdaptor(vtkEventQtSlotConnect::New()),
    BinCount(10)
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

    this->HistogramWidget.setBackgroundColor(Qt::white);
    
    this->HistogramWidget.getTitle().setFont(h1);
    this->HistogramWidget.getTitle().setColor(Qt::black);
    
    this->HistogramWidget.getHistogram()->setBinColorScheme(new pqHistogramColorRange(Qt::blue, Qt::red));
    this->HistogramWidget.getHistogram()->setBinOutlineStyle(pqHistogramChart::Black);
    
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setGridColor(Qt::lightGray);
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setAxisColor(Qt::darkGray);
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setTickLabelColor(Qt::darkGray);
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setTickLabelFont(italic);
    
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->getLabel().setText("Value");
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->getLabel().setFont(bold);

    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setGridColor(Qt::lightGray);
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setAxisColor(Qt::darkGray);
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setTickLabelColor(Qt::darkGray);
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setTickLabelFont(italic);
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setPrecision(0);

    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->getLabel().setText("Count");
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->getLabel().setFont(bold);
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->getLabel().setOrientation(pqChartLabel::VERTICAL);
    
    this->updateChart();
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
  
  void setProxy(vtkSMProxy* Proxy)
  {
    if(this->ClientSideData)
      {
      this->ClientSideData->Delete();
      this->ClientSideData = 0;
      }  
    
    this->CurrentVariable = QString();
    
    // TODO: hack -- figure out how compound proxies really fit in
    vtkSMCompoundProxy* cp = vtkSMCompoundProxy::SafeDownCast(Proxy);
    if(cp)
      {
        Proxy = cp->GetProxy(cp->GetNumberOfProxies()-1);
      }
    
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
  
  void setCurrentVariable(const QString& c)
  {
    this->CurrentVariable = c;
    this->updateChart();
  }
  
  void setBinCount(unsigned long Count)
  {
    if(Count < 2)
      return;
      
    this->BinCount = Count;
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
    int current_index = 0;
    
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
        
        if(this->CurrentVariable == array_name)
          {
          current_index = this->Variables.count() - 1;
          }
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
        
        if(this->CurrentVariable == array_name)
          {
          current_index = this->Variables.count() - 1;
          }
        }
      }

    this->Variables.setCurrentIndex(current_index);
    this->updateChart();
  }
  
  static inline double lerp(double A, double B, double Amount)
  {
    return ((1 - Amount) * A) + (Amount * B);
  }
  
  void updateChart()
  {
    // Set the default (no data) appearance of the chart
    this->HistogramWidget.getHistogram()->clearData();
    this->HistogramWidget.getTitle().setText("Histogram (no data)");
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setVisible(true);
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setValueRange(0.0, 100.0);
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setVisible(true);
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setValueRange(0.0, 100.0);
    
    if(this->CurrentVariable.isEmpty())
      return;
    
    // what about cell and point arrays with the same name?
    this->Variables.setCurrentIndex(this->Variables.findText(this->CurrentVariable, Qt::MatchStartsWith));
    
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

    if(array->GetNumberOfComponents() != 1)
      return;

    double value_min = VTK_DOUBLE_MAX;
    double value_max = -VTK_DOUBLE_MAX;
    typedef vtkstd::vector<int> bins_t;
    bins_t bins(this->BinCount, 0);

    for(vtkIdType i = 0; i != array->GetNumberOfTuples(); ++i)
      {
      const double value = array->GetTuple1(i);
      value_min = vtkstd::min(value_min, value);
      value_max = vtkstd::max(value_max, value);
      }
      
    for(unsigned long bin = 0; bin != this->BinCount; ++bin)
      {
      const double bin_min = lerp(value_min, value_max, static_cast<double>(bin) / static_cast<double>(this->BinCount));
      const double bin_max = lerp(value_min, value_max, static_cast<double>(bin+1) / static_cast<double>(this->BinCount));
      
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

    if(bins.empty())
      return;
      
    if(value_min == value_max)
      return;
      
    this->HistogramWidget.getTitle().setText(this->CurrentVariable + " Histogram");
    this->HistogramWidget.getAxis(pqHistogramWidget::HistogramAxis)->setVisible(true);
    this->HistogramWidget.getAxis(pqHistogramWidget::HorizontalAxis)->setVisible(true);

    this->HistogramWidget.getHistogram()->setData(
      list,
      pqChartValue(value_min), pqChartValue(value_max));
  }
  
  vtkSMClientSideDataProxy* ClientSideData;
  vtkEventQtSlotConnect* EventAdaptor;
  QComboBox Variables;
  QSpinBox BinCountSpinBox;
  pqHistogramWidget HistogramWidget;
  QString CurrentVariable;
  unsigned long BinCount;
};

/////////////////////////////////////////////////////////////////////////////////
// pqObjectHistogramWidget

pqObjectHistogramWidget::pqObjectHistogramWidget(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  QLabel* const bin_label = new QLabel(tr("Bins:"));
  bin_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  this->Implementation->BinCountSpinBox.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->setMargin(0);
  hbox->addWidget(&this->Implementation->Variables);
  hbox->addWidget(bin_label);
  hbox->addWidget(&this->Implementation->BinCountSpinBox);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(&this->Implementation->HistogramWidget);
  
  this->Implementation->BinCountSpinBox.setMinimum(2);
  this->Implementation->BinCountSpinBox.setMaximum(256);
  this->Implementation->BinCountSpinBox.setValue(this->Implementation->BinCount);
  
  this->setLayout(vbox);
  
  QObject::connect(
    &this->Implementation->Variables,
    SIGNAL(activated(int)),
    this,
    SLOT(onCurrentVariableChanged(int)));
    
  QObject::connect(
    &this->Implementation->BinCountSpinBox,
    SIGNAL(valueChanged(int)),
    this,
    SLOT(onBinCountChanged(int)));
}

pqObjectHistogramWidget::~pqObjectHistogramWidget()
{
  delete this->Implementation;
}

void pqObjectHistogramWidget::setProxy(vtkSMProxy* proxy)
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

void pqObjectHistogramWidget::setCurrentVariable(const QString& CurrentVariable)
{
  this->Implementation->setCurrentVariable(CurrentVariable);
}

void pqObjectHistogramWidget::setBinCount(unsigned long Count)
{
  this->Implementation->setBinCount(Count);
}

void pqObjectHistogramWidget::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}

void pqObjectHistogramWidget::onCurrentVariableChanged(int Row)
{
  this->Implementation->setCurrentVariable(this->Implementation->Variables.itemData(Row).toString());
}

void pqObjectHistogramWidget::onBinCountChanged(int Count)
{
  this->Implementation->setBinCount(Count);
}
