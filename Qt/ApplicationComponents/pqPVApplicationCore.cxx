// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqPVApplicationCore.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqApplicationComponentsInit.h"
#include "pqApplicationCore.h"
#include "pqComponentsTestUtility.h"
#include "pqCoreConfiguration.h"
#include "pqCoreUtilities.h"
#include "pqItemViewSearchWidget.h"
#include "pqLiveSourceManager.h"
#include "pqLoadDataReaction.h"
#include "pqPresetGroupsManager.h"
#include "pqQuickLaunchDialogExtended.h"
#include "pqSelectionManager.h"
#include "pqSpreadSheetViewModel.h"
#include "vtkProcessModule.h"

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqPythonManager.h"
#endif

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFileOpenEvent>
#include <QList>
#include <QPointer>

//-----------------------------------------------------------------------------
pqPVApplicationCore::pqPVApplicationCore(int& argc, char** argv, vtkCLIOptions* options,
  bool addStandardArgs /*=true*/, QObject* parentObject /*=nullptr*/)
  : Superclass(argc, argv, options, addStandardArgs, parentObject)
{
  // Initialize pqComponents resources, only for static builds.
#if !BUILD_SHARED_LIBS
  pqApplicationComponentsInit();
#endif
  this->AnimationManager = new pqAnimationManager(this);
  this->SelectionManager = new pqSelectionManager(this);
  this->LiveSourceManager = nullptr;

  pqApplicationCore::instance()->registerManager("SELECTION_MANAGER", this->SelectionManager);

  QPointer<pqPresetGroupsManager> presetGroupManager = new pqPresetGroupsManager(this);
  const bool loadedFromSettings = presetGroupManager->loadGroupsFromSettings();
  // If the groups could not be loaded from the settings, use the default groups
  if (!loadedFromSettings)
  {
    presetGroupManager->loadGroups(pqPresetGroupsManager::getPresetGroupsJson());
  }
  pqApplicationCore::instance()->registerManager("PRESET_GROUP_MANAGER", presetGroupManager);

  this->PythonManager = nullptr;
#if VTK_MODULE_ENABLE_ParaView_pqPython
  this->PythonManager = new pqPythonManager(this);

// Ensure that whenever Python is initialized, we tell paraview.servermanager
// that is being done from the GUI.
#endif

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)),
    this->AnimationManager, SLOT(onActiveServerChanged(pqServer*)));
}

//-----------------------------------------------------------------------------
pqPVApplicationCore::~pqPVApplicationCore()
{
  delete this->AnimationManager;
  delete this->SelectionManager;
  delete this->LiveSourceManager;
#if VTK_MODULE_ENABLE_ParaView_pqPython
  delete this->PythonManager;
#endif
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::registerForQuicklaunch(QWidget* menu)
{
  if (menu)
  {
    this->QuickLaunchMenus.push_back(menu);
  }
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::quickLaunch()
{
  Q_EMIT this->aboutToShowQuickLaunch();
  if (!this->QuickLaunchMenus.empty())
  {
    QList<QAction*> searchableActions;
    for (QWidget* menu : this->QuickLaunchMenus)
    {
      if (menu)
      {
        // don't use QMenu::actions() since that includes only the top-level
        // actions.
        // --> BUT pqProxyGroupMenuManager in order to handle multi-server
        //         setting properly add actions into an internal widget so
        //         actions() should be used instead of findChildren()
        if (menu->findChildren<QAction*>().empty())
        {
          searchableActions << menu->actions();
        }
        else
        {
          searchableActions << menu->findChildren<QAction*>();
        }
      }
    }

    pqQuickLaunchDialogExtended dialog(pqCoreUtilities::mainWidget(), searchableActions);
    QObject::connect(&dialog, &pqQuickLaunchDialogExtended::applyRequested, this,
      &pqPVApplicationCore::triggerApply);
    dialog.exec();
  }
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::applyPipeline()
{
  Q_EMIT this->triggerApply();
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::startSearch()
{
  if (!QApplication::focusWidget())
  {
    return;
  }
  QAbstractItemView* focusItemView = qobject_cast<QAbstractItemView*>(QApplication::focusWidget());
  if (!focusItemView)
  {
    return;
  }
  // currently we don't support search on pqSpreadSheetViewModel
  if (qobject_cast<pqSpreadSheetViewModel*>(focusItemView->model()))
  {
    return;
  }

  pqItemViewSearchWidget* searchDialog = new pqItemViewSearchWidget(focusItemView);
  searchDialog->setAttribute(Qt::WA_DeleteOnClose, true);
  searchDialog->showSearchWidget();
}

//-----------------------------------------------------------------------------
pqSelectionManager* pqPVApplicationCore::selectionManager() const
{
  return this->SelectionManager;
}

//-----------------------------------------------------------------------------
pqAnimationManager* pqPVApplicationCore::animationManager() const
{
  return this->AnimationManager;
}

//-----------------------------------------------------------------------------
pqPythonManager* pqPVApplicationCore::pythonManager() const
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  return this->PythonManager;
#else
  return nullptr;
#endif
}

//-----------------------------------------------------------------------------
pqLiveSourceManager* pqPVApplicationCore::liveSourceManager() const
{
  return this->LiveSourceManager;
}

//-----------------------------------------------------------------------------
pqTestUtility* pqPVApplicationCore::testUtility()
{
  if (!this->TestUtility)
  {
    this->TestUtility = new pqComponentsTestUtility(this);
  }
  return this->TestUtility;
}

//-----------------------------------------------------------------------------
bool pqPVApplicationCore::eventFilter(QObject* obj, QEvent* event_)
{
  if (event_->type() == QEvent::FileOpen)
  {
    QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event_);
    if (!fileEvent->file().isEmpty())
    {
      QList<QString> files;
      files.append(fileEvent->file());

      // If the application is already started just load the data
      if (vtkProcessModule::GetProcessModule()->GetSession())
      {
        pqLoadDataReaction::loadData(files);
      }
      else
      {
        // If the application has not yet started, treat it as a --data argument
        // to be processed after the application starts.
        pqCoreConfiguration::instance()->addDataFile(files[0].toUtf8().data());
      }
    }
    return false;
  }
  else
  {
    // standard event processing
    return QObject::eventFilter(obj, event_);
  }
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::loadStateFromPythonFile(
  const QString& filename, pqServer* server, vtkTypeUInt32 location)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  Q_EMIT this->aboutToReadState(filename);

  pqPythonManager* pythonMgr = this->pythonManager();
  this->clearViewsForLoadingState(server);
  // comment in pqApplicationCore says this->LoadingState is unreliable, but it is still
  // necessary to avoid warning messages
  this->LoadingState = true;
  pythonMgr->executeScriptAndRender(filename, location);
  this->LoadingState = false;
#else
  // Avoid unused parameter warnings
  (void)filename;
  (void)server;
  (void)location;
  qCritical() << "Cannot load a python state file since ParaView was not built with Python.";
#endif
}

//-----------------------------------------------------------------------------
void pqPVApplicationCore::instantiateLiveSourceManager()
{
  if (!this->LiveSourceManager)
  {
    this->LiveSourceManager = new pqLiveSourceManager(this);
  }
}
