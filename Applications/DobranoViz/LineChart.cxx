/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "ImageLinePlot.h"
#include "LineChart.h"

#include <pqConnect.h>
#include <pqDelimitedTextParser.h>
#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>
#include <pqServer.h>
#include <pqChartAxis.h>
#include <pqChartLabel.h>
#include <pqChartLegend.h>
#include <pqLineChart.h>
#include <pqLineChartWidget.h>
#include <pqLinearRamp.h>

#include <QHBoxLayout>
#include <QPrinter>
#include <QPushButton>
#include <QVBoxLayout>

#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMCompoundProxy.h>
#include <vtkSphereSource.h>
#include <vtkUnstructuredGrid.h>

#include <vtkExodusReader.h>
#include <vtkProcessModule.h>

#include <vtkstd/algorithm>
#include <vtkstd/vector>

//////////////////////////////////////////////////////////////////////////////
// LineChart::pqImplementation

struct LineChart::pqImplementation
{
  pqImplementation() :
    SourceProxy(0),
    EventAdaptor(vtkEventQtSlotConnect::New()),
    ExodusVariableType(VARIABLE_TYPE_CELL)
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
    
    this->LineChartWidget.getXAxis().setGridColor(Qt::lightGray);
    this->LineChartWidget.getXAxis().setAxisColor(Qt::darkGray);
    this->LineChartWidget.getXAxis().setTickLabelColor(Qt::darkGray);
    this->LineChartWidget.getXAxis().setTickLabelFont(italic);

    this->LineChartWidget.getXAxis().getLabel().setText("Time");
    this->LineChartWidget.getXAxis().getLabel().setFont(bold);
    
    this->LineChartWidget.getYAxis().setGridColor(Qt::lightGray);
    this->LineChartWidget.getYAxis().setAxisColor(Qt::darkGray);
    this->LineChartWidget.getYAxis().setTickLabelColor(Qt::darkGray);
    this->LineChartWidget.getYAxis().setTickLabelFont(italic);

    this->LineChartWidget.getYAxis().getLabel().setFont(bold);
    this->LineChartWidget.getYAxis().getLabel().setOrientation(pqChartLabel::VERTICAL);
    
