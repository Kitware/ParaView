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

#include "QtWidgetsExport.h"
#include <QWidget>

class pqServer;
class vtkSMSourceProxy;

/// Displays a histogram based on data from a single proxy
class QTWIDGETS_EXPORT pqObjectHistogramWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqObjectHistogramWidget(QWidget* parent);
  ~pqObjectHistogramWidget();

public slots:
  /// Call this whenever the connected server changes
  /** \todo This may need to change when we start supporting multiple server connections */
  void onServerChanged(pqServer* server);
  /// Call this to set the proxy that will become the data source
  void onSetProxy(vtkSMSourceProxy* proxy);
  /// Call this whenever the proxy output data is modified
  void onDisplayData();

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
