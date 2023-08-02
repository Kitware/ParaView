// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPipelineContextMenuBehavior_h
#define pqPipelineContextMenuBehavior_h

#include "pqApplicationComponentsModule.h"
#include "vtkType.h"
#include <QList> // needed for QList.
#include <QObject>
#include <QPoint> // needed for QPoint.
#include <QPointer>

class pqContextMenuInterface;
class pqDataRepresentation;
class pqPipelineRepresentation;
class pqView;
class QAction;
class QMenu;

/**
 * @ingroup Behaviors
 *
 * This behavior manages showing up of a context menu with sensible pipeline
 * related actions for changing color/visibility etc. when the user
 * right-clicks on top of an object in the 3D view. Currently, it only supports
 * views with proxies that vtkSMRenderViewProxy subclasses.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPipelineContextMenuBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqPipelineContextMenuBehavior(QObject* parent = nullptr);
  ~pqPipelineContextMenuBehavior() override;

protected Q_SLOTS:

  /**
   * Called when a new view is added. We add actions to the widget for context
   * menu if the view is a render-view.
   */
  void onViewAdded(pqView*);

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * called to build the context menu for the given representation. If the
   * picked representation was a composite data set the block index of the
   * selected block will be passed in blockIndex.
   *
   * With introduction of vtkPartitionedDataSetCollection and
   * vtkPartitionedDataSet, flatIndex is no longer consistent across ranks and
   * hence rank is also returned. Unless dealing with these data types, rank can
   * be ignored.
   */
  virtual void buildMenu(pqDataRepresentation* repr, unsigned int blockIndex, int rank);

  /**
   * event filter to capture the right-click. We don't directly use mechanisms
   * from QWidget to popup the context menu since all of those mechanism seem
   * to eat away the right button release, leaving the render window in a
   * dragging state.
   */
  bool eventFilter(QObject* caller, QEvent* e) override;

  QMenu* Menu;
  QPoint Position;

private:
  Q_DISABLE_COPY(pqPipelineContextMenuBehavior)
};

#endif
