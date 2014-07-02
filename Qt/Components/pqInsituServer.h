/*=========================================================================

   Program: ParaView
   Module:    pqInsituServer.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef __pqInsituServer_h
#define __pqInsituServer_h


#include <QPointer>
#include <QMap>
#include "pqComponentsModule.h"

#include "vtkType.h"

class pqLiveInsituVisualizationManager;
class pqPipelineSource;
class pqProxy;
class pqServer;
class vtkSMLiveInsituLinkProxy;


/// Singleton that provides access to Catalyst objects (pqServer,
/// pqLiveInsituVisualizationManager, vtkSMLiveInsituLinkProxy).
class PQCOMPONENTS_EXPORT pqInsituServer : public QObject
{
  Q_OBJECT

public:
  static double INVALID_TIME;
  static vtkIdType INVALID_TIME_STEP;
  static pqInsituServer* instance();

  /// Returns the link proxy to Catalyst or NULL if not connected or if not
  /// a catalyst server
  static vtkSMLiveInsituLinkProxy* linkProxy(pqServer* insituServer);
  vtkSMLiveInsituLinkProxy* linkProxy()
  {
    return pqInsituServer::linkProxy(this->insituServer());
  }
  /// Is this the insitu server
  static bool isInsituServer(pqServer* server);
  static pqPipelineSource* pipelineSource(pqServer* insituServer);
  static void time(pqPipelineSource* source, double* time,
                     vtkIdType* timeStep);

signals:
  void catalystConnected(pqServer* displayServer);
  void timeUpdated();
  void breakpointAdded(pqServer* insituServer);
  void breakpointRemoved(pqServer* insituServer);
  void breakpointHit(pqServer* insituServer);

public:
  /// Returns current Catalyst server. The current Catalyst server
  /// is either selected or its displayServer is selected. If no server is
  /// selected we choose the first Catalyst server we find.
  ///  We return NULL if the client is not connected to Catalyst.
  pqServer* insituServer();
  /// Is this the server where Catalyst displays its extracts
  bool isDisplayServer(pqServer* server);
  /// Returns the catalyst manager associated with 
  /// 'displayServer' or 'insituServer'
  pqLiveInsituVisualizationManager* managerFromDisplay(pqServer* displayServer);
  static pqLiveInsituVisualizationManager* managerFromInsitu(
    pqServer* insituServer);
  /// Creates the manager and accept connections from Catalyst
  pqLiveInsituVisualizationManager* connect(pqServer* displayServer);

  double breakpointTime() const
  {
    return this->BreakpointTime;
  }
  double breakpointTimeStep() const
  {
    return this->BreakpointTimeStep;
  }
  void setBreakpoint(double time);
  void setBreakpoint(vtkIdType timeStep);
  void removeBreakpoint();
  bool hasBreakpoint() const
  {
    return this->breakpointTime() != INVALID_TIME ||
      this->breakpointTimeStep() != INVALID_TIME_STEP;
  }

  double time() const
  {
    return this->Time;
  }
  vtkIdType timeStep() const
  {
    return this->TimeStep;
  }

protected slots:
  /// called when Catalyst disconnects. We clean up the Catalyst connection.
  void onCatalystDisconnected();
  void onBreakpointHit(pqServer* insituServer);
  void onSourceAdded(pqPipelineSource* source);
  void onDataUpdated(pqPipelineSource* source);

protected:
  pqInsituServer();
  bool isTimeBreakpointHit() const;
  bool isTimeStepBreakpointHit() const;

protected:
  double BreakpointTime;
  double Time;
  vtkIdType BreakpointTimeStep;
  vtkIdType TimeStep;

private:
  Q_DISABLE_COPY(pqInsituServer)

  typedef QMap<void*, QPointer<pqLiveInsituVisualizationManager> > ManagersType;
  ManagersType Managers;
};

#endif
