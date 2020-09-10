/*=========================================================================

   Program: ParaView
   Module:    pqSaveScreenshotReaction.cxx

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
#include "pqSaveScreenshotReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqEventDispatcher.h"
#include "pqFileDialog.h"
#include "pqImageUtil.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqView.h"
#include "vtkImageData.h"
#include "vtkNew.h"
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
  bool is_enabled = (activeObjects->activeView() && activeObjects->activeServer());
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
QString pqSaveScreenshotReaction::promptFileName(
  vtkSMSaveScreenshotProxy* prototype, const QString& defaultExtension)
{
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
  if (filters.size() == 0)
  {
    qWarning("No image writers detected.");
    return QString();
  }

  pqFileDialog file_dialog(
    NULL, pqCoreUtilities::mainWidget(), tr(prototype->GetXMLLabel()), QString(), filters.c_str());
  file_dialog.setRecentlyUsedExtension(lastUsedExt);
  file_dialog.setObjectName(QString("%1FileDialog").arg(prototype->GetXMLName()));
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
  {
    return QString();
  }

  QString file = file_dialog.getSelectedFiles()[0];
  QFileInfo fileInfo(file);
  settings->setValue(skey, fileInfo.suffix().prepend("*."));
  return file;
}

//-----------------------------------------------------------------------------
void pqSaveScreenshotReaction::saveScreenshot(bool clipboardMode)
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannot save image. No active view.";
    return;
  }

  if (clipboardMode)
  {
    pqSaveScreenshotReaction::copyScreenshotToClipboard(view->getSize(), false);
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

  // Get the filename first, this will determine some of the options shown.
  QString filename = pqSaveScreenshotReaction::promptFileName(shProxy, "*.png");
  if (filename.isEmpty())
  {
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
  shProxy->UpdateDefaultsAndVisibilities(filename.toLocal8Bit().data());
  controller->PostInitializeProxy(shProxy);

  pqProxyWidgetDialog dialog(shProxy, pqCoreUtilities::mainWidget());
  dialog.setObjectName("SaveScreenshotDialog");
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle("Save Screenshot Options");
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveScreenshotDialog");
  if (dialog.exec() == QDialog::Accepted)
  {
    shProxy->WriteImage(filename.toLocal8Bit().data());
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
  // This should not be needed as image capturing code only affects back buffer,
  // however it is currently needed due to paraview/paraview#17256. Once that's
  // fixed, we should remove this.
  pqApplicationCore::instance()->render();
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
  vtkSmartPointer<vtkImageData> image = takeScreenshot(size, all_views);
  if (!image)
  {
    return false;
  }
  return vtkSMUtilities::SaveImage(image, filename.toLocal8Bit().data(), quality) != 0;
}

//-----------------------------------------------------------------------------
bool pqSaveScreenshotReaction::copyScreenshotToClipboard(const QSize& size, bool all_views)
{
  vtkSmartPointer<vtkImageData> image = takeScreenshot(size, all_views);
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
