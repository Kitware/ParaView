/*=========================================================================

   Program: ParaView
   Module:    pqSaveStateAndScreenshotReaction.cxx

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
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"

#include "vtkCollection.h"
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
#include "vtksys/SystemTools.hxx"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QKeySequence>
#include <QRegularExpression>
#include <QShortcut>

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
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(onViewChanged(pqView*)));
  this->updateEnableState();
  this->FromCTest = (vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") != NULL);
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onViewChanged(pqView* view)
{
  if (this->Proxy)
  {
    vtkSMViewProxy* viewProxyNull = nullptr;
    vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(this->Proxy);
    if (view)
    {

      vtkSMProxy* viewSaved =
        vtkSMViewProxy::SafeDownCast(vtkSMPropertyHelper(shProxy, "View").GetAsProxy());
      vtkSMViewProxy* viewProxy = view->getViewProxy();
      if (viewSaved != viewProxy)
      {
        // force going through settings
        vtkSMPropertyHelper(shProxy, "View").Set(viewProxyNull);
      }
    }
    else
    {
      // force going through settings
      vtkSMPropertyHelper(shProxy, "View").Set(viewProxyNull);
    }
    this->updateEnableState();
  }
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
  shProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onTriggered()
{
  vtkSMSaveScreenshotProxy* shProxy = vtkSMSaveScreenshotProxy::SafeDownCast(this->Proxy);
  pqView* view = pqActiveObjects::instance().activeView();
  if (shProxy && view)
  {
    // save the file
    QDateTime dateTime = QDateTime::currentDateTime();
    QString dateTimeString = dateTime.toString("-yyyyMMdd-hhmmss");
    QString nameNoExt = this->Name + (this->FromCTest ? "" : dateTimeString);
    ;
    QString pathNoExt = this->Directory + "/" + nameNoExt;
    QString stateFile = pathNoExt + ".pvsm";
    pqSaveStateReaction::saveState(stateFile);
    QString screenshotFile = pathNoExt + ".png";
    shProxy->WriteImage(screenshotFile.toLocal8Bit().data());
    QString textFile = pathNoExt + ".txt";
    std::ofstream ofs(textFile.toLocal8Bit().data(), std::ofstream::out);
    ofs << nameNoExt.toLocal8Bit().data() << std::endl;
    ofs.close();
  }
}

//-----------------------------------------------------------------------------
void pqSaveStateAndScreenshotReaction::onSettings()
{
  // Configure directory and name
  QString fileExt = tr("ParaView state file (*.pvsm);;All files (*)");
  pqFileDialog fileDialog(
    NULL, pqCoreUtilities::mainWidget(), tr("Save State and Screenshot"), QString(), fileExt);

  fileDialog.setObjectName("FileSaveServerStateDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);

  if (fileDialog.exec() == QDialog::Accepted)
  {
    QString selectedFile = fileDialog.getSelectedFiles()[0];
    QFileInfo info(selectedFile);
    this->Directory = info.dir().absolutePath();
    this->Name = info.baseName();
    QRegularExpression re("-\\d+-\\d+");
    this->Name.remove(re);
  }

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
  dialog.setWindowTitle("Save Screenshot Options");
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveScreenshotDialog");
  if (dialog.exec() == QDialog::Accepted)
  {
    this->Proxy = proxy;
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
