/*=========================================================================

   Program: ParaView
   Module:    pqSelectionManager.h

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

========================================================================*/
#ifndef pqSelectionManager_h
#define pqSelectionManager_h

#include "pqComponentsModule.h"

#include "vtkBoundingBox.h"
#include "vtkType.h"
#include <QObject>
#include <QPair>

class pqOutputPort;
class pqPipelineSource;
class pqRenderView;
class pqSelectionManagerImplementation;
class pqServerManagerModelItem;
class pqView;
class vtkDataObject;
class vtkSelection;
class vtkSMClientDeliveryRepresentationProxy;
class vtkSMProxy;
class vtkSMSession;
class vtkSMSourceProxy;

/**
* pqSelectionManager is the nexus for introspective surface selection in
* paraview.
*
* It responds to UI events to tell the servermanager to setup for making
* selections. It watches the servermanager's state to see if the selection
* parameters are changed (either from the UI or from playback) and tells
* the servermanager to perform the requested selection.
* It is also the link between the server manager level selection and the
* GUI, converting servermanager selection result datastructures into pq/Qt
* level selection datastructures so that all views can be synchronized and
* show the same selection in their own manner.
*/
class PQCOMPONENTS_EXPORT pqSelectionManager : public QObject
{
  Q_OBJECT

public:
  pqSelectionManager(QObject* parent = NULL);
  ~pqSelectionManager() override;

  /**
  * Returns the first currently selected pqOutputPort, if any.
  */
  pqOutputPort* getSelectedPort() const;

  /**
  * Return all currently selected pqOutputPort as a QSet,
  * or an empty QSet if there aren't any
  */
  const QSet<pqOutputPort*>& getSelectedPorts() const;

  /**
  * Return true if there is at least one currently selected pqOutputPort
  * false otherwise
  */
  bool hasActiveSelection() const;

  /**
   * Returns the bounding box for all the selected data.
   */
  vtkBoundingBox selectedDataBounds() const;

signals:
  /**
  * Fired when the selection changes. Argument is the pqOutputPort (if any)
  * that was selected. If selection was cleared then the argument is NULL.
  */
  void selectionChanged(pqOutputPort*);

public slots:
  /**
  * Clear selection on a pqOutputPort.
  * Calling the method without arguments or with null
  * will clear all selection
  */
  void clearSelection(pqOutputPort* outputPort = NULL);

  /**
  * Used to keep track of active render module
  */
  void setActiveView(pqView*);

  /**
  * Updates the selected port.
  */
  void select(pqOutputPort*);

  /**
   * Set up signal/slot from the pipeline source to the selection manager.
   */
  void onSourceAdded(pqPipelineSource*);

  /**
   * Disconnect signal/slot from the pipeline source to the selection manager.
   */
  void onSourceRemoved(pqPipelineSource*);

  /**
   * Expand/contract selection to include additional layers.
   */
  void expandSelection(int layers);

private slots:
  /**
  * Called when pqLinkModel creates a link,
  * to update the selection
  */
  void onLinkAdded(int linkType);

  /**
  * Called when pqLinkModel removes a link,
  * to update the selection
  */
  void onLinkRemoved();

  /**
  * Called when server manager item is being deleted.
  */
  void onItemRemoved(pqServerManagerModelItem* item);

protected:
  void onSelect(pqOutputPort*, bool forceGlobalIds);

private:
  pqSelectionManagerImplementation* Implementation;

  // helpers
  void selectOnSurface(int screenRectange[4]);
};
#endif
