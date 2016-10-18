// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pqSierraPlotToolsManager.h

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

#ifndef pqSierraPlotToolsManager_h
#define pqSierraPlotToolsManager_h

#include <QObject>

class QAction;

class pqPipelineSource;
class pqPlotter;
class pqServer;
class pqView;
class pqPlotVariablesDialog;

class QListWidgetItem;

class vtkSMProxy;

/// This singleton class manages the state associated with the packaged
/// visualizations provided
class pqSierraPlotToolsManager : public QObject
{
  Q_OBJECT;

public:
  static pqSierraPlotToolsManager* instance();

  ~pqSierraPlotToolsManager();

  /// Get the action for the respective operation.
  QAction* actionDataLoadManager();
  QAction* actionPlotVars();
  QAction* actionSolidMesh();
  QAction* actionWireframeSolidMesh();
  QAction* actionWireframeAndBackMesh();
  QAction* actionPlotOverTime();
  QAction* actionToggleBackgroundBW();
  QAction* actionPlotDEBUG();
  QAction* actionHoverGlobalVarVsTimeLabel();

  /// Convenience function for getting the current server.
  pqServer* getActiveServer();

  /// Convenience function for getting the main window.
  QWidget* getMainWindow();

  /// Get the window used for viewing the mesh.
  pqView* getMeshView();

  /// Get the window used for viewing plots.
  pqView* getPlotView();

  /// Get the reader objects.  Returns NULL if that reader was never created.
  pqPipelineSource* getMeshReader();
  pqPipelineSource* getParticlesReader();

  /// Get plotting object.  Returns NULL if that object was never created.
  pqPipelineSource* getPlotFilter();

  /// Get plotting object.  Returns NULL if that object was never created.
  pqPipelineSource* getGlobalVariablesPlotOverTimeFilter();

  /// Get plotting object.  Returns NULL if that object was never created.
  pqPipelineSource* getSelectionPlotOverTimeFilter();

  /// Convenience function for destroying a pipeline object and all of its
  /// consumers.
  static void destroyPipelineSourceAndConsumers(pqPipelineSource* source);

signals:
  void createdPlotGUI();
  void createPlot();

public slots:
  void showDataLoadManager();
  void checkActionEnabled();
  void showSolidMesh();
  void showWireframeSolidMesh();
  void showWireframeAndBackMesh();
  void toggleBackgroundBW();
  void actOnPlotSelection();
  void slotVariableDeselectionByName(QString varStr);
  void slotVariableSelectionByName(QString varStr);
  void slotPlotDialogAccepted();
  void slotUseParaViewGUIToSelectNodesCheck();

protected:
  // Creates a plot over time.
  //  returns true if plot successfuly created, false otherwise;
  virtual bool createPlotOverTime();

  /// Finds a pipeline source with the given SM XML name.  If there is more than
  /// one, the first is returned.
  virtual pqPipelineSource* findPipelineSource(const char* SMName);

  /// Finds a view appropriate for the data of the source and port given,
  /// constrained to those views with the given type.
  virtual pqView* findView(pqPipelineSource* source, int port, const QString& viewType);

  virtual bool setupGUIForVars();

  virtual void setupPlotMenu();
  virtual void showPlotGUI(pqPlotVariablesDialog*);

private:
  pqSierraPlotToolsManager(QObject* p);

  class pqInternal;
  pqInternal* Internal;

  Q_DISABLE_COPY(pqSierraPlotToolsManager)
};

#endif // pqSierraPlotToolsManager_h
