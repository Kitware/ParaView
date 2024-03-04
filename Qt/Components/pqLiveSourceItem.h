// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLiveSourceItem_h
#define pqLiveSourceItem_h

#include "pqComponentsModule.h"
#include <QObject>

#include "vtkType.h" // for vtkTypeUInt32.

class pqPipelineSource;
class pqView;
class vtkPVXMLElement;

/**
 * @class pqLiveSourceItem
 *
 * This component is used by pqLiveSourceManager to interact with
 * the underlying proxy of the LiveSource.
 *
 * Each pqLiveSourceItem have a pqTimer that periodically emit
 * the `refreshSource` signal, which is monitored by pqLiveSourceManager.
 * Upon receiving this signal, it triggers the update function, which
 * in turn invokes GetNeedsUpdate on the proxy.
 *
 * @sa pqLiveSourceManager pqLiveSourceBehavior
 */
class PQCOMPONENTS_EXPORT pqLiveSourceItem : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqLiveSourceItem(pqPipelineSource* src, vtkPVXMLElement* liveHints, QObject* parent = nullptr);
  ~pqLiveSourceItem() override;

  /**
   * Calls `GetNeedsUpdate` on the associated vtkAlgorithm proxy.
   */
  void update();

  ///@{
  /**
   * Pause / Resume live updates for this source.
   */
  void pause();
  void resume();
  ///@}

  /**
   * Returns true if is currently paused.
   */
  bool isPaused();

  /**
   * Get underlying proxy global ID.
   */
  vtkTypeUInt32 getSourceId();

Q_SIGNALS:
  /**
   * Triggered when the internal refresh timer timeout.
   */
  void refreshSource();

  /**
   * Triggered when the live source is paused / resumed.
   */
  void stateChanged(bool isPaused);

private Q_SLOTS:
  void onViewAdded(pqView*);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqLiveSourceItem)

  ///@{
  /**
   * Pause / resume live source when user interact with the view.
   */
  void startInteractionEvent();
  void endInteractionEvent();
  ///@}

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
