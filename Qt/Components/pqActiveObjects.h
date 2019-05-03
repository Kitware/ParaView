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
#ifndef pqActiveObjects_h
#define pqActiveObjects_h

#include "pqComponentsModule.h"
#include <QObject>
#include <QPointer>

/**
* needed for inline get-methods.
*/
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqProxySelection.h"
#include "pqServer.h"
#include "pqView.h"

class vtkEventQtSlotConnect;
class vtkSMProxySelectionModel;
class vtkSMSessionProxyManager;
class vtkSMViewLayoutProxy;

/**
* pqActiveObjects is a singleton that keeps track of "active objects"
* including active view, active source, active representation etc.
* pqActiveObjects also keeps track of selected sources (known as 'selection').
* setActiveSource/setActivePort will affect the selection but not vice-versa
* (unless dealing with multiple server sessions).
*/
class PQCOMPONENTS_EXPORT pqActiveObjects : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
  * Provides access to the singleton.
  */
  static pqActiveObjects& instance();

  /**
  * Returns the active view.
  */
  pqView* activeView() const { return this->ActiveView; }

  /**
  * Returns the active source
  */
  pqPipelineSource* activeSource() const { return this->ActiveSource; }

  /**
  * Returns the active port.
  */
  pqOutputPort* activePort() const { return this->ActivePort; }

  /**
  * Returns the active server.
  */
  pqServer* activeServer() const { return this->ActiveServer; }

  /**
  * Returns the active representation.
  */
  pqDataRepresentation* activeRepresentation() const { return this->ActiveRepresentation; }

  vtkSMProxySelectionModel* activeSourcesSelectionModel() const
  {
    return this->activeServer() ? this->activeServer()->activeSourcesSelectionModel() : NULL;
  }

  /**
  * Returns the current source selection.
  */
  const pqProxySelection& selection() const { return this->Selection; }

  /**
  * Returns the proxyManager() from the active server, if any.
  * Equivalent to calling this->activeServer()->proxyManager();
  */
  vtkSMSessionProxyManager* proxyManager() const;

  /**
   * Convenience method to get the layout for the active view.
   */
  vtkSMViewLayoutProxy* activeLayout() const;

  /**
   * Convenience method to locate the active vtkSMViewLayoutProxy and the active
   * cell in it. Since currently this information is not available at the
   * server-manager level, especially if the active view is nullptr, we use
   * pqMultiViewWidget and pqTabbedMultiViewWidget to find this.
   *
   * Returns 0 if one could not determined.
   */
  int activeLayoutLocation() const;

public slots:
  void setActiveView(pqView* view);
  void setActiveSource(pqPipelineSource* source);
  void setActivePort(pqOutputPort* port);
  void setActiveServer(pqServer*);
  void onActiveServerChanged();

  /**
  * Sets the selected set of proxies. All proxies in the selection must be on
  * the same server/session. This generally doesn't affect the activeSource
  * etc. unless the server is different from the active server. In which case,
  * the active server is changed before the selection is updated.
  */
  void setSelection(const pqProxySelection& selection, pqServerManagerModelItem* current);

signals:
  /**
  * These signals are fired when any of the corresponding active items change.
  */
  void serverChanged(pqServer*);
  void viewChanged(pqView* view);
  void sourceChanged(pqPipelineSource*);
  void portChanged(pqOutputPort*);
  void representationChanged(pqDataRepresentation*);
  void representationChanged(pqRepresentation*);
  void selectionChanged(const pqProxySelection&);

  /**
  * this signal is fired when the active source fires the dataUpdated()
  * signal. This is used by components in the GUI that need to be updated when
  * the active source's pipeline updates.
  */
  void dataUpdated();

private slots:
  /**
  * if a new server connection was established, and no active server is set,
  * this makes the new server active by default. This helps with single-session
  * clients.
  */
  void serverAdded(pqServer*);

  /**
  * if the active server connection was closed, this ensures that the
  * application is notified.
  */
  void serverRemoved(pqServer*);

  /**
  * if any of the active proxies is removed, we fire appropriate signals.
  */
  void proxyRemoved(pqServerManagerModelItem*);

  /**
  * called to update representation
  */
  void updateRepresentation();

  void sourceSelectionChanged();
  void viewSelectionChanged();

protected:
  pqActiveObjects();
  ~pqActiveObjects() override;

  /**
  * single method that fires appropriate signals based on state changes. This
  * also ensures that the Cached* variables are updated correctly.
  */
  void triggerSignals();

private:
  Q_DISABLE_COPY(pqActiveObjects)

  /**
  * method used to reset all active items.
  */
  void resetActives();

  QPointer<pqServer> ActiveServer;
  QPointer<pqPipelineSource> ActiveSource;
  QPointer<pqOutputPort> ActivePort;
  QPointer<pqView> ActiveView;
  QPointer<pqDataRepresentation> ActiveRepresentation;
  pqProxySelection Selection;

  // these are void* maintained to detect when values have changed.
  void* CachedServer;
  void* CachedSource;
  void* CachedPort;
  void* CachedView;
  void* CachedRepresentation;
  pqProxySelection CachedSelection;

  vtkEventQtSlotConnect* VTKConnector;
};

#endif
