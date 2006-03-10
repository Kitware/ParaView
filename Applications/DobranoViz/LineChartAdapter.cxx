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
#include <pqWaitCursor.h>

#include <QDir>
#include <QDomDocument>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkExecutive.h>
#include <vtkEventQtSlotConnect.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkPointSet.h>
#include <vtkSMInputProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMCompoundProxy.h>
#include <vtkSphereSource.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnstructuredGrid.h>

#include <vtkExodusReader.h>
#include <vtkProcessModule.h>

#include <vtkstd/algorithm>
#include <vtkstd/vector>

//////////////////////////////////////////////////////////////////////////////
// LineChartAdapter::pqImplementation

struct LineChartAdapter::pqImplementation
{
  struct ExperimentalDataT
  {
    QString Label;
    QVector<double> Times;
    QVector<double> Values;
  };
  
  struct ExperimentalUncertaintyT
  {
    QString Label;
    QVector<double> Times;
    QVector<double> UpperBounds;
    QVector<double> LowerBounds;
  };
  
  struct SimulationUncertaintyT
  {
    QString Label;
    QVector<double> Times;
    QVector<double> UpperBounds;
    QVector<double> LowerBounds;
  };

  pqImplementation(pqLineChartWidget& chart) :
    SourceProxy(0),
    EventAdaptor(vtkEventQtSlotConnect::New()),
    ExodusVariableType(VARIABLE_TYPE_CELL),
    Chart(chart),
    Samples(50),
    ErrorBarWidth(0.5),
    ShowData(true),
    ShowDifferences(false)
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
  }
  
  void addExodusElements(vtkUnstructuredGrid* elements)
  {
    internalAddExodusElements(elements);
  }

  void clearExperimentalData()
  {
    this->ExperimentalData.clear();
  }
  
  void clearExperimentalUncertainty()
  {
    this->ExperimentalUncertainty.clear();
  }
  
  void clearSimulationUncertainty()
  {
    this->SimulationUncertainty.clear();
  }
  
  void clearExperimentSimulationMap()
  {
    this->ExperimentSimulationMap.clear();
  }

  void startParsingExperimentalData()
  {
    this->CSVSeries.clear();
  }
  
  void parseExperimentalData(const QStringList& series)
  {
    this->CSVSeries.push_back(series);
  }
  
  void finishParsingExperimentalData()
  {
    if(this->CSVSeries.size() < 2)
      return;

    // The first column in the file will be time
    QStringList& time = this->CSVSeries[0];
    if(time.size() < 2)
      return;
    
    // Each column after the first is experimental data
    for(int i = 1; i != this->CSVSeries.size(); ++i)
      {
      QStringList& file_data = this->CSVSeries[i];
      if(file_data.size() != time.size())
        continue;

      ExperimentalDataT data;
      data.Label = file_data[0];

      this->VisibleData = file_data[0];

      for(int i = 1; i != file_data.size(); ++i)
        {
        data.Times.push_back(time[i].toDouble());
        data.Values.push_back(file_data[i].toDouble());
        }
      
      this->ExperimentalData.push_back(data);  
      }

    this->CSVSeries.clear();
  }
  
  void startParsingExperimentalUncertainty()
  {
    this->CSVSeries.clear();
  }
  
  void parseExperimentalUncertainty(const QStringList& series)
  {
    this->CSVSeries.push_back(series);
  }
  
  void finishParsingExperimentalUncertainty()
  {
    if(this->CSVSeries.size() < 3)
      return;

    // The first column in the file will be time
    QStringList& time = this->CSVSeries[0];
    if(time.size() < 2)
      return;
    
    // Each group of two columns after the first is upper and lower bounds of uncertainty
    for(int i = 1; i < this->CSVSeries.size() - 1; i += 2)
      {
      QStringList& upper_bounds = this->CSVSeries[i];
      QStringList& lower_bounds = this->CSVSeries[i + 1];
      
      if(upper_bounds.size() != time.size() || lower_bounds.size() != time.size())
        continue;
        
      if(upper_bounds[0] != lower_bounds[0])
        continue;

      ExperimentalUncertaintyT uncertainty;
      uncertainty.Label = upper_bounds[0];

      for(int i = 1; i != upper_bounds.size(); ++i)
        {
        uncertainty.Times.push_back(time[i].toDouble());
        uncertainty.UpperBounds.push_back(upper_bounds[i].toDouble());
        uncertainty.LowerBounds.push_back(lower_bounds[i].toDouble());
        }
      
      this->ExperimentalUncertainty.push_back(uncertainty);  
      }

    this->CSVSeries.clear();
  }
  
  void startParsingSimulationUncertainty()
  {
    this->CSVSeries.clear();
  }
  
  void parseSimulationUncertainty(const QStringList& series)
  {
    this->CSVSeries.push_back(series);
  }
  
  void finishParsingSimulationUncertainty()
  {
    this->CSVSeries.clear();
  }

  void startParsingExperimentSimulationMap()
  {
    this->CSVSeries.clear();
  }
  
  void parseExperimentSimulationMap(const QStringList& series)
  {
    this->CSVSeries.push_back(series);
  }
  
  void finishParsingExperimentSimulationMap()
  {
    if(this->CSVSeries.size() < 2)
      return;

    QStringList& experimental = this->CSVSeries[0];
    QStringList& simulation = this->CSVSeries[1];

    if(experimental.size() != simulation.size())
      return;

    if(experimental.size() < 2 || simulation.size() < 2)
      return;

    for(int i = 1; i < experimental.size(); ++i)    
      {
      this->ExperimentSimulationMap[experimental[i]] = simulation[i];
      }

    this->CSVSeries.clear();
  }
  
  void onInputChanged()
  {
    this->updateChart();
  }
  
  void setVisibleData(const QString& data)
  {
    this->VisibleData = data;
  }
  
  void setSamples(int samples)
  {
    this->Samples = vtkstd::max(2, samples);
  }
  
  void setErrorBarWidth(double width)
  {
    this->ErrorBarWidth = width;
  }
  
  void showData(bool state)
  {
    this->ShowData = state;
  }
  
  void showDifferences(bool state)
  {
    this->ShowDifferences = state;
  }
  
  void addExperimentalPlot(const ExperimentalDataT& data, const QPen& plot_pen, const QPen& whisker_pen)
  {
    const int sample_size = data.Values.size() > 2 * this->Samples ? data.Values.size() / this->Samples : 1;

    // Look for matching experimental uncertainty data ...
    for(int i = 0; i != this->ExperimentalUncertainty.size(); ++i)
      {
      ExperimentalUncertaintyT& uncertainty = this->ExperimentalUncertainty[i];
      
      // The labels have to match ...
      if(uncertainty.Label != data.Label)
        continue;
        
      // The size of the data must match ...
      if(uncertainty.Times.size() != data.Times.size())
        continue;
        
      // Found matching uncertainty data, so create a line plot with whiskers ...
      double time_delta = VTK_DOUBLE_MAX;
      pqLineErrorPlot::CoordinatesT coordinates;
      for(int i = 0; i < data.Times.size(); i += sample_size)
        {
        if(i != 0)
          {
          time_delta = vtkstd::min(time_delta, data.Times[i] - data.Times[i - sample_size]);
          }
          
        coordinates.push_back(
          pqLineErrorPlot::Coordinate(
            data.Times[i],
            data.Values[i],
            uncertainty.UpperBounds[i],
            uncertainty.LowerBounds[i]));
        }
    
      this->Chart.getLineChart().addData(
        new pqLineErrorPlot(
          new pqNullMarkerPen(plot_pen),
          whisker_pen,
          this->ErrorBarWidth * time_delta,
          coordinates));
      
      this->Chart.getLegend().addEntry(
        new pqNullMarkerPen(plot_pen),
        new pqChartLabel(data.Label));
          
        return;
      }
    
    // No uncertainty data, so create a line plot without whiskers ...
    pqChartCoordinateList coordinates;
    for(int i = 0; i < data.Times.size(); i += sample_size)
      {
      coordinates.pushBack(pqChartCoordinate(data.Times[i], data.Values[i]));
      }
  
    this->Chart.getLineChart().addData(
      new pqLinePlot(
        new pqCircleMarkerPen(plot_pen, QSize(3, 3), QPen(plot_pen.color()), QBrush(Qt::white)),
        coordinates));
    
    this->Chart.getLegend().addEntry(
        new pqCircleMarkerPen(plot_pen, QSize(3, 3), QPen(plot_pen.color()), QBrush(Qt::white)),
      new pqChartLabel(data.Label));
  }
  
  void addSimulationPlot(vtkExodusReader& Reader, const int ElementID, const QPen& Pen)
  {
    // Get timestep data from the reader ...
    vtkInformation* const information = Reader.GetExecutive()->GetOutputInformation(0);
    if(!information)
      return;
    const int time_step_count = information->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    QVector<double> time_steps(time_step_count);
    information->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), time_steps.data());
  
    // Get values from the reader ...
    const int id = ElementID + 1; // Exodus expects one-based cell ids
    const char* type = this->ExodusVariableType == VARIABLE_TYPE_CELL ? "CELL" : "POINT";
    vtkFloatArray* const values = vtkFloatArray::New();
    Reader.GetTimeSeriesData(id, this->ExodusVariableName.toAscii().data(), type, values); 
    
    if(time_steps.size() != values->GetNumberOfTuples())
      return;
    
    pqChartCoordinateList coordinates;
    for(vtkIdType i = 0; i != values->GetNumberOfTuples(); ++i)
      {
      const double time = time_steps[i];
      const double value = values->GetValue(i) - 273.15;
      coordinates.pushBack(pqChartCoordinate(time, value));
      }

    values->Delete();
    
    pqLinePlot* const plot = new pqLinePlot(
      new pqCircleMarkerPen(Pen, QSize(4, 4), QPen(Pen.color(), 0.5), QBrush(Qt::white)),
      coordinates);
    
    this->Chart.getLineChart().addData(plot);
    this->Chart.getLegend().addEntry(new pqCircleMarkerPen(Pen, QSize(4, 4), QPen(Pen.color(), 0.5), QBrush(Qt::white)), new pqChartLabel(QString("Element %1").arg(ElementID)));
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
 
    // Plot experimental data ...
    if(this->ShowData)
      {
      for(int i = 0; i != ExperimentalData.size(); ++i)
        {
        if(ExperimentalData[i].Label != this->VisibleData)
          continue;
          
        const double hue = static_cast<double>(i) / static_cast<double>(ExperimentalData.size());
        const QColor color = QColor::fromHsvF(hue, 1.0, 0.5);
        addExperimentalPlot(ExperimentalData[i], QPen(color, 2), QPen(color, 1));
        }
      }
    
    if(this->ExodusVariableName.isEmpty())
      return;

    if(this->ExodusElements.empty() && this->ExperimentSimulationMap.empty())
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

    // Plot the corresponding simulation data, if available ...
    if(this->ShowData)
      {
      for(int i = 0; i != ExperimentalData.size(); ++i)
        {
        if(ExperimentalData[i].Label != this->VisibleData)
          continue;
          
        const QString element = this->ExperimentSimulationMap[ExperimentalData[i].Label];
        if(element.isEmpty())
          continue;

        const double hue = static_cast<double>(i) / static_cast<double>(ExperimentalData.size());
        const QColor color = QColor::fromHsvF(hue, 1.0, 0.5);
        
        addSimulationPlot(*reader, element.toInt(), QPen(color, 2, Qt::DashLine));
        }
      }

