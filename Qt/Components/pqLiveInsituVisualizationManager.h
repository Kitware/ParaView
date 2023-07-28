// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLiveInsituVisualizationManager_h
#define pqLiveInsituVisualizationManager_h

#include "pqComponentsModule.h"
#include <QObject>

class pqOutputPort;
class pqPipelineSource;
class pqServer;
class vtkEventQtSlotConnect;
class vtkSMLiveInsituLinkProxy;

/**
 * Manages the live-coprocessing link. When
 * pqLiveInsituVisualizationManager is instantiated, it creates a new
 * dummy session that represents the catalyst pipeline. The proxy
 * manager in this session reflects the state of the proxies on the
 * coprocessing side.  Next, it creates the (coprocessing,
 * LiveInsituLink) proxy that sets up the server socket to accept
 * connections from catalyst.
 * @ingroup LiveInsitu
 */
class PQCOMPONENTS_EXPORT pqLiveInsituVisualizationManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqLiveInsituVisualizationManager(int connection_port, pqServer* server);
  ~pqLiveInsituVisualizationManager() override;

  /**
   * returns true of the port is extracted over to the visualization server.
   */
  bool hasExtracts(pqOutputPort*) const;

  pqServer* insituSession() const;
  pqServer* displaySession() const;

  /**
   * create an extract to view the output from the given port. pqOutputPort
   * must be an instance on the dummy session corresponding to the catalyst
   * pipeline.
   */
  bool addExtract(pqOutputPort*);

  vtkSMLiveInsituLinkProxy* getProxy() const;

  /**
   * Convenience method to return the displaySession for a catalystSession. If
   * the argument is not a catalystSession, it will simply return the same
   * session without any errors.
   */
  static pqServer* displaySession(pqServer* catalystSession);

Q_SIGNALS:
  void insituConnected();
  void insituDisconnected();
  void nextTimestepAvailable();

protected Q_SLOTS:
  void timestepsUpdated();
  void sourceRemoved(pqPipelineSource*);

private:
  Q_DISABLE_COPY(pqLiveInsituVisualizationManager)

  class pqInternals;
  pqInternals* Internals;
};

#endif
