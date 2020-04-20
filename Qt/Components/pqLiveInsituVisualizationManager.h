/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