    this->updateChart();
  }
  
  ~pqImplementation()
  {
    this->EventAdaptor->Delete();
  }
  
  void setExodusProxy(vtkSMProxy* Proxy)
  {
    this->SourceProxy = Proxy;
    this->ExodusVariableName = QString();

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
  
  void setExodusVariable(pqVariableType type, const QString& name)
  {
    this->ExodusVariableType = type;
    this->ExodusVariableName = name;
    this->updateChart();
  }

  void internalAddExodusElements(vtkUnstructuredGrid* elements)
  {
    for(int i = 0; i != elements->GetCellData()->GetNumberOfArrays(); ++i)
      {
      const QString array_name = elements->GetCellData()->GetArrayName(i);
      if(array_name != "Id")
        continue;
      
      vtkDataArray* const data = elements->GetCellData()->GetArray(i);
      if(!data)
        break;
      
      for(int j = 0; j != elements->GetNumberOfCells(); ++j)
        {
        const unsigned long id = static_cast<unsigned long>(data->GetTuple1(j));
        if(!vtkstd::count(this->ExodusElements.begin(), this->ExodusElements.end(), id))
          {
          this->ExodusElements.push_back(id);
          }
        }
      
      break;
      }
  }
  
  void clearExodusElements()
  {
    this->ExodusElements.clear();
    this->updateChart();
  }
  
  void addExodusElements(vtkUnstructuredGrid* elements)
  {
    internalAddExodusElements(elements);
    this->updateChart();
  }
  
  void clearCSV()
  {
    this->CSVPlots.clear();
    this->updateChart();
  }
  
  void addCSV(const QStringList& plot)
  {
    this->CSVPlots.push_back(plot);
    this->updateChart();
  }
  
  void onInputChanged()
  {
    this->updateChart();
  }
  
  void addExodusPlot(vtkExodusReader& Reader, const int ElementID, const QPen& Pen)
  {
    const int id = ElementID + 1; // Exodus expects one-based cell ids
    const char* type = this->ExodusVariableType == VARIABLE_TYPE_CELL ? "CELL" : "POINT";
    vtkFloatArray* const array = vtkFloatArray::New();
    Reader.GetTimeSeriesData(id, this->ExodusVariableName.toAscii().data(), type, array); 
    
    pqChartCoordinateList coordinates;
    for(vtkIdType i = 0; i != array->GetNumberOfTuples(); ++i)
      {
      double value = array->GetValue(i);
      coordinates.pushBack(pqChartCoordinate(static_cast<double>(i), value));
      }

    array->Delete();
    
    ImageLinePlot* const plot = new ImageLinePlot(QPixmap());
    plot->setCoordinates(coordinates);
    plot->setPen(Pen);
    
    this->LineChartWidget.getLineChart().addData(plot);
    this->LineChartWidget.getLegend().addEntry(Pen, new pqChartLabel(QString("Element %1").arg(ElementID)));
  }
  
  void addCSVPlot(QStringList& Plot, const QPen& Pen)
  {
    if(Plot.size() < 3)
      return;
      
    pqChartCoordinateList coordinates;
    for(int i = 2; i < Plot.size(); ++i)
      {
      coordinates.pushBack(pqChartCoordinate(static_cast<double>(i - 2), Plot[i].toDouble()));
      }
      
    ImageLinePlot* const plot = new ImageLinePlot(QPixmap(Plot[1]));
    plot->setCoordinates(coordinates);
    plot->setPen(Pen);
    
    this->LineChartWidget.getLineChart().addData(plot);
    this->LineChartWidget.getLegend().addEntry(Pen, new pqChartLabel(Plot[0]));
  }
  
  void updateChart()
  {
    // Set the default (no data) appearance of the chart ...
    this->LineChartWidget.getLineChart().clearData();
    this->LineChartWidget.getTitle().setText("Time Plot (no data)");
    this->LineChartWidget.getXAxis().setVisible(true);
    this->LineChartWidget.getXAxis().setValueRange(0.0, 100.0);
    this->LineChartWidget.getYAxis().getLabel().setText("Value");
    this->LineChartWidget.getYAxis().setVisible(true);
    this->LineChartWidget.getYAxis().setValueRange(0.0, 100.0);
    this->LineChartWidget.getLegend().clear();
    
    if(CSVPlots.size())
      this->LineChartWidget.getTitle().setText("External Data");
      
    for(int i = 0; i != CSVPlots.size(); ++i)
      {
      const double hue = static_cast<double>(i) / static_cast<double>(CSVPlots.size());
      const QColor color = QColor::fromHsvF(hue, 1.0, 1.0);
      addCSVPlot(CSVPlots[i], QPen(color, 1.0, Qt::DotLine));
      }

    if(this->ExodusVariableName.isEmpty())
      return;

    if(this->ExodusElements.empty())
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

    int array_id = -1;
    switch(this->ExodusVariableType)
      {
      case VARIABLE_TYPE_CELL:
        array_id = reader->GetCellArrayID(this->ExodusVariableName.toAscii().data());
        if(-1 == array_id)
          return;
        break;
      case VARIABLE_TYPE_NODE:
        array_id = reader->GetPointArrayID(this->ExodusVariableName.toAscii().data());
        if(-1 == array_id)
          return;
        break;
      }

    if(this->CSVPlots.size())
      this->LineChartWidget.getTitle().setText(this->ExodusVariableName + " vs. Time / External Data");
    else
      this->LineChartWidget.getTitle().setText(this->ExodusVariableName + " vs. Time");
      
    this->LineChartWidget.getXAxis().setVisible(true);
    this->LineChartWidget.getYAxis().setVisible(true);
    this->LineChartWidget.getYAxis().getLabel().setText(this->ExodusVariableName);

    unsigned long count = 0;
    for(vtkstd::vector<unsigned long>::reverse_iterator element = this->ExodusElements.rbegin(); element != this->ExodusElements.rend(); ++element, ++count)
      {
      const double hue = static_cast<double>(count) / static_cast<double>(ExodusElements.size());
      const QColor color = QColor::fromHsvF(hue, 1.0, 1.0);
      addExodusPlot(*reader, *element, QPen(color, 1.5));
      }
  }
  
  vtkSMProxy* SourceProxy;
  vtkEventQtSlotConnect* EventAdaptor;
  pqLineChartWidget LineChartWidget;
  pqVariableType ExodusVariableType;
  QString ExodusVariableName;
  vtkstd::vector<unsigned long> ExodusElements;
  QVector<QStringList> CSVPlots;
};

