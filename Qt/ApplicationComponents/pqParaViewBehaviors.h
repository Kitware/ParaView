/*=========================================================================

   Program: ParaView
   Module:    pqParaViewBehaviors.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqParaViewBehaviors_h
#define pqParaViewBehaviors_h

#include "pqApplicationComponentsModule.h"

#include "vtkSetGet.h" // for VTK_LEGACY.

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
* using static methods of the form pqParaViewBehaviors::set<behavior name>(bool)
* e.g. pqParaViewBehaviors::setStandardPropertyWidgets(false).
*
* Since ParaView 5.1, ObjectPickingBehavior is disabled by default in
* ParaView.
*
* As of ParaView 5.4, QtMessageHandlerBehavior is deprecated. It should
* no longer be needed. Applications should consider using pqOutputWidget
* to capture VTK and Qt messages. QtMessageHandlerBehavior is needed if using
* deprecated pqOutputWindow.
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
  PQ_BEHAVIOR_DEFINE_METHODS(ObjectPickingBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(DefaultViewBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(UndoRedoBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(AlwaysConnectedBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CrashRecoveryBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(AutoLoadPluginXMLBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginDockWidgetsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(VerifyRequiredPluginBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginActionGroupBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CommandLineOptionsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PersistentMainWindowStateBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(CollaborationBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(ViewStreamingBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(PluginSettingsBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(ApplyBehavior);
  PQ_BEHAVIOR_DEFINE_METHODS(QuickLaunchShortcuts);
  PQ_BEHAVIOR_DEFINE_METHODS(LockPanelsBehavior);

#if !defined(VTK_LEGACY_REMOVE)
  PQ_BEHAVIOR_DEFINE_METHODS_LEGACY(QtMessageHandlerBehavior);
#endif

  pqParaViewBehaviors(QMainWindow* window, QObject* parent = NULL);
  virtual ~pqParaViewBehaviors();

private:
  Q_DISABLE_COPY(pqParaViewBehaviors)

  PQ_BEHAVIOR_DECLARE_FLAG(StandardPropertyWidgets);
  PQ_BEHAVIOR_DECLARE_FLAG(StandardViewFrameActions);
  PQ_BEHAVIOR_DECLARE_FLAG(StandardRecentlyUsedResourceLoader);
  PQ_BEHAVIOR_DECLARE_FLAG(DataTimeStepBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(SpreadSheetVisibilityBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PipelineContextMenuBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(ObjectPickingBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(DefaultViewBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(UndoRedoBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(AlwaysConnectedBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CrashRecoveryBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(AutoLoadPluginXMLBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginDockWidgetsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(VerifyRequiredPluginBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginActionGroupBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CommandLineOptionsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PersistentMainWindowStateBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(CollaborationBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(ViewStreamingBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(PluginSettingsBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(ApplyBehavior);
  PQ_BEHAVIOR_DECLARE_FLAG(QuickLaunchShortcuts);
  PQ_BEHAVIOR_DECLARE_FLAG(LockPanelsBehavior);

#if !defined(VTK_LEGACY_REMOVE)
  PQ_BEHAVIOR_DECLARE_FLAG(QtMessageHandlerBehavior);
#endif
};

#undef PQ_BEHAVIOR_DECLARE_FLAG
#undef PQ_BEHAVIOR_DEFINE_METHODS

#endif
