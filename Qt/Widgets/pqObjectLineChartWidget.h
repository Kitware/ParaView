/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqObjectLineChartWidget_h
#define _pqObjectLineChartWidget_h

#include "pqVariableType.h"
#include "QtWidgetsExport.h"
#include <QWidget>

class pqServer;
class vtkCommand;
class vtkObject;
class vtkSMProxy;
class vtkUnstructuredGrid;

/// Displays a histogram based on data from a single proxy
class QTWIDGETS_EXPORT pqObjectLineChartWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqObjectLineChartWidget(QWidget* parent);
  ~pqObjectLineChartWidget();

public slots:
  /// Call this to set the current server
  void setServer(pqServer*);
  /// Call this to set the proxy that will become the data source
  void setProxy(vtkSMProxy*);
  /// Call this to set the current variable
  void setVariable(pqVariableType, const QString&);
  /// Call this to clear the set of elements
  void clear();
  /// Call this to add a collection of element IDs to the set of elements
  void addElements(vtkUnstructuredGrid* Elements);
  /// Call this to set the collection of element IDs
  void setElements(vtkUnstructuredGrid* Elements);

private slots:
  void onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*);

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
