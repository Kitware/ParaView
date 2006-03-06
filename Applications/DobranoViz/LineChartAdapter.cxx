/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "ImageLinePlot.h"
#include "LineChartAdapter.h"

#include <pqConnect.h>
#include <pqChartCoordinate.h>
#include <pqDelimitedTextParser.h>
#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>
#include <pqServer.h>
#include <pqChartAxis.h>
#include <pqChartLabel.h>
#include <pqChartLegend.h>
#include <pqLineChart.h>
#include <pqLineChartWidget.h>
#include <pqLinePlot.h>
#include <pqMarkerPen.h>

#include <QDir>
#include <QFileInfo>
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
// LineChartAdapter::pqImplementation

struct LineChartAdapter::pqImplementation
{
  struct CSVPlot
  {
    QString Label;
    QPixmap Pixmap;
    double DeltaX;
    pqLineErrorPlot::CoordinatesT Coordinates;
  };

  pqImplementation(pqLineChartWidget& chart) :
    SourceProxy(0),
    EventAdaptor(vtkEventQtSlotConnect::New()),
    ExodusVariableType(VARIABLE_TYPE_CELL),
    Chart(chart),
    Samples(50),
    Differences(false)
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
    
    this->Chart.setBackgroundColor(Qt::white);
    
    this->Chart.getTitle().setFont(h1);
    this->Chart.getTitle().setColor(Qt::black);
    
    this->Chart.getXAxis().setGridColor(Qt::lightGray);
    this->Chart.getXAxis().setAxisColor(Qt::darkGray);
    this->Chart.getXAxis().setTickLabelColor(Qt::darkGray);
    this->Chart.getXAxis().setTickLabelFont(italic);

    this->Chart.getXAxis().getLabel().setText("Time");
    this->Chart.getXAxis().getLabel().setFont(bold);
    
    this->Chart.getYAxis().setGridColor(Qt::lightGray);
    this->Chart.getYAxis().setAxisColor(Qt::darkGray);
    this->Chart.getYAxis().setTickLabelColor(Qt::darkGray);
    this->Chart.getYAxis().setTickLabelFont(italic);

    this->Chart.getYAxis().getLabel().setFont(bold);
    this->Chart.getYAxis().getLabel().setOrientation(pqChartLabel::VERTICAL);
    
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

  void startParsing(const QString& file)
  {
  this->CSVFile = file;
  }
  
  void startParsing()
  {
    this->CSVSeries.clear();
  }

  void parseSeries(const QStringList& series)
  {
    this->CSVSeries.push_back(series);
  }
  
  void finishParsing()
  {
    if(this->CSVSeries.size() < 2)
      return;

    // The first column in the file will be time
    QStringList& time = this->CSVSeries[0];
    if(time.size() < 3)
      return;
    
    QVector<CSVPlot> plots;
    QVector<CSVPlot> difference_plots;
    
    // Next, each group of three columns form a plot with upper and lower error bounds
    for(int i = 1; i < this->CSVSeries.size() - 2; i += 3)
      {
      QStringList& values = this->CSVSeries[i];
      QStringList& upper_bounds = this->CSVSeries[i+1];
      QStringList& lower_bounds = this->CSVSeries[i+2];
      
      if(values.size() < 3)
        continue;

      CSVPlot plot;
      plot.Label = values[0];
      
      const QString pixmap_file = QFileInfo(this->CSVFile).absoluteDir().absoluteFilePath(QDir::cleanPath(values[1]));
      plot.Pixmap = QPixmap(pixmap_file);
      if(plot.Pixmap.width() > 300 || plot.Pixmap.height() > 300)
        plot.Pixmap = plot.Pixmap.scaled(QSize(300, 300), Qt::KeepAspectRatio, Qt::SmoothTransformation);
      
      plot.DeltaX = VTK_DOUBLE_MAX;
      
      const int sample_size = values.size() > 2 * this->Samples ? values.size() / this->Samples : 1;
      for(int i = 2; i < time.size() && i < values.size() && i < upper_bounds.size() && i < lower_bounds.size(); i += sample_size)
        {
        if(i != 2)
          {
          plot.DeltaX = vtkstd::min(plot.DeltaX, time[i].toDouble() - time[i - sample_size].toDouble());
          }
          
        plot.Coordinates.push_back(
          pqLineErrorPlot::Coordinate(
            time[i].toDouble(),
            values[i].toDouble(),
            upper_bounds[i].toDouble(),
            lower_bounds[i].toDouble()));
        }

      if(plot.Coordinates.size() > 1)
        {
        plots.push_back(plot);
        }
      }

    // Last, create a "difference" plot for each pair of input plots
    for(int i = 0; i < plots.size() - 1; i += 2)
      {
      CSVPlot& a = plots[i];
      CSVPlot& b = plots[i+1];
      
      CSVPlot c;
      c.Label = a.Label + " - " + b.Label;
      c.Pixmap = a.Pixmap;
      c.DeltaX = a.DeltaX;
      
      for(int i = 0; i < a.Coordinates.size() && i < b.Coordinates.size(); ++i)
        {
        c.Coordinates.push_back(
          pqLineErrorPlot::Coordinate(
            a.Coordinates[i].X,
            a.Coordinates[i].Y - b.Coordinates[i].Y,
            sqrt(pow(a.Coordinates[i].UpperBound, 2.0) + pow(b.Coordinates[i].UpperBound, 2.0)),
            sqrt(pow(a.Coordinates[i].LowerBound, 2.0) + pow(b.Coordinates[i].LowerBound, 2.0))));
        }
        
      difference_plots.push_back(c);
      }

    this->CSVPlots += plots;
    this->CSVDifferences += difference_plots;

    this->updateChart();
  }
  