/*    
    // Plot difference data, if available ...
    if(this->ShowDifference)
      {
      for(int i = 0; i != ExperimentalData.size(); ++i)
        {
        if(ExperimentalData[i].Label != this->VisibleData)
          continue;
          
        const QString element = this->ExperimentSimulationMap[ExperimentalData[i].Label];
        if(element.isEmpty())
          continue;

        const double hue = static_cast<double>(i) / static_cast<double>(ExperimentalData.size());
        const QColor color = QColor::fromHsvF(hue, 1.0, 0.5);
        
        addDifferencePlot(*reader, element.toInt(), QPen(color, 2, Qt::DashLine));
        }
      }
*/
    
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

    this->Chart.getYAxis().getLabel().setText(this->ExodusVariableName);

    if(this->ShowData)
      {
      unsigned long count = 0;
      for(vtkstd::vector<unsigned long>::reverse_iterator element = this->ExodusElements.rbegin(); element != this->ExodusElements.rend(); ++element, ++count)
        {
        const double hue = static_cast<double>(count) / static_cast<double>(ExodusElements.size());
        const QColor color = QColor::fromHsvF(hue, 1.0, 1.0);
        addSimulationPlot(*reader, *element, QPen(color, 1));
        }
      }
  }
  
  vtkSMProxy* SourceProxy;
  vtkEventQtSlotConnect* EventAdaptor;
  pqLineChartWidget& Chart;
  
  pqVariableType ExodusVariableType;
  QString ExodusVariableName;
  vtkstd::vector<unsigned long> ExodusElements;

  QString VisibleData;
  int Samples;
  double ErrorBarWidth;
  bool ShowData;
  bool ShowDifferences;

  /// Temporary storage for CSV data during parsing
  QVector<QStringList> CSVSeries;

  QVector<ExperimentalDataT> ExperimentalData;
  QVector<ExperimentalUncertaintyT> ExperimentalUncertainty;
  QVector<SimulationUncertaintyT> SimulationUncertainty;
  QMap<QString, QString> ExperimentSimulationMap;
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
  pqWaitCursor cursor;
  this->Implementation->setExodusVariable(type, name);
}

