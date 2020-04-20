/*=========================================================================

   Program: ParaView
   Module:    pqPVApplicationCore.cxx

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
#include "pqPVApplicationCore.h"
#include "vtkPVConfig.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqApplicationComponentsInit.h"
#include "pqApplicationCore.h"
#include "pqComponentsTestUtility.h"
#include "pqCoreUtilities.h"
#include "pqItemViewSearchWidget.h"
#include "pqLoadDataReaction.h"
#include "pqOptions.h"
#include "pqPresetGroupsManager.h"
#include "pqPropertiesPanel.h"
#include "pqQuickLaunchDialog.h"
#include "pqSelectionManager.h"
#include "pqSetName.h"
#include "pqSpreadSheetViewModel.h"
#include "vtkPVLogger.h"
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
#include <QShortcut>

//-----------------------------------------------------------------------------
pqPVApplicationCore::pqPVApplicationCore(int& argc, char** argv, pqOptions* options)
  : Superclass(argc, argv, options)
{
  // Initialize pqComponents resources.
  pqApplicationComponentsInit();

  this->AnimationManager = new pqAnimationManager(this);
  this->SelectionManager = new pqSelectionManager(this);

  pqApplicationCore::instance()->registerManager("SELECTION_MANAGER", this->SelectionManager);

  pqPresetGroupsManager* presetGroupManager = new pqPresetGroupsManager(this);
  QString groupString;
  QFile groupsFile(":pqWidgets/pqPresetGroups.json");

  if (!groupsFile.open(QIODevice::ReadOnly))
  {
    qWarning() << "Could not load preset group list.";
  }
  else
  {
    groupString = groupsFile.readAll();
  }
  groupsFile.close();

  presetGroupManager->loadGroups(groupString);
  pqApplicationCore::instance()->registerManager("PRESET_GROUP_MANAGER", presetGroupManager);

  this->PythonManager = 0;
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
  if (this->QuickLaunchMenus.size() > 0)
  {
    pqQuickLaunchDialog dialog(pqCoreUtilities::mainWidget());
    foreach (QWidget* menu, this->QuickLaunchMenus)
    {
      if (menu)
      {
        // don't use QMenu::actions() since that includes only the top-level
        // actions.
        // --> BUT pqProxyGroupMenuManager in order to handle multi-server
        //         setting properly add actions into an internal widget so
        //         actions() should be used instead of findChildren()
        if (menu->findChildren<QAction*>().size() == 0)
        {
          dialog.addActions(menu->actions());
        }
        else
        {
          dialog.addActions(menu->findChildren<QAction*>());
        }
      }
    }
    // If shift modifier is pressed, let's force the auto apply
    bool forceAutoApply = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    bool autoApplyState = pqPropertiesPanel::autoApply();
    pqPropertiesPanel::setAutoApply(autoApplyState || forceAutoApply);
    dialog.exec();
    // Restore the auto apply state
    pqPropertiesPanel::setAutoApply(autoApplyState);
  }
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
  return 0;
#endif
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
        this->Options->SetParaViewDataName(files[0].toLocal8Bit().data());
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
void pqPVApplicationCore::loadStateFromPythonFile(const QString& filename, pqServer* server)
{
#if VTK_MODULE_ENABLE_ParaView_pqPython
  pqPythonManager* pythonMgr = this->pythonManager();
  this->clearViewsForLoadingState(server);
  // comment in pqApplicationCore says this->LoadingState is unreliable, but it is still
  // necessary to avoid warning messages
  this->LoadingState = true;
  pythonMgr->executeScriptAndRender(filename);
  this->LoadingState = false;
#else
  // Avoid unused parameter warnings
  (void)filename;
  (void)server;
  qCritical() << "Cannot load a python state file since ParaView was not built with Python.";
#endif
}