  void clearCSV()
  {
    this->CSVPlots.clear();
    this->CSVDifferences.clear();
    
    this->updateChart();
  }
  
  void onInputChanged()
  {
    this->updateChart();
  }
  
  void setSamples(int samples)
  {
    this->Samples = samples;
    this->clearCSV();
  }
  
  void showDifferences(bool state)
  {
    this->Differences = state;
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
    
    pqLinePlot* const plot = new pqLinePlot(
      new pqCircleMarkerPen(Pen, QSize(4, 4), QPen(Pen.color(), 0.5), QBrush(Qt::white)),
      coordinates);
    
    this->Chart.getLineChart().addData(plot);
    this->Chart.getLegend().addEntry(new pqCircleMarkerPen(Pen, QSize(4, 4), QPen(Pen.color(), 0.5), QBrush(Qt::white)), new pqChartLabel(QString("Element %1").arg(ElementID)));
  }
  
  void addCSVPlot(const CSVPlot& plot, const QPen& plot_pen, const QPen& whisker_pen)
  {
    ImageLinePlot* const line_plot = new ImageLinePlot(
      new pqNullMarkerPen(plot_pen),
      whisker_pen,
      plot.DeltaX * 0.7,
      plot.Coordinates,
      plot.Pixmap);
    this->Chart.getLineChart().addData(line_plot);
    
    this->Chart.getLegend().addEntry(new pqNullMarkerPen(plot_pen), new pqChartLabel(plot.Label));
  }
  