void LineChartAdapter::clearExodusElements()
{
  pqWaitCursor cursor;
  this->Implementation->clearExodusElements();
  this->Implementation->updateChart();
}

void LineChartAdapter::addExodusElements(vtkUnstructuredGrid* elements)
{
  pqWaitCursor cursor;
  this->Implementation->addExodusElements(elements);
  this->Implementation->updateChart();
}

void LineChartAdapter::setExodusElements(vtkUnstructuredGrid* elements)
{
  pqWaitCursor cursor;
  this->Implementation->clearExodusElements();
  this->Implementation->addExodusElements(elements);
  this->Implementation->updateChart();
}

void LineChartAdapter::setSamples(int samples)
{
  pqWaitCursor cursor;
  this->Implementation->setSamples(samples);
  this->Implementation->updateChart();
}

void LineChartAdapter::setErrorBarWidth(double width)
{
  pqWaitCursor cursor;
  this->Implementation->setErrorBarWidth(width);
  this->Implementation->updateChart();
}

void LineChartAdapter::showData(bool state)
{
  pqWaitCursor cursor;
  this->Implementation->showData(state);
  this->Implementation->updateChart();
}

void LineChartAdapter::showDifferences(bool state)
{
  pqWaitCursor cursor;
  this->Implementation->showDifferences(state);
  this->Implementation->updateChart();
}

