/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqObjectHistogramWidget_h
#define _pqObjectHistogramWidget_h

#include "pqVariableType.h"
#include "QtWidgetsExport.h"
#include <QWidget>

class pqServer;
class vtkCommand;
class vtkObject;
class vtkSMProxy;

/// Displays a histogram based on data from a single proxy
class QTWIDGETS_EXPORT pqObjectHistogramWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqObjectHistogramWidget(QWidget* parent);
  ~pqObjectHistogramWidget();

public slots:
  /// Call this to set the current server
  void setServer(pqServer*);
  /// Call this to set the proxy that will become the data source
  void setProxy(vtkSMProxy*);
  /// Call this to set the current variable type and variable name
  void setVariable(pqVariableType type, const QString& name);
  /// Call this to set the current bin count (defaults to 10)
  void setBinCount(unsigned long Count);

  /// Prompts the user to save the chart to a PDF file
  void onSavePDF();
  /// Saves the chart to one-to-many PDF files
  void onSavePDF(const QStringList& files);
  
private slots:
  void onInputChanged(vtkObject*,unsigned long, void*, void*, vtkCommand*);
  void onBinCountChanged(int);

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
