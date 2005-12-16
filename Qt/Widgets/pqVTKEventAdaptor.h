/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqVTKEventAdaptor_h
#define _pqVTKEventAdaptor_h

#include "QtWidgetsExport.h"
#include <QObject>
#include <vtkCommand.h>

/// Helper class that converts a VTK event into a Qt signal
class QTWIDGETS_EXPORT pqVTKEventAdaptor :
  public QObject,
  public vtkCommand
{
  Q_OBJECT
  
public:
  /// Called by the VTK event system to handle a VTK event
  void Execute(vtkObject*, unsigned long, void*);
  
signals:
  /// Emitted whenever a VTK event is received
  void vtkEvent();
  /// Emitted whenever a VTK event is received, passing all of the event arguments
  void vtkEvent(vtkObject*, unsigned long, void*);
};

#endif