/////////////////////////////////////////////////////////////////////////////////
// LineChart

LineChart::LineChart(QWidget *p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  QPushButton* const csv_button = new QPushButton("Load .csv");
  this->connect(csv_button, SIGNAL(clicked()), this, SLOT(onLoadCSV()));
  
  QPushButton* const clear_button = new QPushButton("Clear .csv");
  this->connect(clear_button, SIGNAL(clicked()), this, SLOT(clearCSV()));

  QPushButton* const save_button = new QPushButton("Save .pdf");
  this->connect(save_button, SIGNAL(clicked()), this, SLOT(onSavePDF()));

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->setMargin(0);
  hbox->addWidget(csv_button);
  hbox->addWidget(clear_button);
  hbox->addWidget(save_button);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(&this->Implementation->LineChartWidget);
  
  this->setLayout(vbox);
}

LineChart::~LineChart()
{
  delete this->Implementation;
}

void LineChart::setServer(pqServer* /*server*/)
{
  this->setExodusProxy(0);
}

void LineChart::setExodusProxy(vtkSMProxy* proxy)
{
  this->Implementation->setExodusProxy(proxy);
  
  if(proxy)
    {
    this->Implementation->EventAdaptor->Connect(
      proxy,
      vtkCommand::UpdateEvent,
      this,
      SLOT(onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)));    
    }
}

void LineChart::setExodusVariable(pqVariableType type, const QString& name)
{
  this->Implementation->setExodusVariable(type, name);
}

void LineChart::clearExodusElements()
{
  this->Implementation->clearExodusElements();
}

void LineChart::addExodusElements(vtkUnstructuredGrid* elements)
{
  this->Implementation->addExodusElements(elements);
}

void LineChart::setExodusElements(vtkUnstructuredGrid* elements)
{
  this->clearExodusElements();
  this->addExodusElements(elements);
}

void LineChart::clearCSV()
{
  this->Implementation->clearCSV();
}

void LineChart::addCSV(const QStringList& plot)
{
  this->Implementation->addCSV(plot);
}

void LineChart::onLoadCSV()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open .csv File:"), this, "fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this, SLOT(onLoadCSV(const QStringList&)));
    
  file_dialog->show();
}

void LineChart::onLoadCSV(const QStringList& files)
{
  pqDelimitedTextParser parser(pqDelimitedTextParser::COLUMN_SERIES, ',');
  QObject::connect(&parser, SIGNAL(parseSeries(const QStringList&)), this, SLOT(addCSV(const QStringList&)));

  for(int i = 0; i != files.size(); ++i)
    {
    parser.parse(files[i]);
    }
}

void LineChart::onSavePDF()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save .pdf File:"), this, "fileSavePDFDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this, SLOT(onSavePDF(const QStringList&)));
    
  file_dialog->show();
}

void LineChart::onSavePDF(const QStringList& files)
{
  for(int i = 0; i != files.size(); ++i)
    {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(files[i]);
    
    this->Implementation->LineChartWidget.printChart(printer);
    }
}

void LineChart::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}
