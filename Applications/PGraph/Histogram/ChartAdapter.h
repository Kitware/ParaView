/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _ChartAdapter_h
#define _ChartAdapter_h

#include <QObject>

class pqHistogramWidget;
class pqServer;
class vtkCommand;
class vtkObject;
class vtkSMProxy;

/// Displays two sets of line plots - one set is extracted from an Exodus reader, the other set is imported from a local CSV file
class ChartAdapter :
  public QObject
{
  Q_OBJECT
  
public:
  ChartAdapter(pqHistogramWidget& chart);
  ~ChartAdapter();

public slots:
  /// Called when the user selects a new source
  void setSource(vtkSMProxy*);

private slots:
  /// Called when the input data changes
  void onInputChanged(vtkObject*, unsigned long, void*, void*, vtkCommand*);

private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
