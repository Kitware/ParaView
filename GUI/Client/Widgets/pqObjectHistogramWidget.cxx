/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

=========================================================================*/

#include "pqObjectHistogramWidget.h"
#include "pqServer.h"

#include <pqChartAxis.h>
#include <pqChartLabel.h>
#include <pqChartValue.h>
#include <pqConnect.h>
#include <pqHistogramChart.h>
#include <pqHistogramColor.h>
#include <pqHistogramWidget.h>
#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>
#include <pqPipelineData.h>
#include <pqPipelineObject.h>

#include <QLabel>
#include <QPrinter>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkProcessModule.h>
#include <vtkSMClientSideDataProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMCompoundProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMLookupTableProxy.h>

//////////////////////////////////////////////////////////////////////////////
// pqHistogramColorLookup

/// Histogram color policy that uses a VTK lookup table
class pqHistogramColorLookup : public pqHistogramColor
{
public:
  pqHistogramColorLookup(pqChartAxis* const axis, vtkLookupTable* const table) :
    ChartAxis(axis),
    LookupTable(table)
  {
  }

private:
  QColor getColor(int index, int total) const
  {
    if(this->ChartAxis && this->LookupTable)
    {
      const double lower_value = this->ChartAxis->getValueForIndex(index).getDoubleValue();
      const double upper_value = this->ChartAxis->getValueForIndex(index+1).getDoubleValue();
      const double value = (lower_value + upper_value) * 0.5;
      
      double rgb[3];
      this->LookupTable->GetColor(value, rgb);

      return QColor::fromRgbF(rgb[0], rgb[1], rgb[2]);
    }
   
    return pqHistogramColor::getColor(index, total);
  }
  
  pqChartAxis* const ChartAxis;
  vtkLookupTable* const LookupTable;
};

//////////////////////////////////////////////////////////////////////////////
// pqObjectHistogramWidget::pqImplementation

struct pqObjectHistogramWidget::pqImplementation
{
  pqImplementation() :
    CurrentProxy(0),
    ClientSideData(0),
    EventAdaptor(vtkEventQtSlotConnect::New()),
    VariableType(VARIABLE_TYPE_CELL),
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
    
    this->HistogramWidget.getHistogram().setBinOutlineStyle(pqHistogramChart::Black);
    
    this->HistogramWidget.getHorizontalAxis().setGridColor(Qt::lightGray);
    this->HistogramWidget.getHorizontalAxis().setAxisColor(Qt::darkGray);
    this->HistogramWidget.getHorizontalAxis().setTickLabelColor(Qt::darkGray);
    this->HistogramWidget.getHorizontalAxis().setTickLabelFont(italic);
    
    this->HistogramWidget.getHorizontalAxis().getLabel().setText("Value");
    this->HistogramWidget.getHorizontalAxis().getLabel().setFont(bold);

    this->HistogramWidget.getHistogramAxis().setGridColor(Qt::lightGray);
    this->HistogramWidget.getHistogramAxis().setAxisColor(Qt::darkGray);
    this->HistogramWidget.getHistogramAxis().setTickLabelColor(Qt::darkGray);
    this->HistogramWidget.getHistogramAxis().setTickLabelFont(italic);
    this->HistogramWidget.getHistogramAxis().setPrecision(0);

    this->HistogramWidget.getHistogramAxis().getLabel().setText("Count");
    this->HistogramWidget.getHistogramAxis().getLabel().setFont(bold);
    this->HistogramWidget.getHistogramAxis().getLabel().setOrientation(pqChartLabel::VERTICAL);
    
