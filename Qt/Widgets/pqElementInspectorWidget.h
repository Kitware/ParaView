/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqElementInspectorWidget_h
#define _pqElementInspectorWidget_h

#include "QtWidgetsExport.h"
#include <QWidget>

class vtkUnstructuredGrid;

/// Displays a collection of data set elements in spreadsheet form
class QTWIDGETS_EXPORT pqElementInspectorWidget :
  public QWidget
{
  Q_OBJECT
  
public:
  pqElementInspectorWidget(QWidget* parent);
  ~pqElementInspectorWidget();

signals:
  void elementsChanged(vtkUnstructuredGrid*);

public slots:
  /// Call this to clear the collection of elements
  void clearElements();
  /// Call this to set the collection of elements to be displayed
  void setElements(vtkUnstructuredGrid* Elements);
  
private:
  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif
