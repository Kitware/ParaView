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
 * If the LiveSource is an emulated time algorithm (with the emulated_time
 * XML attribute), the pqLiveSourceManager will call the update function
 * with a synchronized "real" time among all live sources.
 *
 * @sa pqLiveSourceManager pqLiveSourceBehavior vtkEmulatedTimeAlgorithm
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
   * If the LiveSource is a EmulatedTimeAlgorithm send time as a parameter.
   */
  void update(double time);

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
   * If the LiveSource is an EmulatedTimeAlgorithm, return the first and last
   * proxy timestamp, else return nullptr.
   */
  double* getTimestampRange();

  /**
   * Return true is the LiveSource XML tag has a `emulated_time` attribute to true.
   */
  bool isEmulatedTimeAlgorithm();

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
   * Triggered when the proxy information are updated.
   * (Its time range could have been updated as well)
   */
  void onInformationUpdated();

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

  /**
   * Update timestamp range information if the LiveSource is an EmulatedTimeAlgorithm.
   * Emit a onInformationUpdated() signal.
   */
  void onUpdateInformation();

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
