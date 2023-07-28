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
    QApplication::setActiveWindow(pqCoreUtilities::mainWidget());
    pqApplicationCore::instance()->testUtility()->recordTests(filename);
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
  sizeDialog->setAttribute(Qt::WA_DeleteOnClose, true);
  sizeDialog->show();
}
