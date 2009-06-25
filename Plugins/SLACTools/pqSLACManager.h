// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSLACManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#ifndef __pqSLACManager_h
#define __pqSLACManager_h

#include <QObject>

class QAction;

class pqServer;
class pqView;

/// This singleton class manages the state associated with the packaged
/// visualizations provided by the SLAC tools.
class pqSLACManager : public QObject
{
  Q_OBJECT;
public:
  static pqSLACManager *instance();

  ~pqSLACManager();

  /// Get the action for the respective operation.
  QAction *actionDataLoadManager();

  /// Convenience function for getting the current server.
  pqServer *activeServer();

  /// Convenience function for getting the main window.
  QWidget *mainWindow();

  /// Get the window to use for 3D rendering.
  pqView *view3D();

public slots:
  void showDataLoadManager();

private:
  pqSLACManager(QObject *p);

  class pqInternal;
  pqInternal *Internal;

  pqSLACManager(const pqSLACManager &);         // Not implemented
  void operator=(const pqSLACManager &);        // Not implemented
};

#endif //__pqSLACManager_h
