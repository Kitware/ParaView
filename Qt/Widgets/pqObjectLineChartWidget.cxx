/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqObjectLineChartWidget.h"
#include "pqServer.h"

#include <pqChartAxis.h>
#include <pqChartLabel.h>
#include <pqChartLegend.h>
#include <pqLineChart.h>
#include <pqLineChartWidget.h>
#include <pqPiecewiseLine.h>
#include <pqLinearRamp.h>

#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkSMClientSideDataProxy.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMCompoundProxy.h>
#include <vtkSphereSource.h>

#include <vtkExodusReader.h>
#include <vtkProcessModule.h>

//////////////////////////////////////////////////////////////////////////////
// pqObjectLineChartWidget::pqImplementation

struct pqObjectLineChartWidget::pqImplementation
{
  pqImplementation() :
    SourceProxy(0),
    ClientSideData(0),
    EventAdaptor(vtkEventQtSlotConnect::New()),
    CurrentVariable(""),
    CurrentElementID(0)
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
    
    QFont bold_italic;
    bold_italic.setBold(true);
    bold_italic.setItalic(true);
    bold_italic.setStyleStrategy(QFont::PreferAntialias);
    
    this->LineChartWidget.setBackgroundColor(Qt::white);
    
    this->LineChartWidget.getTitle().setFont(h1);
    this->LineChartWidget.getTitle().setColor(Qt::black);
    
    this->LineChartWidget.getXAxis()->setGridColor(Qt::lightGray);
    this->LineChartWidget.getXAxis()->setAxisColor(Qt::darkGray);
    this->LineChartWidget.getXAxis()->setTickLabelColor(Qt::darkGray);
    this->LineChartWidget.getXAxis()->setTickLabelFont(italic);

    this->LineChartWidget.getXAxis()->getLabel().setText("Time");
    this->LineChartWidget.getXAxis()->getLabel().setFont(bold);
    
    this->LineChartWidget.getYAxis()->setGridColor(Qt::lightGray);
    this->LineChartWidget.getYAxis()->setAxisColor(Qt::darkGray);
    this->LineChartWidget.getYAxis()->setTickLabelColor(Qt::darkGray);
    this->LineChartWidget.getYAxis()->setTickLabelFont(italic);

    this->LineChartWidget.getYAxis()->getLabel().setFont(bold);
    this->LineChartWidget.getYAxis()->getLabel().setOrientation(pqChartLabel::VERTICAL);
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

    this->SourceProxy = Proxy;

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
  
  void setCurrentVariable(const QString& Variable)
  {
    this->CurrentVariable = Variable;
    this->updateChart();
  }
  
