/*=========================================================================

   Program: ParaView
   Module:    pqActiveObjects.h

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
#ifndef __pqActiveObjects_h 
#define __pqActiveObjects_h

#include <QObject>
#include <QPointer>
#include "pqComponentsExport.h"

/// needed for inline get-methods.
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqView.h"

class vtkEventQtSlotConnect;
class vtkSMProxySelectionModel;

/// pqActiveObjects is a singleton that keeps track of "active objects"
/// including active view, active source, active representation etc. 
class PQCOMPONENTS_EXPORT pqActiveObjects : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  /// Provides access to the singleton.
  static pqActiveObjects& instance();

  /// Returns the active view.
  pqView* activeView() const { return this->ActiveView; }

  /// Returns the active source
  pqPipelineSource* activeSource() const { return this->ActiveSource; }

  /// Returns the active port.
  pqOutputPort* activePort() const { return this->ActivePort; }

  /// Returns the active server.
  pqServer* activeServer() const { return this->ActiveServer; }

  /// Returns the active representation.
  pqDataRepresentation* activeRepresentation() const
    { return this->ActiveRepresentation; }

  vtkSMProxySelectionModel* activeSourcesSelectionModel() const
    {
    return this->activeServer()?
      this->activeServer()->activeSourcesSelectionModel() : NULL;
    }

public slots:
  void setActiveView(pqView * view);
  void setActiveSource(pqPipelineSource * source);
  void setActivePort(pqOutputPort * port);
  void setActiveServer(pqServer*);

signals:
  /// These signals are fired when any of the corresponding active items change.
  void serverChanged(pqServer*);
  void viewChanged(pqView* view);
  void sourceChanged(pqPipelineSource*);
  void portChanged(pqOutputPort*);
  void representationChanged(pqDataRepresentation*);
  void representationChanged(pqRepresentation*);

  /// signals fired as a consequence of serverChanged() to make it easier to
  /// components that depend on selection models.
  void sourcesSelectionModelChanged(vtkSMProxySelectionModel*);

private slots:
  /// if a new server connection was established, and no active server is set,
  /// this makes the new server active by default. This helps with single-session
  /// clients.
  void serverAdded(pqServer*);

  /// if the active server connection was closed, this ensures that the
  /// application is notified.
  void serverRemoved(pqServer*);

  /// if any of the active proxies is removed, we fire appropriate signals.
  void proxyRemoved(pqServerManagerModelItem*);

  /// called to update representation
  void updateRepresentation();

  void sourceSelectionChanged();
  void viewSelectionChanged();
 
protected:
  pqActiveObjects();
  ~pqActiveObjects();

  /// single method that fires appropriate signals based on state changes. This
  /// also ensures that the Cached* variables are updated correctly.
  void triggerSignals();

private:
  Q_DISABLE_COPY(pqActiveObjects);

  QPointer<pqServer> ActiveServer;
  QPointer<pqPipelineSource> ActiveSource;
  QPointer<pqOutputPort> ActivePort;
  QPointer<pqView> ActiveView;
  QPointer<pqDataRepresentation> ActiveRepresentation;

  // these are void* maintained to detect when values have changed.
  void* CachedServer;
  void* CachedSource;
  void* CachedPort;
  void* CachedView;
  void* CachedRepresentation;

  vtkEventQtSlotConnect* VTKConnector;
};

#endif


