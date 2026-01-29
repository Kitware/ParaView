// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqZSpaceManager.h"

#include "vtkPVZSpaceSettings.h"
#include "vtkPVZSpaceView.h"

#include "pqApplicationCore.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqView.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkSMViewProxy.h"
#include "vtkZSpaceRenderWindowInteractor.h"

#if PARAVIEW_USE_PYTHON
#include "pqPythonMacroSupervisor.h"
#include "pqPythonManager.h"
#endif

#include <QMap>
#include <QString>

//-----------------------------------------------------------------------------
pqZSpaceManager::pqZSpaceManager(QObject* p)
  : QObject(p)
{
  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, &pqServerManagerModel::preViewAdded, this, &pqZSpaceManager::onPreViewAdded);
  QObject::connect(smmodel, &pqServerManagerModel::viewAdded, this, &pqZSpaceManager::onViewAdded);
  QObject::connect(
    smmodel, &pqServerManagerModel::preViewRemoved, this, &pqZSpaceManager::onViewRemoved);

  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  QObject::connect(viewManager, &pqTabbedMultiViewWidget::fullScreenActiveViewEnabled, this,
    &pqZSpaceManager::onActiveFullScreenEnabled);

  // Add currently existing ZSpace views
  for (pqView* view : smmodel->findItems<pqView*>())
  {
    this->onViewAdded(view);
  }

  this->Observer = vtkMakeMemberFunctionCommand(*this, &pqZSpaceManager::onEvent);
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onShutdown()
{
  this->Observer->Delete();
  this->Observer = nullptr;

  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    // TODO fix crash due to `zccompatShutDown(this->ZSpaceContext)`
    // Related issue: https://gitlab.kitware.com/paraview/paraview/-/issues/23141
    sdkManager->ShutDown();
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onRenderEnded()
{
  pqView* view = dynamic_cast<pqView*>(sender());
  if (view != nullptr)
  {
    view->render();
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onPreViewAdded(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkPVZSpaceView")
    {
      this->ZSpaceViews.insert(view);
      QObject::connect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));

      vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
      if (sdkManager)
      {
        // Disable stereo display until active view fullscreen is on
        // (it only has effect on zSpace Inspire models)
        sdkManager->SetStereoDisplayEnabled(false);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onViewAdded(pqView* view)
{
  vtkPVZSpaceView* zSpaceView =
    vtkPVZSpaceView::SafeDownCast(view->getViewProxy()->GetClientSideObject());
  if (zSpaceView)
  {
    zSpaceView->GetInteractor()->AddObserver(
      vtkZSpaceRenderWindowInteractor::StylusButtonEvent, this->Observer);
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onViewRemoved(pqView* view)
{
  if (dynamic_cast<pqRenderView*>(view))
  {
    const std::string viewName = view->getViewProxy()->GetVTKClassName();
    if (viewName == "vtkPVZSpaceView")
    {
      QObject::disconnect(view, SIGNAL(endRender()), this, SLOT(onRenderEnded()));
      this->ZSpaceViews.erase(view);
    }
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onActiveFullScreenEnabled(bool enabled)
{
  vtkZSpaceSDKManager* sdkManager = vtkZSpaceSDKManager::GetInstance();
  if (sdkManager)
  {
    // Stereo display should only be enabled in fullscreen
    // (it only has effect on zSpace Inspire models)
    sdkManager->SetStereoDisplayEnabled(enabled);
  }
}

//-----------------------------------------------------------------------------
void pqZSpaceManager::onEvent(vtkObject*, unsigned long event, void* data)
{
#if PARAVIEW_USE_PYTHON
  if (event != vtkZSpaceRenderWindowInteractor::StylusButtonEvent)
  {
    return;
  }

  vtkZSpaceSDKManager::StylusEventData* eventData =
    reinterpret_cast<vtkZSpaceSDKManager::StylusEventData*>(data);
  if (!vtkPVZSpaceSettings::GetUseCustomMacroFromButton(eventData->Button) ||
    eventData->State != vtkZSpaceSDKManager::Down)
  {
    return;
  }

  QAction* runMacroAction = this->findMacro(eventData->Button);
  if (runMacroAction)
  {
    runMacroAction->trigger();
  }
#else
  Q_UNUSED(event);
  Q_UNUSED(data);
#endif
}

//-----------------------------------------------------------------------------
QAction* pqZSpaceManager::findMacro(vtkZSpaceSDKManager::ButtonIds buttonId)
{
#if PARAVIEW_USE_PYTHON
  QString macroName = QString::fromStdString(vtkPVZSpaceSettings::GetMacroNameFromButton(buttonId));
  if (macroName.isEmpty())
  {
    std::string buttonName = vtkZSpaceSDKManager::ButtonToString(buttonId);
    vtkWarningWithObjectMacro(
      nullptr, << "There is no macro specified for the " << buttonName << " button");
    return nullptr;
  }

  pqPythonMacroSupervisor* macroSupervisor =
    pqPVApplicationCore::instance()->pythonManager()->macroSupervisor();
  QMap<QString, QString> macros = macroSupervisor->getStoredMacros();
  QString macroFilename = macros.key(macroName);
  return macroSupervisor->getMacro(macroFilename);
#else
  Q_UNUSED(buttonId);
  return nullptr;
#endif
}