  void updateChart()
  {
    // Set the default (no data) appearance of the chart ...
    this->Chart.getLineChart().clearData();
    this->Chart.getTitle().setText("Time Plot (no data)");
    this->Chart.getXAxis().setVisible(true);
    this->Chart.getXAxis().setValueRange(0.0, 100.0);
    this->Chart.getYAxis().getLabel().setText("Value");
    this->Chart.getYAxis().setVisible(true);
    this->Chart.getYAxis().setValueRange(0.0, 100.0);
    this->Chart.getLegend().clear();
    
    if(CSVPlots.size() || CSVDifferences.size())
      this->Chart.getTitle().setText("CSV Data");
      
    if(this->Differences)
      {
      for(int i = 0; i != CSVDifferences.size(); ++i)
        {
        const double hue = static_cast<double>(i) / static_cast<double>(CSVDifferences.size());
        const QColor color = QColor::fromHsvF(hue, 1.0, 0.5);
        addCSVPlot(CSVDifferences[i], QPen(color, 2), QPen(color, 1));
        }
      }
    else
      {
      for(int i = 0; i != CSVPlots.size(); ++i)
        {
        const double hue = static_cast<double>(i) / static_cast<double>(CSVPlots.size());
        const QColor color = QColor::fromHsvF(hue, 1.0, 0.5);
        addCSVPlot(CSVPlots[i], QPen(color, 2), QPen(color, 1));
        }
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
      this->Chart.getTitle().setText(this->ExodusVariableName + " vs. Time / External Data");
    else
      this->Chart.getTitle().setText(this->ExodusVariableName + " vs. Time");
      
    this->Chart.getXAxis().setVisible(true);
    this->Chart.getYAxis().setVisible(true);
    this->Chart.getYAxis().getLabel().setText(this->ExodusVariableName);

    unsigned long count = 0;
    for(vtkstd::vector<unsigned long>::reverse_iterator element = this->ExodusElements.rbegin(); element != this->ExodusElements.rend(); ++element, ++count)
      {
      const double hue = static_cast<double>(count) / static_cast<double>(ExodusElements.size());
      const QColor color = QColor::fromHsvF(hue, 1.0, 1.0);
      addExodusPlot(*reader, *element, QPen(color, 1.0));
      }
  }
  
  vtkSMProxy* SourceProxy;
  vtkEventQtSlotConnect* EventAdaptor;
  pqLineChartWidget& Chart;
  
  pqVariableType ExodusVariableType;
  QString ExodusVariableName;
  vtkstd::vector<unsigned long> ExodusElements;

  QString CSVFile;
  QVector<QStringList> CSVSeries;

  int Samples;
  bool Differences;
  QVector<CSVPlot> CSVPlots;
  QVector<CSVPlot> CSVDifferences;
};

/////////////////////////////////////////////////////////////////////////////////
// LineChartAdapter

LineChartAdapter::LineChartAdapter(pqLineChartWidget& chart) :
  Implementation(new pqImplementation(chart))
{
}

LineChartAdapter::~LineChartAdapter()
{
  delete this->Implementation;
}

void LineChartAdapter::setServer(pqServer* /*server*/)
{
  this->setExodusProxy(0);
}

void LineChartAdapter::setExodusProxy(vtkSMProxy* proxy)
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

void LineChartAdapter::setExodusVariable(pqVariableType type, const QString& name)
{
  this->Implementation->setExodusVariable(type, name);
}

void LineChartAdapter::clearExodusElements()
{
  this->Implementation->clearExodusElements();
}

void LineChartAdapter::addExodusElements(vtkUnstructuredGrid* elements)
{
  this->Implementation->addExodusElements(elements);
}

void LineChartAdapter::setExodusElements(vtkUnstructuredGrid* elements)
{
  this->clearExodusElements();
  this->addExodusElements(elements);
}

void LineChartAdapter::setSamples(int samples)
{
  this->Implementation->setSamples(samples);
}

void LineChartAdapter::showDifferences(bool state)
{
  this->Implementation->showDifferences(state);
}

void LineChartAdapter::clearCSV()
{
  this->Implementation->clearCSV();
}

void LineChartAdapter::onLoadCSV(const QStringList& files)
{
  pqDelimitedTextParser parser(pqDelimitedTextParser::COLUMN_SERIES, ',');
  QObject::connect(&parser, SIGNAL(startParsing()), this, SLOT(startParsing()));
  QObject::connect(&parser, SIGNAL(parseSeries(const QStringList&)), this, SLOT(parseSeries(const QStringList&)));
  QObject::connect(&parser, SIGNAL(finishParsing()), this, SLOT(finishParsing()));

  for(int i = 0; i != files.size(); ++i)
    {
    this->Implementation->startParsing(files[i]);
    parser.parse(files[i]);
    }
}

void LineChartAdapter::onSavePDF(const QStringList& files)
{
  for(int i = 0; i != files.size(); ++i)
    {
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(files[i]);
    
    this->Implementation->Chart.printChart(printer);
    }
}

void LineChartAdapter::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}

void LineChartAdapter::startParsing()
{
  this->Implementation->startParsing();
}

void LineChartAdapter::parseSeries(const QStringList& plot)
{
  this->Implementation->parseSeries(plot);
}

void LineChartAdapter::finishParsing()
{
  this->Implementation->finishParsing();
}
