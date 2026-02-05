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

#include "vtkZSpaceSDKManager.h"

#include <QAction>
#include <QObject>

#include <set>

class pqView;
class pqPipelineSource;
class vtkCommand;
class vtkObject;

class pqZSpaceManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqZSpaceManager(QObject* p = nullptr);
  ~pqZSpaceManager() override = default;

  /**
   * Called when the plugin is loaded. Currently it does not do anything.
   */
  void onStartup() {}

  /**
   * Called when the application shuts down. Currently calls `Shutdown` on the
   * `vtkZSpaceSDKManager`.
   */
  void onShutdown();

private Q_SLOTS:
  /**
   * Called when a view is added, before the call to onViewAdded.
   * Internally fills the `ZSPaceViews` set if the given is a zSpace view.
   */
  void onPreViewAdded(pqView*);

  /**
   * Called after onPreViewAdded signal. Setup the event callback for stylus custom events.
   */
  void onViewAdded(pqView*);

  /**
   * Called to remove the view from the `ZSpaceViews` set if the given view is a zSpace view.
   */
  void onViewRemoved(pqView*);

  /**
   * Called when full screen option is toggled On or Off.
   */
  void onActiveFullScreenEnabled(bool);

  /**
   * Called from the added views when their corresponding render are ended.
   */
  void onRenderEnded();

private: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Event callback from the `vtkZSpaceSDKManager` class.
   */
  void onEvent(vtkObject* caller, unsigned long event, void* data);

  /**
   * Find the macro action specified by the settings.
   * If there there is no macro found, it returns a nullptr.
   */
  QAction* findMacro(vtkZSpaceSDKManager::ButtonIds buttonId);

  std::set<pqView*> ZSpaceViews;
  vtkCommand* Observer;

  Q_DISABLE_COPY(pqZSpaceManager)
};

#endif
