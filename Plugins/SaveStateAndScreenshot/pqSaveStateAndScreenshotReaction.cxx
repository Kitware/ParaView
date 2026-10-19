// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSaveStateAndScreenshotReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqDataRepresentation.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqPipelineModel.h"
#include "pqProxyWidgetDialog.h"
#include "pqRenderView.h"
#include "pqSaveScreenshotReaction.h"
#include "pqSaveStateReaction.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

#include "vtkCollection.h"
#include "vtkPVXMLElement.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSMViewResolutionDomain.h"
#include "vtksys/SystemTools.hxx"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTimer>

#include <array>

//-----------------------------------------------------------------------------
pqSaveStateAndScreenshotReaction::pqSaveStateAndScreenshotReaction(
  QAction* saveAction, QAction* settingsAction)
  : Superclass(saveAction)
  , SettingsAction(settingsAction)
{
  QObject::connect(settingsAction, SIGNAL(triggered()), this, SLOT(onSettings()));
  QObject::connect(settingsAction, SIGNAL(triggered()), this, SLOT(updateEnableState()));
  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(onViewChanged()));
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(aboutToRemoveServer(pqServer*)), this, SLOT(onAboutToRemoveServer(pqServer*)));
  this->updateEnableState();
  this->FromCTest = (vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") != nullptr);
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onViewChanged()
{
  // a new view becomes active before it is assigned to a layout
  QTimer::singleShot(0, this,
    [this]()
    {
      vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(this->Proxy);
      if (shProxy)
      {
        pqView* view = pqActiveObjects::instance().activeView();
        this->ResetView(shProxy, view ? view->getViewProxy() : nullptr);
        this->updateEnableState();
      }
    });
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onAboutToRemoveServer(pqServer* server)
{
  if (this->Proxy && this->Proxy->GetSession() == server->session())
  {
    this->Proxy = nullptr;
    this->updateEnableState();
  }
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::ResetView(
  vtkSMSaveScreenshotProxy* shProxy, vtkSMViewProxy* viewProxy)
{
  vtkSMProxy* viewSaved = vtkSMPropertyHelper(shProxy, "View").GetAsProxy();
  if (viewSaved == viewProxy)
  {
    return;
  }
  vtkSMViewProxy* viewProxyNull = nullptr;
  vtkSMViewLayoutProxy* layout = viewProxy ? vtkSMViewLayoutProxy::FindLayout(viewProxy) : nullptr;
  if (!layout)
  {
    // nothing we can capture, disable save until a view in a layout becomes active
    vtkSMPropertyHelper(shProxy, "View").Set(viewProxyNull);
    return;
  }
  vtkSMProxy* layoutSaved = vtkSMPropertyHelper(shProxy, "Layout").GetAsProxy();
  vtkSMPropertyHelper(shProxy, "View").Set(viewProxy);
  if (vtkSMPropertyHelper(shProxy, "SaveAllViews").GetAsInt() != 0 && layout == layoutSaved)
  {
    // we capture the same layout, so we keep the configured resolution
    shProxy->UpdateVTKObjects();
    return;
  }
  vtkSMPropertyHelper(shProxy, "Layout").Set(layout);
  // the capture target changed, so Save uses the size of the new view or layout
  this->ResolutionFromTarget = true;
  shProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::updateEnableState()
{
  vtkSMProxy* viewProxy = nullptr;
  if (this->Proxy)
  {
    vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(this->Proxy);
    viewProxy = vtkSMPropertyHelper(shProxy, "View").GetAsProxy();
  }
  this->parentAction()->setEnabled(
    !this->Name.isNull() && !this->Directory.isNull() && this->Proxy && viewProxy);
  this->SettingsAction->setEnabled(true);
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::CopyProperties(
  vtkSMSaveScreenshotProxy* shProxySaved, vtkSMSaveScreenshotProxy* shProxy)
{
  // if I copy ImageResolution over the color bar and fonts are very big
  std::array<const char*, 7> properties = { "SaveAllViews", "FontScaling", "SeparatorWidth",
    "StereoMode", "TransparentBackground", "SeparatorColor", "OverrideColorPalette" };
  for (const char* property : properties)
  {
    vtkSMProperty* src = shProxySaved->GetProperty(property);
    vtkSMProperty* target = shProxy->GetProperty(property);
    target->Copy(src);
  }
  // both formats are PNG, so this keeps compression, metadata and embedded state
  vtkSMProxy* formatSaved = vtkSMPropertyHelper(shProxySaved, "Format").GetAsProxy();
  vtkSMProxy* format = vtkSMPropertyHelper(shProxy, "Format").GetAsProxy();
  format->Copy(formatSaved);
  format->UpdateVTKObjects();
  shProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onTriggered()
{
  vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(this->Proxy);
  pqView* view = pqActiveObjects::instance().activeView();
  if (shProxy && view)
  {
    if (this->ResolutionFromTarget)
    {
      // a new view has its real size only after it is shown, so read it now
      vtkSMProperty* resolution = shProxy->GetProperty("ImageResolution");
      resolution->FindDomain<vtkSMViewResolutionDomain>()->Update(nullptr);
      resolution->ResetToDomainDefaults();
      shProxy->UpdateVTKObjects();
    }
    // save the file
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("-yyyyMMdd-hhmmss");
    QString nameNoExt = this->Name + (this->FromCTest ? "" : dateTimeString);
    QString pathNoExt = this->Directory + "/" + nameNoExt;
    QString stateFile = pathNoExt + ".pvsm";
    pqSaveStateReaction::saveState(stateFile, this->Location);
    QString screenshotFile = pathNoExt + ".png";

    vtkSMProxy* formatProxy = vtkSMPropertyHelper(shProxy, "Format").GetAsProxy();
    vtkSMProperty* embedStateProp = formatProxy->GetProperty("EmbedParaViewState");
    vtkSmartPointer<vtkPVXMLElement> stateXMLRoot;
    if (embedStateProp && vtkSMPropertyHelper(embedStateProp).GetAsInt() == 1)
    {
      Q_EMIT pqApplicationCore::instance() -> aboutToWriteState(screenshotFile);
      vtkSMSessionProxyManager* pxm = view->getServer()->proxyManager();
      stateXMLRoot = vtkSmartPointer<vtkPVXMLElement>::Take(pxm->SaveXMLState());
    }
    else
    {
      stateXMLRoot = nullptr;
    }
    shProxy->WriteImage(screenshotFile.toUtf8().data(), this->Location, stateXMLRoot);
    QString textFile = pathNoExt + ".txt";
    auto pxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
    pxm->SaveString(nameNoExt.toUtf8().data(), textFile.toUtf8().data(), this->Location);
  }
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onSettings()
{
  pqServer* server = pqActiveObjects::instance().activeServer();
  // Configure directory and name
  QString fileExt =
    QString("%1 (*.pvsm);;%2 (*)").arg(tr("ParaView state file")).arg(tr("All files"));
  pqFileDialog fileDialog(server, pqCoreUtilities::mainWidget(), tr("Save State and Screenshot"),
    QString(), fileExt, false, false);

  fileDialog.setObjectName("FileSaveServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() != QDialog::Accepted)
  {
    return;
  }
  QString selectedFile = fileDialog.getSelectedFiles()[0];
  this->Location = fileDialog.getSelectedLocation();
  QFileInfo info(selectedFile);
  this->Directory = info.dir().absolutePath();
  this->Name = info.completeBaseName();
  QRegularExpression re("-\\d{8}-\\d{6}$");
  this->Name.remove(re);

  // Configure Screenshot options
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    return;
  }

  vtkSMViewProxy* viewProxy = view->getViewProxy();
  vtkSMViewLayoutProxy* layout = vtkSMViewLayoutProxy::FindLayout(viewProxy);
  vtkSMSessionProxyManager* pxm = view->getServer()->proxyManager();
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "SaveScreenshot"));
  vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(proxy);
  if (!shProxy)
  {
    qCritical() << "Incorrect type for `SaveScreenshot` proxy.";
    return;
  }

  bool restorePreviewMode = false;

  // Cache the separator width and color
  int width = vtkSMPropertyHelper(shProxy, "SeparatorWidth").GetAsInt();
  double color[3];
  vtkSMPropertyHelper(shProxy, "SeparatorColor").Get(color, 3);
  // Link the vtkSMViewLayoutProxy to vtkSMSaveScreenshotProxy to update
  // the SeparatorWidth and SeparatorColor
  vtkNew<vtkSMPropertyLink> widthLink, colorLink;
  if (layout)
  {
    widthLink->AddLinkedProperty(shProxy, "SeparatorWidth", vtkSMLink::INPUT);
    widthLink->AddLinkedProperty(layout, "SeparatorWidth", vtkSMLink::OUTPUT);
    colorLink->AddLinkedProperty(shProxy, "SeparatorColor", vtkSMLink::INPUT);
    colorLink->AddLinkedProperty(layout, "SeparatorColor", vtkSMLink::OUTPUT);
  }

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(shProxy);
  vtkSMPropertyHelper(shProxy, "SaveAllViews").Set(1);
  vtkSMPropertyHelper(shProxy, "View").Set(viewProxy);
  vtkSMPropertyHelper(shProxy, "Layout").Set(layout);
  // use a fake file to set the Format (PNG)
  shProxy->UpdateDefaultsAndVisibilities("fake.png");
  controller->PostInitializeProxy(shProxy);

  if (layout)
  {
    int previewMode[2] = { -1, -1 };
    vtkSMPropertyHelper previewHelper(layout, "PreviewMode");
    previewHelper.Get(previewMode, 2);
    if (previewMode[0] == 0 && previewMode[1] == 0)
    {
      // If we are not in the preview mode, enter it with maximum size possible
      vtkVector2i layoutSize = layout->GetSize();
      previewHelper.Set(layoutSize.GetData(), 2);
      restorePreviewMode = true;
    }
    else
    {
      // if in preview mode, check "save all views".
      vtkSMPropertyHelper(shProxy, "SaveAllViews").Set(1);
    }
  }

  if (this->Proxy)
  {
    vtkSMSaveScreenshotProxy* shProxySaved = vtkSMSaveScreenshotProxy::SafeDownCast(this->Proxy);
    this->CopyProperties(shProxySaved, shProxy);
  }

  pqProxyWidgetDialog dialog(shProxy, pqCoreUtilities::mainWidget());
  dialog.setObjectName("SaveScreenshotDialog");
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle(tr("Save Screenshot Options"));
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveScreenshotDialog");
  if (dialog.exec() == QDialog::Accepted)
  {
    this->Proxy = proxy;
    this->ResolutionFromTarget = false;
  }

  if (layout)
  {
    // Reset the separator width and color
    vtkSMPropertyHelper(layout, "SeparatorWidth").Set(width);
    vtkSMPropertyHelper(layout, "SeparatorColor").Set(color, 3);
    // Reset to the previous preview resolution or exit preview mode
    if (restorePreviewMode)
    {
      int psize[2] = { 0, 0 };
      vtkSMPropertyHelper(layout, "PreviewMode").Set(psize, 2);
    }
    layout->UpdateVTKObjects();
    widthLink->RemoveAllLinks();
    colorLink->RemoveAllLinks();
  }
}