void LineChartAdapter::clearExperimentalData()
{
  pqWaitCursor cursor;
  this->Implementation->clearExperimentalData();
  this->Implementation->updateChart();
  this->emitExperimentalDataChanged();
}

void LineChartAdapter::loadExperimentalData(const QStringList& files)
{
  pqWaitCursor cursor;
  for(int i = 0; i != files.size(); ++i)
    {
    this->loadExperimentalData(files[i]);
    }
    
  this->Implementation->updateChart();
  this->emitExperimentalDataChanged();
  emit visibleDataChanged(this->Implementation->VisibleData);
}

void LineChartAdapter::loadExperimentalData(const QString& file)
{
  pqDelimitedTextParser parser(pqDelimitedTextParser::COLUMN_SERIES, ',');
  QObject::connect(&parser, SIGNAL(startParsing()), this, SLOT(startParsingExperimentalData()));
  QObject::connect(&parser, SIGNAL(parseSeries(const QStringList&)), this, SLOT(parseExperimentalData(const QStringList&)));
  QObject::connect(&parser, SIGNAL(finishParsing()), this, SLOT(finishParsingExperimentalData()));
  parser.parse(file);
}

void LineChartAdapter::clearExperimentalUncertainty()
{
  pqWaitCursor cursor;
  this->Implementation->clearExperimentalUncertainty();
  this->Implementation->updateChart();
}