    this->updateChart();
  }
  
  ~pqImplementation()
  {
    this->EventAdaptor->Delete();
    
    if(this->ClientSideData)
      {
      /** \todo Plug this leak */
//      this->ClientSideData->Delete();
      this->ClientSideData = 0;
      }
  }
  
  void setProxy(vtkSMProxy* Proxy)
  {
    if(this->ClientSideData)
      {
      /** \todo Plug this leak */
//      this->ClientSideData->Delete();
      this->ClientSideData = 0;
      }  
    
    this->VariableType = VARIABLE_TYPE_CELL;
    this->VariableName = QString();
    
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
    
    // Retain the proxy for later use ...
    this->CurrentProxy = Proxy;

    if(Proxy)
      {
      // Connect a client side data object to the input source
      this->ClientSideData = vtkSMClientSideDataProxy::SafeDownCast(
        Proxy->GetProxyManager()->NewProxy("displays", "ClientSideData"));
      vtkSMInputProperty* const input = vtkSMInputProperty::SafeDownCast(
        this->ClientSideData->GetProperty("Input"));
      input->AddProxy(Proxy);
      }
    
    this->onInputChanged();
  }
  
  void setVariable(pqVariableType type, const QString& name)
  {
    this->VariableType = type;
    this->VariableName = name;
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
    if(this->ClientSideData)
      {
      this->ClientSideData->UpdateVTKObjects();
      this->ClientSideData->Update();
      }

    this->updateChart();
  }
  
  static inline double lerp(double A, double B, double Amount)
  {
    return ((1 - Amount) * A) + (Amount * B);
  }
  
  void updateChart()
  {
    // Set the default (no data) appearance of the chart
    this->HistogramWidget.getHistogram().clearData();
    this->HistogramWidget.getTitle().setText("Histogram (no data)");
    this->HistogramWidget.getHistogramAxis().setVisible(true);
    this->HistogramWidget.getHistogramAxis().setValueRange(0.0, 100.0);
    this->HistogramWidget.getHorizontalAxis().setVisible(true);
    this->HistogramWidget.getHorizontalAxis().setValueRange(0.0, 100.0);
    this->HistogramWidget.getHistogram().setBinColorScheme(new pqHistogramColor());
    
    // See if we can display the current variable, if not, we're done ...
    if(this->VariableName.isEmpty())
      return;
    
    if(!this->ClientSideData)
      return;

    vtkDataSet* const data = this->ClientSideData->GetCollectedData();
    if(!data)
      return;

    vtkDataArray* array = 0;
    switch(this->VariableType)
      {
      case VARIABLE_TYPE_NODE:
        array = data->GetPointData()->GetArray(this->VariableName.toAscii().data());
        break;
      case VARIABLE_TYPE_CELL:
        array = data->GetCellData()->GetArray(this->VariableName.toAscii().data());
        break;
      }
      
    if(!array)
      return;

    if(array->GetNumberOfComponents() != 1)
      return;

    // Bin the current variable data ...
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
    
    // Display the results ...
    this->HistogramWidget.getTitle().setText(this->VariableName + " Histogram");
    this->HistogramWidget.getHistogramAxis().setVisible(true);
    this->HistogramWidget.getHorizontalAxis().setVisible(true);

    this->HistogramWidget.getHistogram().setData(
      list,
      pqChartValue(value_min), pqChartValue(value_max));

    // Setup the histogram to use the same color as the display ...
    pqPipelineObject* const pipeline_object = pqPipelineData::instance()->getObjectFor(this->CurrentProxy);
    if(!pipeline_object)
      return;
      
    vtkSMDisplayProxy* const display_proxy = pipeline_object->GetDisplayProxy();
    if(!display_proxy)
      return;
      
    vtkSMProxyProperty* const lookup_table_property = vtkSMProxyProperty::SafeDownCast(display_proxy->GetProperty("LookupTable"));
    if(!lookup_table_property)
      return;
      
    vtkSMLookupTableProxy* const lookup_table_proxy = vtkSMLookupTableProxy::SafeDownCast(lookup_table_property->GetProxy(0));
    if(!lookup_table_proxy)
      return;
      
    vtkLookupTable* const lookup_table = vtkLookupTable::SafeDownCast(vtkProcessModule::GetProcessModule()->GetObjectFromID(lookup_table_proxy->GetID(0)));
    if(!lookup_table)
      return;

    this->HistogramWidget.getHistogram().setBinColorScheme(new pqHistogramColorLookup(&this->HistogramWidget.getHorizontalAxis(), lookup_table));
  }
  
  vtkSMProxy* CurrentProxy;
  vtkSMClientSideDataProxy* ClientSideData;
  vtkEventQtSlotConnect* EventAdaptor;
  QSpinBox BinCountSpinBox;
  pqHistogramWidget HistogramWidget;
  pqVariableType VariableType;
  QString VariableName;
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

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->setMargin(0);
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
    &this->Implementation->BinCountSpinBox,
    SIGNAL(valueChanged(int)),
    this,
    SLOT(onBinCountChanged(int)));
}

pqObjectHistogramWidget::~pqObjectHistogramWidget()
{
  delete this->Implementation;
}

void pqObjectHistogramWidget::setServer(pqServer* /*server*/)
{
  this->setProxy(0);
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

void pqObjectHistogramWidget::setVariable(pqVariableType type, const QString& name)
{
  this->Implementation->setVariable(type, name);
}

void pqObjectHistogramWidget::setBinCount(unsigned long Count)
{
  this->Implementation->setBinCount(Count);
}

void pqObjectHistogramWidget::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}

void pqObjectHistogramWidget::onBinCountChanged(int Count)
{
  this->Implementation->setBinCount(Count);
}
