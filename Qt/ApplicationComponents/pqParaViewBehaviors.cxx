/*=========================================================================

   Program: ParaView
   Module:    pqParaViewBehaviors.cxx

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
#include "pqParaViewBehaviors.h"

#include "pqAlwaysConnectedBehavior.h"
#include "pqApplicationCore.h"
#include "pqApplyBehavior.h"
#include "pqAutoLoadPluginXMLBehavior.h"
#include "pqCollaborationBehavior.h"
#include "pqCommandLineOptionsBehavior.h"
#include "pqCoreTestUtility.h"
#include "pqCrashRecoveryBehavior.h"
#include "pqCustomShortcutBehavior.h"
#include "pqDataTimeStepBehavior.h"
#include "pqDefaultViewBehavior.h"
#include "pqInterfaceTracker.h"
#include "pqLiveSourceBehavior.h"
#include "pqLockPanelsBehavior.h"
#include "pqMainWindowEventBehavior.h"
#include "pqObjectPickingBehavior.h"
#include "pqPersistentMainWindowStateBehavior.h"
#include "pqPipelineContextMenuBehavior.h"
#include "pqPluginActionGroupBehavior.h"
#include "pqPluginDockWidgetsBehavior.h"
#include "pqPluginSettingsBehavior.h"
#include "pqPluginToolBarBehavior.h"
#include "pqPropertiesPanel.h"
#include "pqServerManagerModel.h"
#include "pqSpreadSheetVisibilityBehavior.h"
#include "pqStandardPropertyWidgetInterface.h"
#include "pqStandardRecentlyUsedResourceLoaderImplementation.h"
#include "pqStandardViewFrameActionsImplementation.h"
#include "pqStreamingTestingEventPlayer.h"
#include "pqUndoRedoBehavior.h"
#include "pqUndoStack.h"
#include "pqUsageLoggingBehavior.h"
#include "pqVerifyRequiredPluginBehavior.h"
#include "pqViewStreamingBehavior.h"

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPythonShell.h"
#endif

#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QMainWindow>
#include <QShortcut>
#include <QSlider>

#include <cassert>

namespace
{
class WheelFilter : public QObject
{
public:
  WheelFilter(QObject* obj)
    : QObject(obj)
  {
  }
  ~WheelFilter() {}
  bool eventFilter(QObject* obj, QEvent* evt) override
  {
    assert(obj && evt);
    if (obj->isWidgetType()) // shortcut to avoid doing work when not a widget.
    {
      QWidget* wdg = reinterpret_cast<QWidget*>(obj);
      if (evt->type() == QEvent::Wheel)
      {
        if (qobject_cast<QComboBox*>(obj) != nullptr || qobject_cast<QSlider*>(obj) != nullptr ||
          qobject_cast<QAbstractSpinBox*>(obj) != nullptr)
        {
          if (!wdg->hasFocus())
          {
            return true;
          }
        }
      }
      else if (evt->type() == QEvent::Show)
      {
        // we need to change focus policy to StrongFocus so that these widgets
        // don't get focus and subsequently, wheel events on mouse-wheel
        // unless the widget has focus. To avoid having to go through all
        // instances of combo-box & slider creations to change focus policy,
        // we use an event filter to do that.
        if (wdg->focusPolicy() == Qt::WheelFocus)
        {
          if (qobject_cast<QComboBox*>(obj) != nullptr || qobject_cast<QSlider*>(obj) != nullptr ||
            qobject_cast<QAbstractSpinBox*>(obj) != nullptr)
          {
            wdg->setFocusPolicy(Qt::StrongFocus);
          }
        }
      }
    }
    return QObject::eventFilter(obj, evt);
  }
};
}

#define PQ_BEHAVIOR_DEFINE_FLAG(_name, _default) bool pqParaViewBehaviors::_name = _default;
PQ_BEHAVIOR_DEFINE_FLAG(StandardPropertyWidgets, true);
PQ_BEHAVIOR_DEFINE_FLAG(StandardViewFrameActions, true);
PQ_BEHAVIOR_DEFINE_FLAG(StandardRecentlyUsedResourceLoader, true);
PQ_BEHAVIOR_DEFINE_FLAG(DataTimeStepBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(SpreadSheetVisibilityBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PipelineContextMenuBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(ObjectPickingBehavior, false);
PQ_BEHAVIOR_DEFINE_FLAG(DefaultViewBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(UndoRedoBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(AlwaysConnectedBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(CrashRecoveryBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(AutoLoadPluginXMLBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PluginDockWidgetsBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(VerifyRequiredPluginBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PluginActionGroupBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PluginToolBarBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(CommandLineOptionsBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PersistentMainWindowStateBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(CollaborationBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(ViewStreamingBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PluginSettingsBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(ApplyBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(QuickLaunchShortcuts, true);
PQ_BEHAVIOR_DEFINE_FLAG(LockPanelsBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(PythonShellResetBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(WheelNeedsFocusBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(LiveSourceBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(CustomShortcutBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(MainWindowEventBehavior, true);
PQ_BEHAVIOR_DEFINE_FLAG(UsageLoggingBehavior, false);
#undef PQ_BEHAVIOR_DEFINE_FLAG

#define PQ_IS_BEHAVIOR_ENABLED(_name) enable##_name()

//-----------------------------------------------------------------------------
pqParaViewBehaviors::pqParaViewBehaviors(QMainWindow* mainWindow, QObject* parentObject)
  : Superclass(parentObject)
{
  // Register ParaView interfaces.
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();

  if (PQ_IS_BEHAVIOR_ENABLED(StandardPropertyWidgets))
  {
    // Register standard types of property widgets.
    pgm->addInterface(new pqStandardPropertyWidgetInterface(pgm));
  }

  if (PQ_IS_BEHAVIOR_ENABLED(StandardViewFrameActions))
  {
    // Register standard types of view-frame actions.
    pgm->addInterface(new pqStandardViewFrameActionsImplementation(pgm));
  }

  if (PQ_IS_BEHAVIOR_ENABLED(StandardRecentlyUsedResourceLoader))
  {
    // Register standard recent file menu handlers.
    pgm->addInterface(new pqStandardRecentlyUsedResourceLoaderImplementation(pgm));
  }

  // Define application behaviors.
  if (PQ_IS_BEHAVIOR_ENABLED(DataTimeStepBehavior))
  {
    new pqDataTimeStepBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(LiveSourceBehavior))
  {
    new pqLiveSourceBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(SpreadSheetVisibilityBehavior))
  {
    new pqSpreadSheetVisibilityBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(PipelineContextMenuBehavior))
  {
    new pqPipelineContextMenuBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(ObjectPickingBehavior))
  {
    new pqObjectPickingBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(DefaultViewBehavior))
  {
    new pqDefaultViewBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(UndoRedoBehavior))
  {
    new pqUndoRedoBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(AlwaysConnectedBehavior))
  {
    new pqAlwaysConnectedBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(CrashRecoveryBehavior))
  {
    new pqCrashRecoveryBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(AutoLoadPluginXMLBehavior))
  {
    new pqAutoLoadPluginXMLBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(PluginDockWidgetsBehavior))
  {
    new pqPluginDockWidgetsBehavior(mainWindow);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(VerifyRequiredPluginBehavior))
  {
    new pqVerifyRequiredPluginBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(PluginActionGroupBehavior))
  {
    new pqPluginActionGroupBehavior(mainWindow);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(PluginToolBarBehavior))
  {
    new pqPluginToolBarBehavior(mainWindow);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(CommandLineOptionsBehavior))
  {
    new pqCommandLineOptionsBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(PersistentMainWindowStateBehavior))
  {
    new pqPersistentMainWindowStateBehavior(mainWindow);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(CollaborationBehavior))
  {
    new pqCollaborationBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(ViewStreamingBehavior))
  {
    // some special handling for pqStreamingTestingEventPlayer
    pqViewStreamingBehavior* vsbehv = new pqViewStreamingBehavior(this);
    pqWidgetEventPlayer* player =
      pqApplicationCore::instance()->testUtility()->eventPlayer()->getWidgetEventPlayer(
        "pqStreamingTestingEventPlayer");
    pqStreamingTestingEventPlayer* splayer = NULL;
    if (!player)
    {
      splayer = new pqStreamingTestingEventPlayer(NULL);
      // the testUtility takes ownership of the player.
      pqApplicationCore::instance()->testUtility()->eventPlayer()->addWidgetEventPlayer(splayer);
    }
    else
    {
      splayer = qobject_cast<pqStreamingTestingEventPlayer*>(player);
    }
    if (splayer)
    {
      splayer->setViewStreamingBehavior(vsbehv);
    }
  }
  if (PQ_IS_BEHAVIOR_ENABLED(PluginSettingsBehavior))
  {
    new pqPluginSettingsBehavior(this);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(ApplyBehavior))
  {
    pqApplyBehavior* applyBehavior = new pqApplyBehavior(this);
    foreach (pqPropertiesPanel* ppanel, mainWindow->findChildren<pqPropertiesPanel*>())
    {
      applyBehavior->registerPanel(ppanel);
    }
  }

  if (PQ_IS_BEHAVIOR_ENABLED(QuickLaunchShortcuts))
  {
    // Setup quick-launch shortcuts.
    QShortcut* ctrlSpace = new QShortcut(Qt::CTRL + Qt::Key_Space, mainWindow);
    QObject::connect(
      ctrlSpace, SIGNAL(activated()), pqApplicationCore::instance(), SLOT(quickLaunch()));
    QShortcut* ctrlShiftSpace = new QShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Space, mainWindow);
    QObject::connect(
      ctrlShiftSpace, SIGNAL(activated()), pqApplicationCore::instance(), SLOT(quickLaunch()));
    QShortcut* altSpace = new QShortcut(Qt::ALT + Qt::Key_Space, mainWindow);
    QObject::connect(
      altSpace, SIGNAL(activated()), pqApplicationCore::instance(), SLOT(quickLaunch()));
  }

  if (PQ_IS_BEHAVIOR_ENABLED(LockPanelsBehavior))
  {
    new pqLockPanelsBehavior(mainWindow);
  }

#if VTK_MODULE_ENABLE_ParaView_pqPython
  if (PQ_IS_BEHAVIOR_ENABLED(PythonShellResetBehavior))
  {
    pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
    for (pqPythonShell* ashell : mainWindow->findChildren<pqPythonShell*>())
    {
      ashell->connect(smmodel, SIGNAL(aboutToRemoveServer(pqServer*)), SLOT(reset()));
    }
  }
#endif

  if (PQ_IS_BEHAVIOR_ENABLED(WheelNeedsFocusBehavior))
  {
    auto afilter = new WheelFilter(mainWindow);
    qApp->installEventFilter(afilter);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(CustomShortcutBehavior))
  {
    new pqCustomShortcutBehavior(mainWindow);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(MainWindowEventBehavior))
  {
    new pqMainWindowEventBehavior(mainWindow);
  }
  if (PQ_IS_BEHAVIOR_ENABLED(UsageLoggingBehavior))
  {
    new pqUsageLoggingBehavior(mainWindow);
  }
  CLEAR_UNDO_STACK();
}

//-----------------------------------------------------------------------------
pqParaViewBehaviors::~pqParaViewBehaviors()
{
}

#undef PQ_IS_BEHAVIOR_ENABLED
