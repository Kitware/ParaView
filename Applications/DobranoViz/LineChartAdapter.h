/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _LineChartAdapter_h
#define _LineChartAdapter_h

#include <pqVariableType.h>
#include <QObject>

class pqChartWidget;
class pqServer;
class vtkCommand;
class vtkObject;
class vtkSMProxy;
class vtkUnstructuredGrid;

/// Displays two sets of line plots - one set is extracted from an Exodus reader, the other set is imported from a local CSV file
class LineChartAdapter :
  public QObject
{
  Q_OBJECT
  
public:
  LineChartAdapter(pqChartWidget& chart);
  ~LineChartAdapter();

signals:
  /// Signal emitted when the set of experimental data changes - sends the list of experimental data labels
  void experimentalDataChanged(const QStringList&);
  /// Signal emitted when the choice of visible data changes
  void visibleDataChanged(const QString&);

public slots:
  /// Call this to set the current server
  void setServer(pqServer*);
  /// Call this to set the proxy that will become the data source
  void setExodusProxy(vtkSMProxy*);
  /// Call this to set the current variable to be displayed from the Exodus data
  void setExodusVariable(pqVariableType, const QString&);

  /// Clears the set of Exodus elements to be displayed
  void clearExodusElements();
  /// Adds to the set of Exodus elements to be displayed
  void addExodusElements(vtkUnstructuredGrid*);
  /// Overrides the set of Exodus elements to be displayed
  void setExodusElements(vtkUnstructuredGrid*);

  /// Sets the number of samples to extract from the CSV data
  void setSamples(int);
  /// Sets the width of error bars, as a percentage of the distance between samples
  void setErrorBarWidth(double);
  /// Enables / disables plotting of experimental / simulation data pairs
  void showData(bool);
  /// Enables / disables plotting of experimental / simulation difference plots
  void showDifferences(bool);

  /// Clears experimental data
  void clearExperimentalData();
  /// Load experimental data
  void loadExperimentalData(const QStringList&);
  
  /// Clears experimental uncertainty data
  void clearExperimentalUncertainty();
  /// Load experimental uncertainty data
  void loadExperimentalUncertainty(const QStringList&);
  
  /// Clears simulation uncertainty data
  void clearSimulationUncertainty();
  /// Load simulation uncertainty data
  void loadSimulationUncertainty(const QStringList&);
  
  /// Clears experiment / simulation mapping
  void clearExperimentSimulationMap();
  /// Load experiment / simulation mapping
  void loadExperimentSimulationMap(const QStringList&);
  
  /// Loads a "setup" file, which contains experimental, experimental uncertainty, simulation uncertainty, and experiment / simulation mapping data
  void loadSetup(const QStringList&);
  
  /// Sets the experimental data that should be visible (using its label)
  void setVisibleData(const QString&);
  
private slots:
  /// Called when the Exodus data changes
  void onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*);

  // Called during parsing of experimental data
  void loadExperimentalData(const QString& path);
  void startParsingExperimentalData();
  void parseExperimentalData(const QStringList&);
  void finishParsingExperimentalData();
  
  // Called during parsing of experimental uncertainty data
  void loadExperimentalUncertainty(const QString& path);
  void startParsingExperimentalUncertainty();
  void parseExperimentalUncertainty(const QStringList&);
  void finishParsingExperimentalUncertainty();

  // Called during parsing of simulation uncertainty data
  void loadSimulationUncertainty(const QString& path);
  void startParsingSimulationUncertainty();
  void parseSimulationUncertainty(const QStringList&);
  void finishParsingSimulationUncertainty();

  // Called during parsing of the experiment / simulation mapping
  void loadExperimentSimulationMap(const QString& path);
  void startParsingExperimentSimulationMap();
  void parseExperimentSimulationMap(const QStringList&);
  void finishParsingExperimentSimulationMap();

  // Called to emit the experimentalDataChanged signal
  void emitExperimentalDataChanged();

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
