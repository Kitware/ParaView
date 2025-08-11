// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTestingReaction.h"

#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqLockViewSizeCustomDialog.h"
#include "pqPVApplicationCore.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqTestUtility.h"

#include <QApplication>
#include <QWindow>
//-----------------------------------------------------------------------------
pqTestingReaction::pqTestingReaction(QAction* parentObject, Mode mode, Qt::ConnectionType type)
  : Superclass(parentObject, type)
{
  this->ReactionMode = mode;
  if (mode == LOCK_VIEW_SIZE)
  {
    parentObject->setCheckable(true);
    pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
      pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
    QObject::connect(
      viewManager, SIGNAL(viewSizeLocked(bool)), parentObject, SLOT(setChecked(bool)));
  }
}

//-----------------------------------------------------------------------------
void pqTestingReaction::recordTest()
{
  QString filters;
  filters += tr("XML Files") + QString(" (*.xml);;");
#ifdef QT_TESTING_WITH_PYTHON
  filters += tr("Python Files") + QString(" (*.py);;");
#endif
  filters += tr("All Files") + QString(" (*)");
  pqFileDialog fileDialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Record Test"), QString(), filters, false);
  fileDialog.setObjectName("ToolsRecordTestDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    pqTestingReaction::recordTest(fileDialog.getSelectedFiles()[0]);
  }
}

//-----------------------------------------------------------------------------
void pqTestingReaction::recordTest(const QString& filename)
{
  if (!filename.isEmpty())
  {
    pqCoreUtilities::mainWidget()->activateWindow();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // The `activateWindow` function schedules a request on the platform's event queue to set the
    // active widget. To enable the test recorder to check the active window, we use a one-time
    // timer. This allows the platform's windowing system to process the request for the active
    // window change, before we query it.
    //
    // \note We use the single shot timer method for Qt < 6.0 because Qt::SingleShotConnection as a
    // connection type was added in Qt 6.0.
    QTimer::singleShot(
      50, [=]() { pqApplicationCore::instance()->testUtility()->recordTests(filename); });
#else
    // The `activateWindow` function schedules a request on the platform's event queue to set the
    // active widget. To enable the test recorder to check the active window, we use a one-time
    // connection to the activeChanged signal of the window. This ensures that  platform's windowing
    // system has processed the request for the active window change, before we query it.
    QObject::connect(
      pqCoreUtilities::mainWidget()->window()->windowHandle(), &QWindow::activeChanged,
      pqCoreUtilities::mainWidget(),
      [filename]() { pqApplicationCore::instance()->testUtility()->recordTests(filename); },
      Qt::SingleShotConnection);
#endif
  }
}

//-----------------------------------------------------------------------------
void pqTestingReaction::playTest()
{
  QString filters;
  filters += tr("XML Files") + QString(" (*.xml);;");
#ifdef QT_TESTING_WITH_PYTHON
  filters += tr("Python Files") + QString(" (*.py);;");
#endif
  filters += tr("All Files") + QString(" (*)");
  pqFileDialog fileDialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Play Test"), QString(), filters, false);
  fileDialog.setObjectName("ToolsPlayTestDialog");
  fileDialog.setFileMode(pqFileDialog::ExistingFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    pqTestingReaction::playTest(fileDialog.getSelectedFiles()[0]);
  }
}

//-----------------------------------------------------------------------------
void pqTestingReaction::playTest(const QString& filename)
{
  if (!filename.isEmpty())
  {
    pqApplicationCore::instance()->testUtility()->playTests(filename);
  }
}

//-----------------------------------------------------------------------------
void pqTestingReaction::lockViewSize(bool lock)
{
  pqTabbedMultiViewWidget* viewManager = qobject_cast<pqTabbedMultiViewWidget*>(
    pqApplicationCore::instance()->manager("MULTIVIEW_WIDGET"));
  if (viewManager)
  {
    viewManager->lockViewSize(lock ? QSize(300, 300) : QSize(-1, -1));
    // wait for 500ms to give the widgets a chance to sync up.
    pqEventDispatcher::processEventsAndWait(500);
  }
  else
  {
    qCritical("pqTestingReaction requires pqTabbedMultiViewWidget.");
  }
}

//-----------------------------------------------------------------------------
void pqTestingReaction::lockViewSizeCustom()
{
  // Launch the dialog box.  The box will take care of everything else.
  pqLockViewSizeCustomDialog* sizeDialog =
    new pqLockViewSizeCustomDialog(pqCoreUtilities::mainWidget());
  QObject::connect(sizeDialog, &QWidget::close, sizeDialog, &QObject::deleteLater);
  sizeDialog->show();
}
