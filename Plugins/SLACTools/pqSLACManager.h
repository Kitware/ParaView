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

class pqPipelineSource;
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
  QAction *actionShowEField();
  QAction *actionShowBField();
  QAction *actionShowParticles();
  QAction *actionSolidMesh();
  QAction *actionWireframeSolidMesh();
  QAction *actionWireframeAndBackMesh();
  QAction *actionPlotOverZ();
  QAction *actionToggleBackgroundBW();
  QAction *actionShowStandardViewpoint();

  /// Convenience function for getting the current server.
  pqServer *getActiveServer();

  /// Convenience function for getting the main window.
  QWidget *getMainWindow();

  /// Get the window used for viewing the mesh.
  pqView *getMeshView();

  /// Get the window used for viewing plots.
  pqView *getPlotView();

  /// Get the reader objects.  Returns NULL if that reader was never created.
  pqPipelineSource *getMeshReader();
  pqPipelineSource *getParticlesReader();

  /// Get plotting object.  Returns NULL if that object was never created.
  pqPipelineSource *getPlotFilter();

  /// Convenience function for destroying a pipeline object and all of its
  /// consumers.
  static void destroyPipelineSourceAndConsumers(pqPipelineSource *source);

public slots:
  void showDataLoadManager();
  void checkActionEnabled();
  void showField(const char *name);
  void showEField();
  void showBField();
  void showParticles(bool show);
  void showSolidMesh();
  void showWireframeSolidMesh();
  void showWireframeAndBackMesh();
  void createPlotOverZ();
  void toggleBackgroundBW();
  void showStandardViewpoint();

protected:
  /// Finds a pipeline source with the given SM XML name.  If there is more than
  /// one, the first is returned.
  virtual pqPipelineSource *findPipelineSource(const char *SMName);

  /// Finds a view appropriate for the data of the source and port given,
  /// constrained to those views with the given type.
  virtual pqView *findView(pqPipelineSource *source, int port,
                           const QString &viewType);

  /// Updates the plot view (if it is created) with the field shown in the
  /// mesh view.
  virtual void updatePlotField();

private:
  pqSLACManager(QObject *p);

  class pqInternal;
  pqInternal *Internal;

  pqSLACManager(const pqSLACManager &);         // Not implemented
  void operator=(const pqSLACManager &);        // Not implemented
};

#endif //__pqSLACManager_h