void LineChartAdapter::loadExperimentalUncertainty(const QStringList& files)
{
  pqWaitCursor cursor;
  for(int i = 0; i != files.size(); ++i)
    {
    loadExperimentalUncertainty(files[i]);
    }

  this->Implementation->updateChart();
}

void LineChartAdapter::loadExperimentalUncertainty(const QString& file)
{
  pqDelimitedTextParser parser(pqDelimitedTextParser::COLUMN_SERIES, ',');
  QObject::connect(&parser, SIGNAL(startParsing()), this, SLOT(startParsingExperimentalUncertainty()));
  QObject::connect(&parser, SIGNAL(parseSeries(const QStringList&)), this, SLOT(parseExperimentalUncertainty(const QStringList&)));
  QObject::connect(&parser, SIGNAL(finishParsing()), this, SLOT(finishParsingExperimentalUncertainty()));
  parser.parse(file);
}

void LineChartAdapter::clearSimulationUncertainty()
{
  pqWaitCursor cursor;
  this->Implementation->clearSimulationUncertainty();
  this->Implementation->updateChart();
}

void LineChartAdapter::loadSimulationUncertainty(const QStringList& files)
{
  pqWaitCursor cursor;
  for(int i = 0; i != files.size(); ++i)
    {
    this->loadSimulationUncertainty(files[i]);
    }

  this->Implementation->updateChart();
}

void LineChartAdapter::loadSimulationUncertainty(const QString& file)
{
  pqDelimitedTextParser parser(pqDelimitedTextParser::COLUMN_SERIES, ',');
  QObject::connect(&parser, SIGNAL(startParsing()), this, SLOT(startParsingSimulationUncertainty()));
  QObject::connect(&parser, SIGNAL(parseSeries(const QStringList&)), this, SLOT(parseSimulationUncertainty(const QStringList&)));
  QObject::connect(&parser, SIGNAL(finishParsing()), this, SLOT(finishParsingSimulationUncertainty()));
  parser.parse(file);
}

void LineChartAdapter::clearExperimentSimulationMap()
{
  pqWaitCursor cursor;
  this->Implementation->clearExperimentSimulationMap();
  this->Implementation->updateChart();
}

void LineChartAdapter::loadExperimentSimulationMap(const QStringList& files)
{
  pqWaitCursor cursor;
  for(int i = 0; i != files.size(); ++i)
    {
    loadExperimentSimulationMap(files[i]);
    }

  this->Implementation->updateChart();
}

void LineChartAdapter::loadExperimentSimulationMap(const QString& file)
{
  pqDelimitedTextParser parser(pqDelimitedTextParser::COLUMN_SERIES, ',');
  QObject::connect(&parser, SIGNAL(startParsing()), this, SLOT(startParsingExperimentSimulationMap()));
  QObject::connect(&parser, SIGNAL(parseSeries(const QStringList&)), this, SLOT(parseExperimentSimulationMap(const QStringList&)));
  QObject::connect(&parser, SIGNAL(finishParsing()), this, SLOT(finishParsingExperimentSimulationMap()));
  parser.parse(file);
}

