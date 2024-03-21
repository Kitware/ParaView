// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   pqZSpaceManager
 * @brief   Autoload class that enable input independent update of the ZSpace render views.
 *
 * This class observes every views of type vtkZSpaceView. When a render is called on one
 * of these views, an other render is manually triggered to ensure constant update
 * of the zSpace render view.
 * @par Thanks:
 * Kitware SAS
 * This work was supported by EDF.
 */

#ifndef pqZSpaceManager_h
#define pqZSpaceManager_h

#include <QObject>

#include <set>

class pqView;
class pqPipelineSource;

class pqZSpaceManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqZSpaceManager(QObject* p = nullptr);
  ~pqZSpaceManager() override = default;

  void onShutdown() {}
  void onStartup() {}

public Q_SLOTS:
  void onViewAdded(pqView*);
  void onViewRemoved(pqView*);
  void onActiveFullScreenEnabled(bool);

protected Q_SLOTS:
  void onRenderEnded();

protected:
  std::set<pqView*> ZSpaceViews;

private:
  Q_DISABLE_COPY(pqZSpaceManager)
};

#endif
