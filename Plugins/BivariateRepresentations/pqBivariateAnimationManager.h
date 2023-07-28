// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqBivariateAnimationManager
 * @brief   Manage animation of the vtkBivariateNoiseRepresentation.
 *
 * This class observes every representation of pqRenderView instances when
 * a rendering pass ends. If a representation of type Bivariate Noise Surface
 * is found, then render() is triggered to ensure the next update of the simulation.
 *
 * @sa vtkBivariateNoiseRepresentation pqStreamLinesAnimationManager
 */

#ifndef pqBivariateAnimationManager_h
#define pqBivariateAnimationManager_h

#include <QObject>

#include <set>

class pqView;

class pqBivariateAnimationManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqBivariateAnimationManager(QObject* p = nullptr);
  ~pqBivariateAnimationManager() override;

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
  Q_DISABLE_COPY(pqBivariateAnimationManager)
};

#endif
