// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqStreamLinesAnimationManager
 * @brief   Autoload class that enable representation streamlines animation
 *
 * This class observes every representation of pqRenderView instances when
 * a rendering pass ends. If a representation of type StreamLines if found
 * then render() is triggered to ensure the next update of the simulation.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Bastien Jacquet, Kitware 2017
 * This work was supported by Total SA.
 */

#ifndef pqStreamLinesAnimationManager_h
#define pqStreamLinesAnimationManager_h

#include <QObject>

#include <set>

class pqView;

class pqStreamLinesAnimationManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqStreamLinesAnimationManager(QObject* p = nullptr);
  ~pqStreamLinesAnimationManager() override;

  void onShutdown() {}
  void onStartup() {}

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void onViewAdded(pqView*);
  void onViewRemoved(pqView*);

protected Q_SLOTS:
  void onRenderEnded();

protected: // NOLINT(readability-redundant-access-specifiers)
  std::set<pqView*> Views;

private:
  Q_DISABLE_COPY(pqStreamLinesAnimationManager)
};

#endif
