/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _LineChart_h
#define _LineChart_h

#include "pqVariableType.h"
#include <QWidget>

class pqServer;
class vtkCommand;
class vtkObject;
class vtkSMProxy;
class vtkUnstructuredGrid;

/// Displays two sets of line plots - one set is extracted from an Exodus reader, the other set is imported from a local CSV file
class LineChart :
  public QWidget
{
  Q_OBJECT
  
public:
  LineChart(QWidget* parent);
  ~LineChart();

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
  void addExodusElements(vtkUnstructuredGrid* Elements);
  /// Overrides the set of Exodus elements to be displayed
  void setExodusElements(vtkUnstructuredGrid* Elements);

  /// Clears the set of CSV data to be displayed
  void clearCSV();
  /// Adds to the set of CSV data to be displayed
  void addCSV(const QStringList& plot);
  
  /// Prompts the user to choose CSV data to be displayed
  void onLoadCSV();
  /// Displays a set of CSV files
  void onLoadCSV(const QStringList& files);
  
  /// Prompts the user to save the chart to a PDF file
  void onSavePDF();
  /// Saves the chart to one-to-many PDF files
  void onSavePDF(const QStringList& files);
  
private slots:
  /// Called when the Exodus data changes
  void onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*);

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
