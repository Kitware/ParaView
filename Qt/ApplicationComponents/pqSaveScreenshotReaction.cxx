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
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqImageUtil.h"
#include "pqPVApplicationCore.h"
#include "pqRenderViewBase.h"
#include "pqSaveSnapshotDialog.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqView.h"
#include "vtkImageData.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"

#include "vtkPVConfig.h"
#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#include "pqPythonDialog.h"
#include "pqPythonShell.h"
#endif

#include <QDebug>
#include <QFileInfo>

//-----------------------------------------------------------------------------
pqSaveScreenshotReaction::pqSaveScreenshotReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(activeObjects, SIGNAL(serverChanged(pqServer*)),
    this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)),
    this, SLOT(updateEnableState()));
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
void pqSaveScreenshotReaction::saveScreenshot()
{
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (!viewManager)
    {
    qCritical("Could not locate pqTabbedMultiViewWidget. "
      "If using custom-widget as the "
      "central widget, you cannot use pqSaveScreenshotReaction.");
    return;
    }

  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
    {
    qDebug() << "Cannnot save image. No active view.";
    return;
    }

  pqSaveSnapshotDialog ssDialog(pqCoreUtilities::mainWidget());
  ssDialog.setViewSize(view->getSize());
  ssDialog.setAllViewsSize(viewManager->clientSize());

  if (ssDialog.exec() != QDialog::Accepted)
    {
    return;
    }

  QString lastUsedExt;
  // Load the most recently used file extensions from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("extensions/ScreenshotExtension"))
    {
    lastUsedExt = 
      settings->value("extensions/ScreenshotExtension").toString();
    }

  QString filters;
  filters += "PNG image (*.png)";
  filters += ";;BMP image (*.bmp)";
  filters += ";;TIFF image (*.tif)";
  filters += ";;PPM image (*.ppm)";
  filters += ";;JPG image (*.jpg)";
  filters += ";;PDF file (*.pdf)";
  pqFileDialog file_dialog(NULL,
    pqCoreUtilities::mainWidget(),
    tr("Save Screenshot:"), QString(), filters);
  file_dialog.setRecentlyUsedExtension(lastUsedExt);
  file_dialog.setObjectName("FileSaveScreenshotDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
    {
    return;
    }

  QString file = file_dialog.getSelectedFiles()[0];
  QFileInfo fileInfo = QFileInfo( file );
  lastUsedExt = QString("*.") + fileInfo.suffix();
  settings->setValue("extensions/ScreenshotExtension", lastUsedExt);

  QSize size = ssDialog.viewSize();
  QString palette = ssDialog.palette();

  // temporarily load the color palette chosen by the user.
  vtkSmartPointer<vtkPVXMLElement> currentPalette;
  pqApplicationCore* core = pqApplicationCore::instance();
  if (!palette.isEmpty())
    {
    currentPalette.TakeReference(core->getCurrrentPalette());
    core->loadPalette(palette);
    }

  int stereo = ssDialog.getStereoMode();
  if (stereo)
    {
    pqRenderViewBase::setStereo(stereo);
    }

  pqSaveScreenshotReaction::saveScreenshot(file,
    size, ssDialog.quality(), ssDialog.saveAllViews());

  // restore palette.
  if (!palette.isEmpty())
    {
    core->loadPalette(currentPalette);
    }

  if (stereo)
    {
    pqRenderViewBase::setStereo(0);
    core->render();
    }
}

//-----------------------------------------------------------------------------
void pqSaveScreenshotReaction::saveScreenshot(
  const QString& filename, const QSize& size, int quality, bool all_views)
{
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (!viewManager)
    {
    qCritical("Could not locate pqTabbedMultiViewWidget. "
      "If using custom-widget as the "
      "central widget, you cannot use pqSaveScreenshotReaction.");
    return;
    }
  pqView* view = pqActiveObjects::instance().activeView();
  vtkSmartPointer<vtkImageData> img;
  if (all_views)
    {
    img.TakeReference(
      viewManager->captureImage(size.width(), size.height()));
    }
  else if (view)
    {
    img.TakeReference(view->captureImage(size));
    }

  if (img.GetPointer() == NULL)
    {
    qCritical() << "Save Image failed.";
    }
  else
    {
    pqImageUtil::saveImage(img, filename, quality);
    }

#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager* manager = pqPVApplicationCore::instance()->pythonManager();
  if (manager && manager->interpreterIsInitialized())
    {
    QString allViewsStr = all_views ? "True" : "False";
    QString script =
    "try:\n"
    "  paraview.smtrace\n"
    "  paraview.smtrace.trace_save_screenshot('%1', (%2, %3), %4)\n"
    "except AttributeError: pass\n";
    script = script.arg(filename).arg(size.width()).arg(size.height()).arg(allViewsStr);
    pqPythonShell* shell = manager->pythonShellDialog()->shell();
    shell->executeScript(script);
    return;
    }
#endif
}