void LineChartAdapter::loadSetup(const QStringList& files)
{
  pqWaitCursor cursor;
  for(int i = 0; i != files.size(); ++i)
    {
    QFile file(files[i]);
    QDomDocument xml_document;
    xml_document.setContent(&file, false);

    QDomElement xml_setup = xml_document.documentElement();
    if(xml_setup.nodeName() != "setup")
      {
      QMessageBox::warning(0, "Dobran-O-Viz Error:", files[i] + " is not a Dobran-O-Viz setup file");
      continue;
      }

    for(QDomNode xml_file = xml_setup.firstChild(); !xml_file.isNull(); xml_file = xml_file.nextSibling())
      {
      if(!xml_file.isElement())
        continue;
      
      if(xml_file.nodeName() == "experimental")
        {
        loadExperimentalData(xml_file.toElement().text());
        }
      else if(xml_file.nodeName() == "experimental_uncertainty")
        {
        loadExperimentalUncertainty(xml_file.toElement().text());
        }
      else if(xml_file.nodeName() == "simulation_uncertainty")
        {
        loadSimulationUncertainty(xml_file.toElement().text());
        }
      else if(xml_file.nodeName() == "experiment_simulation_map")
        {
        loadExperimentSimulationMap(xml_file.toElement().text());
        }
      }
    }

  this->Implementation->updateChart();
  this->emitExperimentalDataChanged();
  emit visibleDataChanged(this->Implementation->VisibleData);
}

void LineChartAdapter::setVisibleData(const QString& label)
{
  pqWaitCursor cursor;
  this->Implementation->setVisibleData(label);
  this->Implementation->updateChart();
  
  emit visibleDataChanged(this->Implementation->VisibleData);
}

void LineChartAdapter::onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*)
{
  this->Implementation->onInputChanged();
}

void LineChartAdapter::startParsingExperimentalData()
{
  this->Implementation->startParsingExperimentalData();
}

void LineChartAdapter::parseExperimentalData(const QStringList& series)
{
  this->Implementation->parseExperimentalData(series);
}

void LineChartAdapter::finishParsingExperimentalData()
{
  this->Implementation->finishParsingExperimentalData();
}

void LineChartAdapter::startParsingExperimentalUncertainty()
{
  this->Implementation->startParsingExperimentalUncertainty();
}

void LineChartAdapter::parseExperimentalUncertainty(const QStringList& series)
{
  this->Implementation->parseExperimentalUncertainty(series);
}

void LineChartAdapter::finishParsingExperimentalUncertainty()
{
  this->Implementation->finishParsingExperimentalUncertainty();
}

void LineChartAdapter::startParsingSimulationUncertainty()
{
  this->Implementation->startParsingSimulationUncertainty();
}

void LineChartAdapter::parseSimulationUncertainty(const QStringList& series)
{
  this->Implementation->parseSimulationUncertainty(series);
}

void LineChartAdapter::finishParsingSimulationUncertainty()
{
  this->Implementation->finishParsingSimulationUncertainty();
}

void LineChartAdapter::startParsingExperimentSimulationMap()
{
  this->Implementation->startParsingExperimentSimulationMap();
}

void LineChartAdapter::parseExperimentSimulationMap(const QStringList& series)
{
  this->Implementation->parseExperimentSimulationMap(series);
}

void LineChartAdapter::finishParsingExperimentSimulationMap()
{
  this->Implementation->finishParsingExperimentSimulationMap();
}

void LineChartAdapter::emitExperimentalDataChanged()
{
  QStringList data;
  for(int i = 0; i != this->Implementation->ExperimentalData.size(); ++i)
    {
    data.push_back(this->Implementation->ExperimentalData[i].Label);
    }
    
  emit experimentalDataChanged(data);
}