  void setCurrentElementID(unsigned long e)
  {
    this->CurrentElementID = e;
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
  
  void addPlot(vtkExodusReader& Reader, const int ID, const QPen& Pen)
  {
    const int id = ID + 1; // Exodus expects one-based cell ids
    QByteArray name = this->CurrentVariable.toAscii();
    const char* type = "CELL";
    vtkFloatArray* const array = vtkFloatArray::New();
    Reader.GetTimeSeriesData(id, name.data(), type, array); 
    
    pqChartCoordinateList coordinates;
    for(vtkIdType i = 0; i != array->GetNumberOfTuples(); ++i)
      {
      double value = array->GetValue(i);
      coordinates.pushBack(pqChartCoordinate(static_cast<double>(i), value));
      }

    array->Delete();
      
    pqPiecewiseLine* const plot = new pqPiecewiseLine();
    plot->setCoordinates(coordinates);
    plot->setPen(Pen);
    
    this->LineChartWidget.getLineChart()->addData(plot);
  }
  
  void updateChart()
  {
    this->LineChartWidget.getLineChart()->clearData();
    this->LineChartWidget.getTitle().setText("");
    this->LineChartWidget.getXAxis()->setVisible(false);
    this->LineChartWidget.getYAxis()->setVisible(false);
    
    if(this->CurrentVariable.isEmpty())
      return;
    
    if(!this->SourceProxy)
      return;

    const QString source_class = SourceProxy->GetVTKClassName();
    if(source_class != "vtkExodusReader" && source_class != "vtkPExodusReader")
      return;

    vtkProcessModule* const process_module = vtkProcessModule::GetProcessModule();
    vtkExodusReader* const reader = vtkExodusReader::SafeDownCast(
      process_module->GetObjectFromID(this->SourceProxy->GetID(0)));
    if(!reader)
      return;

    this->LineChartWidget.getTitle().setText(this->CurrentVariable + " vs. Time");
    this->LineChartWidget.getXAxis()->setVisible(true);
    this->LineChartWidget.getYAxis()->setVisible(true);
    this->LineChartWidget.getYAxis()->getLabel().setText(this->CurrentVariable);

    this->LineChartWidget.getLegend().clear();
    this->LineChartWidget.getLegend().addEntry(QPen(Qt::darkBlue, 1.5), new pqChartLabel(QString("Element %1").arg(this->CurrentElementID)));
    this->LineChartWidget.getLegend().addEntry(QPen(Qt::darkGray, 1.0, Qt::DotLine), new pqChartLabel(QString("Element %1").arg(this->CurrentElementID + 1)));
    this->LineChartWidget.getLegend().addEntry(QPen(Qt::lightGray, 1.0, Qt::DotLine), new pqChartLabel(QString("Element %1").arg(this->CurrentElementID + 2)));
    
    addPlot(*reader, this->CurrentElementID, QPen(Qt::darkBlue, 1.5));
    addPlot(*reader, this->CurrentElementID + 1, QPen(Qt::darkGray, 1.0, Qt::DotLine));
    addPlot(*reader, this->CurrentElementID + 2, QPen(Qt::lightGray, 1.0, Qt::DotLine));
  }
  
  vtkSMProxy* SourceProxy;
  vtkSMClientSideDataProxy* ClientSideData;
  vtkEventQtSlotConnect* EventAdaptor;
  QComboBox Variables;
  QSpinBox ElementID;
  pqLineChartWidget LineChartWidget;
  QString CurrentVariable;
  unsigned long CurrentElementID;
};

/////////////////////////////////////////////////////////////////////////////////
// pqObjectLineChartWidget

pqObjectLineChartWidget::pqObjectLineChartWidget(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  QLabel* const element_label = new QLabel(tr("Element:"));
  element_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->setMargin(0);
  hbox->addWidget(&this->Implementation->Variables, 4);
  hbox->addWidget(element_label);
  hbox->addWidget(&this->Implementation->ElementID, 1);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(&this->Implementation->LineChartWidget);
  this->setLayout(vbox);

  this->Implementation->ElementID.setMinimum(0);
  this->Implementation->ElementID.setMaximum(VTK_LONG_MAX);
  this->Implementation->ElementID.setValue(this->Implementation->CurrentElementID);

  QObject::connect(
    &this->Implementation->Variables,
    SIGNAL(activated(int)),
    this,
    SLOT(onCurrentVariableChanged(int)));
    
  QObject::connect(
    &this->Implementation->ElementID,
    SIGNAL(valueChanged(int)),
    this,
    SLOT(onElementIDChanged(int)));
}

pqObjectLineChartWidget::~pqObjectLineChartWidget()
{
  delete this->Implementation;
}

void pqObjectLineChartWidget::setProxy(vtkSMProxy* proxy)
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

void pqObjectLineChartWidget::setCurrentVariable(const QString& CurrentVariable)
{
  this->Implementation->setCurrentVariable(CurrentVariable);
}

void pqObjectLineChartWidget::setElementID(unsigned long ElementID)
{
  this->Implementation->setCurrentElementID(ElementID);
}

void pqObjectLineChartWidget::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}

void pqObjectLineChartWidget::onCurrentVariableChanged(int Row)
{
  this->Implementation->setCurrentVariable(this->Implementation->Variables.itemData(Row).toString());
}

void pqObjectLineChartWidget::onElementIDChanged(int ElementID)
{
  this->Implementation->setCurrentElementID(ElementID);
}
