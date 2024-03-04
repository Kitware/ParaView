// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLiveSourceManager_h
#define pqLiveSourceManager_h

#include <QObject>

#include "pqComponentsModule.h" // for exports

class pqPipelineSource;
class pqLiveSourceItem;
class vtkSMProxy;

/**
 * pqLiveSourceManager is the manager that handle all live sources in ParaView
 * It is usually instantiated by the pqLiveSourceBehavior.
 * It provide feature to control live sources.
 *
 * @sa pqLiveSourceBehavior pqLiveSourceItem
 */
class PQCOMPONENTS_EXPORT pqLiveSourceManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqLiveSourceManager(QObject* parent = nullptr);
  ~pqLiveSourceManager() override;

  ///@{
  /**
   * Pause / Resume live updates for all live sources.
   */
  void pause();
  void resume();
  ///@}

  /**
   * Returns true if any live source updates is paused.
   */
  bool isPaused();

  /**
   * Returns the pqLiveSourceItem corresponding to the proxy
   */
  pqLiveSourceItem* getLiveSourceItem(vtkSMProxy*);

private Q_SLOTS:
  ///@{
  /**
   * Theses slots handle sources addition/removal.
   */
  void onSourceAdded(pqPipelineSource*);
  void onSourceRemove(pqPipelineSource*);
  ///@}

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqLiveSourceManager);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
