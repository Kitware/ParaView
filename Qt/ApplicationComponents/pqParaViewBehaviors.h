// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqParaViewBehaviors_h
#define pqParaViewBehaviors_h

#include "pqApplicationComponentsModule.h"

#include "vtkParaViewDeprecation.h" // for deprecation
#include "vtkSetGet.h"              // for VTK_LEGACY.

#include <QFlags>
#include <QObject>

class QMainWindow;

/**
 * @defgroup Behaviors ParaView Behaviors
 * Behaviors are classes that manage certain behaviors in the application.
 * Developers should simply instantiate behaviors if the expect that
 * behavior in their client.
 */

/**
 * @class pqParaViewBehaviors
 * @brief creates all standard ParaView behaviours
 * @ingroup Behaviors
 *
 * pqParaViewBehaviors creates all the behaviors used by ParaView. If your
 * client is merely a branded version of ParaView, then you may want to simply
 * use this behavior. You can also enable/disable behaviors created by
 * pqParaViewBehaviors before instantiating the pqParaViewBehaviors instance by
 * using static methods of the form pqParaViewBehaviors::setEnable<behavior name>(bool)
 * e.g. pqParaViewBehaviors::setEnableStandardPropertyWidgets(false).
 *
 * Since ParaView 5.1, ObjectPickingBehavior is disabled by default in
 * ParaView.
 *
 */

#define PQ_BEHAVIOR_DEFINE_METHODS(_name)                                                          \
  static void setEnable##_name(bool val) { pqParaViewBehaviors::_name = val; }                     \
  static bool enable##_name() { return pqParaViewBehaviors::_name; }

#define PQ_BEHAVIOR_DEFINE_METHODS_LEGACY(_name)                                                   \
  VTK_LEGACY(static void setEnable##_name(bool val) { pqParaViewBehaviors::_name = val; });        \
  VTK_LEGACY(static bool enable##_name() { return pqParaViewBehaviors::_name; });

#define PQ_BEHAVIOR_DECLARE_FLAG(_name) static bool _name;

class PQAPPLICATIONCOMPONENTS_EXPORT pqParaViewBehaviors : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  PQ_BEHAVIOR_DEFINE_METHODS(StandardPropertyWidgets);
  PQ_BEHAVIOR_DEFINE_METHODS(StandardViewFrameActions);
  PQ_BEHAVIOR_DEFINE_METHODS(StandardRecentlyUsedResourceLoader);
  PQ_BEHAVIOR_DEFINE_METHODS(DataTimeStepBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(SpreadSheetVisibilityBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PipelineContextMenuBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(BlockContentMenu);
  PQ_BEHAVIOR_DEFINE_METHODS(ObjectPickingBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(DefaultViewBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(UndoRedoBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(AlwaysConnectedBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CrashRecoveryBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(AutoLoadPluginXMLBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginDockWidgetsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(VerifyRequiredPluginBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginActionGroupBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginToolBarBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CommandLineOptionsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PersistentMainWindowStateBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CollaborationBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(ViewStreamingBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginSettingsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(ApplyBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(QuickLaunchShortcuts);
  PQ_BEHAVIOR_DEFINE_METHODS(LockPanelsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PythonShellResetBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CustomShortcutBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(MainWindowEventBehavior);

  PARAVIEW_DEPRECATED_IN_5_12_0("Use AddExamplesInFileDialogBehavior instead")
  PQ_BEHAVIOR_DEFINE_METHODS(AddExamplesInFavoritesBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(AddExamplesInFileDialogBehavior);

  ///@{
  /**
   * Controls whether `pqUsageLoggingBehavior` is created. Disabled by default
   * (except in the ParaView application itself).
   * @sa pqUsageLoggingBehavior.
   */
  PQ_BEHAVIOR_DEFINE_METHODS(UsageLoggingBehavior);
  ///@}

  ///@{
  /**
   * Controls whether `pqLiveSourceBehavior` is created.
   * @sa pqLiveSourceBehavior
   */
  PQ_BEHAVIOR_DEFINE_METHODS(LiveSourceBehavior);
  ///@}

  ///@{
  /**
   * By default, widgets like QComboBox, QSlider handle wheel event even when
   * the widget doesn't have the focus. While that's handy, in many use-cases,
   * in several where these are embedded in scrollable panels, they can
   * interrupt the scrolling of the panel. Hence, this behavior has been added
   * since ParaView 5.5 default. When enabled, this is enabled for QComboBox,
   * QSlider, QAbstractSpinBox and subclasses.
   */
  PQ_BEHAVIOR_DEFINE_METHODS(WheelNeedsFocusBehavior);
  ///@}

  pqParaViewBehaviors(QMainWindow* window, QObject* parent = nullptr);
  ~pqParaViewBehaviors() override;

private:
  Q_DISABLE_COPY(pqParaViewBehaviors)

  PQ_BEHAVIOR_DECLARE_FLAG(StandardPropertyWidgets);
  PQ_BEHAVIOR_DECLARE_FLAG(StandardViewFrameActions);
  PQ_BEHAVIOR_DECLARE_FLAG(StandardRecentlyUsedResourceLoader);
  PQ_BEHAVIOR_DECLARE_FLAG(DataTimeStepBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(SpreadSheetVisibilityBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PipelineContextMenuBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(BlockContentMenu);
  PQ_BEHAVIOR_DECLARE_FLAG(ObjectPickingBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(DefaultViewBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(UndoRedoBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(AlwaysConnectedBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CrashRecoveryBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(AutoLoadPluginXMLBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginDockWidgetsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(VerifyRequiredPluginBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginActionGroupBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginToolBarBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CommandLineOptionsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PersistentMainWindowStateBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CollaborationBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(ViewStreamingBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginSettingsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(ApplyBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(QuickLaunchShortcuts);
  PQ_BEHAVIOR_DECLARE_FLAG(LockPanelsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PythonShellResetBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(WheelNeedsFocusBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(LiveSourceBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CustomShortcutBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(MainWindowEventBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(UsageLoggingBehavior);
  // PARAVIEW_DEPRECATED_IN_5_12_0
  PQ_BEHAVIOR_DECLARE_FLAG(AddExamplesInFavoritesBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(AddExamplesInFileDialogBehavior);
};

#undef PQ_BEHAVIOR_DECLARE_FLAG
#undef PQ_BEHAVIOR_DEFINE_METHODS

#endif
