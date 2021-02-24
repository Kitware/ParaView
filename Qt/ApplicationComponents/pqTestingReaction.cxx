/*=========================================================================

   Program: ParaView
   Module:    pqTestingReaction.cxx

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
  filters += "XML Files (*.xml);;";
#ifdef QT_TESTING_WITH_PYTHON
  filters += "Python Files (*.py);;";
#endif
  filters += "All Files (*)";
  pqFileDialog fileDialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Record Test"), QString(), filters);
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
  filters += "XML Files (*.xml);;";
#ifdef QT_TESTING_WITH_PYTHON
  filters += "Python Files (*.py);;";
#endif
  filters += "All Files (*)";
  pqFileDialog fileDialog(
    nullptr, pqCoreUtilities::mainWidget(), tr("Play Test"), QString(), filters);
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
