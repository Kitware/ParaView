// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
  pqSelectionManager(QObject* parent = nullptr);
  ~pqSelectionManager() override;

  /**
   * Returns the first currently selected pqOutputPort, if any.
   */
  pqOutputPort* getSelectedPort() const;

  /**
   * Return all currently selected pqOutputPort as a QSet, or an empty QSet if
   * there aren't any
   */
  const QSet<pqOutputPort*>& getSelectedPorts() const;

  /**
   * Return true if there is at least one currently selected pqOutputPort false
   * otherwise
   */
  bool hasActiveSelection() const;

  /**
   * Returns the bounding box for all the selected data.
   */
  vtkBoundingBox selectedDataBounds() const;

Q_SIGNALS:
  /**
   * Fired when the selection changes. Argument is the pqOutputPort (if any)
   * that was selected. If selection was cleared then the argument is nullptr.
   */
  void selectionChanged(pqOutputPort*);

public Q_SLOTS:
  /**
   * Clear selection on a pqOutputPort.
   * Calling the method without arguments or with nullptr will clear all
   * selection
   */
  void clearSelection(pqOutputPort* outputPort = nullptr);

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
   * Expand/contract the selection to include additional layers.
   * This cannot shrink the initial selection seed.
   * Set layers to control the number of layers to expand/contract from current selection
   * Set removeSeed to remove the initial selection seed
   * Set removeIntermediateLayers to remove intermediate expand layers
   */
  void expandSelection(int layers, bool removeSeed = false, bool removeIntermediateLayers = false);

private Q_SLOTS:
  /**
   * Called when pqLinkModel creates a link, to update the selection
   */
  void onLinkAdded(int linkType);

  /**
   * Called when pqLinkModel removes a link, to update the selection
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
