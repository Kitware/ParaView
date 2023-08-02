// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSaveScreenshotReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqImageUtil.h"
#include "pqProxyWidgetDialog.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqUndoStack.h"
#include "pqView.h"

#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMSaveScreenshotProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMUtilities.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QMainWindow>
#include <QMimeData>
#include <QStatusBar>

//-----------------------------------------------------------------------------
pqSaveScreenshotReaction::pqSaveScreenshotReaction(QAction* parentObject, bool clipboardMode)
  : Superclass(parentObject)
  , ClipboardMode(clipboardMode)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveScreenshotReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  const bool is_enabled = (activeObjects->activeView() && activeObjects->activeServer());
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
QString pqSaveScreenshotReaction::promptFileName(
  vtkSMSaveScreenshotProxy* prototype, const QString& defaultExtension, vtkTypeUInt32& location)
{
  location = vtkPVSession::CLIENT;
  if (!prototype)
  {
    qWarning("No `prototype` proxy specified.");
    return QString();
  }

  const QString skey =
    QString("extensions/%1/%2").arg(prototype->GetXMLGroup()).arg(prototype->GetXMLName());

  // Load the most recently used file extensions from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  const QString lastUsedExt = settings->value(skey, defaultExtension).toString();

  auto filters = prototype->GetFileFormatFilters();
  if (filters.empty())
  {
    qWarning("No image writers detected.");
    return QString();
  }

  pqServer* server = pqActiveObjects::instance().activeServer();
  pqFileDialog file_dialog(server, pqCoreUtilities::mainWidget(),
    QCoreApplication::translate("ServerManagerXML", prototype->GetXMLLabel()), QString(),
    filters.c_str(), false, false);
  file_dialog.setRecentlyUsedExtension(lastUsedExt);
  file_dialog.setObjectName(QString("%1FileDialog").arg(prototype->GetXMLName()));
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
  {
    return QString();
  }
  location = file_dialog.getSelectedLocation();

  QString file = file_dialog.getSelectedFiles()[0];
  const QFileInfo fileInfo(file);
  settings->setValue(skey, fileInfo.suffix().prepend("*."));
  return file;
}

//-----------------------------------------------------------------------------
bool pqSaveScreenshotReaction::saveScreenshot(bool clipboardMode)
{
  SCOPED_UNDO_EXCLUDE();
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannot save image. No active view.";
    return false;
  }

  vtkSMViewProxy* viewProxy = view->getViewProxy();

  if (clipboardMode)
  {
    // Get pixel size (not scaled pixel size) for the view.
    // fixes #20225
    const vtkSMPropertyHelper helper(viewProxy, "ViewSize");
    const QSize pixelSize(helper.GetAsInt(0), helper.GetAsInt(1));
    return pqSaveScreenshotReaction::copyScreenshotToClipboard(pixelSize, false);
  }

  vtkSMViewLayoutProxy* layout = vtkSMViewLayoutProxy::FindLayout(viewProxy);
  vtkSMSessionProxyManager* pxm = view->getServer()->proxyManager();
  auto proxy = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("misc", "SaveScreenshot"));
  vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(proxy);
  if (!shProxy)
  {
    qCritical() << "Incorrect type for `SaveScreenshot` proxy.";
    return false;
  }

  // Get the filename first, this will determine some options shown.
  vtkTypeUInt32 location;
  const QString filename = pqSaveScreenshotReaction::promptFileName(shProxy, "*.png", location);
  if (filename.isEmpty())
  {
    return false;
  }

  bool restorePreviewMode = false;

  // Cache the separator width and color
  const int width = vtkSMPropertyHelper(shProxy, "SeparatorWidth").GetAsInt();
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
  auto stateXMLRoot = vtkSmartPointer<vtkPVXMLElement>::Take(pxm->SaveXMLState());

  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->PreInitializeProxy(shProxy);

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
      // essential to give the UI a change to update after the preview change.
      pqEventDispatcher::processEvents();
    }
    else
    {
      // if in preview mode, check "save all views".
      vtkSMPropertyHelper(shProxy, "SaveAllViews").Set(1);
    }
  }

  vtkSMPropertyHelper(shProxy, "View").Set(viewProxy);
  vtkSMPropertyHelper(shProxy, "Layout").Set(layout);
  shProxy->UpdateDefaultsAndVisibilities(filename.toUtf8().data());
  controller->PostInitializeProxy(shProxy);

  pqProxyWidgetDialog dialog(shProxy, pqCoreUtilities::mainWidget());
  dialog.setObjectName("SaveScreenshotDialog");
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle(tr("Save Screenshot Options"));
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveScreenshotDialog");
  if (dialog.exec() == QDialog::Accepted)
  {
    const bool embedParaViewState =
      vtkSMPropertyHelper(shProxy, "EmbedParaViewState").GetAsInt() == 1;
    if (embedParaViewState)
    {
      Q_EMIT pqApplicationCore::instance()->aboutToWriteState(filename);
    }
    else
    {
      stateXMLRoot = nullptr;
    }
    shProxy->WriteImage(filename.toUtf8().data(), location, stateXMLRoot);
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

  return true;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> pqSaveScreenshotReaction::takeScreenshot(
  const QSize& size, bool all_views)
{
  vtkSmartPointer<vtkImageData> image;
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannot save image. No active view.";
    return image;
  }

  vtkSMViewProxy* viewProxy = view->getViewProxy();

  const vtkVector2i isize(size.width(), size.height());
  if (all_views)
  {
    vtkSMViewLayoutProxy* layout = vtkSMViewLayoutProxy::FindLayout(viewProxy);
    image = vtkSMSaveScreenshotProxy::CaptureImage(layout, isize);
  }
  else
  {
    image = vtkSMSaveScreenshotProxy::CaptureImage(viewProxy, isize);
  }
  return image;
}

//-----------------------------------------------------------------------------
bool pqSaveScreenshotReaction::saveScreenshot(
  const QString& filename, const QSize& size, int quality, bool all_views)
{
  const vtkSmartPointer<vtkImageData> image = takeScreenshot(size, all_views);
  if (!image)
  {
    return false;
  }
  return vtkSMUtilities::SaveImage(image, filename.toUtf8().data(), quality) != 0;
}

//-----------------------------------------------------------------------------
bool pqSaveScreenshotReaction::copyScreenshotToClipboard(const QSize& size, bool all_views)
{
  const vtkSmartPointer<vtkImageData> image = takeScreenshot(size, all_views);
  if (!image)
  {
    return false;
  }

  QImage qimg;
  pqImageUtil::fromImageData(image, qimg);
  QMimeData* data = new QMimeData;
  data->setImageData(qimg);
  QApplication::clipboard()->setMimeData(data);
  QMainWindow* mainWindow = qobject_cast<QMainWindow*>(pqCoreUtilities::mainWidget());
  if (mainWindow)
  {
    mainWindow->statusBar()->showMessage(
      tr("View content has been copied to the clipboard."), 2000);
  }

  return true;
}
