// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCrashRecoveryBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqProgressManager.h"
#include "pqSaveStateReaction.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"

#include "vtksys/SystemTools.hxx"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

#define CrashRecoveryStateFile ".PVCrashRecoveryState.pvsm"
#include <cstdlib> /* EXIT_FAILURE */
#if !defined(_WIN32)
#include <unistd.h> /* _exit */
#endif

#include <cassert>

//-----------------------------------------------------------------------------
pqCrashRecoveryBehavior::pqCrashRecoveryBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  // Look for a crash recovery state file, nag user and load if desired.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool recoveryEnabled = settings->value("GeneralSettings.CrashRecovery", false).toBool();
  if (recoveryEnabled && QFile::exists(CrashRecoveryStateFile))
  {
    int recover = QMessageBox::question(pqCoreUtilities::mainWidget(), tr("ParaView"),
      tr("A crash recovery state file has been found.\n"
         "Would you like to save it?"),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (recover == QMessageBox::Yes)
    {
      QString fileExt = tr("ParaView state file") + " (*.pvsm);;" + tr("All files") + " (*)";
      QString path = QFileDialog::getSaveFileName(
        pqCoreUtilities::mainWidget(), tr("Save crash state file"), QDir::currentPath(), fileExt);
      if (!path.isNull())
      {
        if (!path.endsWith(".pvsm"))
        {
          path += ".pvsm";
        }
        QFile::copy(CrashRecoveryStateFile, path);
      }
    }
  }
  if (QFile::exists(CrashRecoveryStateFile))
  {
    QFile::remove(CrashRecoveryStateFile);
  }
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(dataUpdated(pqPipelineSource*)), this, SLOT(delayedSaveRecoveryState()));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerAdded(pqServer*)));

  this->Timer.setInterval(1000);
  this->Timer.setSingleShot(true);
  QObject::connect(&this->Timer, SIGNAL(timeout()), this, SLOT(saveRecoveryState()));
}

//-----------------------------------------------------------------------------
pqCrashRecoveryBehavior::~pqCrashRecoveryBehavior()
{
  // Paraview is closing all is well, remove the crash
  // recovery file.
  if (QFile::exists(CrashRecoveryStateFile))
  {
    QFile::remove(CrashRecoveryStateFile);
  }
}

//-----------------------------------------------------------------------------
void pqCrashRecoveryBehavior::delayedSaveRecoveryState()
{
  this->Timer.start();
}

//-----------------------------------------------------------------------------
void pqCrashRecoveryBehavior::saveRecoveryState()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool recoveryEnabled = settings->value("GeneralSettings.CrashRecovery", false).toBool();
  if (recoveryEnabled)
  {
    pqApplicationCore::instance()->saveState(CrashRecoveryStateFile);
  }
}

//-----------------------------------------------------------------------------
void pqCrashRecoveryBehavior::onServerAdded(pqServer* server)
{
  QObject::connect(server, SIGNAL(serverSideDisconnected()), this, SLOT(onServerDisconnect()));
}

//-----------------------------------------------------------------------------
void pqCrashRecoveryBehavior::onServerDisconnect()
{
  // Prevent re-execution
  QObject::disconnect(this);
  static bool inQuit = false;
  if (inQuit)
  {
    return;
  }
  inQuit = true;

  if (vtksys::SystemTools::GetEnv("DASHBOARD_TEST_FROM_CTEST") == nullptr)
  {
    // enable user interaction (BUG #17155).
    pqProgressManager* pgm = pqApplicationCore::instance()->getProgressManager();
    bool prev = pgm->unblockEvents(true);

    // Try to handle recovery
    QMessageBox mbox(QMessageBox::Critical, tr("Server disconnected!"),
      tr("The server side has disconnected. "
         "The application will now quit since it may be in an unrecoverable state.\n\n"
         "Would you like to save a ParaView state file?"),
      QMessageBox::NoButton, pqCoreUtilities::mainWidget());
    mbox.addButton(QMessageBox::Yes)->setText(tr("Save state and exit"));
    mbox.addButton(QMessageBox::No)->setText(tr("Exit without saving state"));
    mbox.setDefaultButton(QMessageBox::Yes);
    if (mbox.exec() == QMessageBox::Yes)
    {
      pqSaveStateReaction::saveState();
    }
    // restore.
    pgm->unblockEvents(prev);
  }

  // It's tempting to think that we shouldn't exit here, but simply disconnect
  // from the server. However, more often then not, the server may disconnect in
  // middle of some communication in which case there may be other asserts or
  // checks that would fail causing the application to end up in a weird state.
  // Hence, we simply _exit().
  _exit(-1);
}
