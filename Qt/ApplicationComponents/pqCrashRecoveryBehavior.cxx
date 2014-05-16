/*=========================================================================

   Program: ParaView
   Module:    pqCrashRecoveryBehavior.cxx

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
#include "pqCrashRecoveryBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqSaveStateReaction.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"

#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#define CrashRecoveryStateFile ".PVCrashRecoveryState.pvsm"

//-----------------------------------------------------------------------------
pqCrashRecoveryBehavior::pqCrashRecoveryBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  // Look for a crash recovery state file, nag user and load if desired.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool recoveryEnabled = settings->value(
    "GeneralSettings.CrashRecovery", false).toBool();
  if (recoveryEnabled && QFile::exists(CrashRecoveryStateFile))
    {
    int recover = QMessageBox::question(
      pqCoreUtilities::mainWidget(),
      "ParaView",
      "A crash recovery state file has been found.\n"
      "Would you like to save it?",
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);
    if (recover==QMessageBox::Yes)
      {
      QString fileExt = tr("ParaView state file (*.pvsm);;All files (*)");
      QString path = QFileDialog::getSaveFileName(pqCoreUtilities::mainWidget(),
                                                  "Save crash state file",
                                                  QDir::currentPath(),
                                                  fileExt);
      if(!path.isNull())
        {
        if(!path.endsWith(".pvsm"))
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
    SIGNAL(dataUpdated(pqPipelineSource*)),
    this, SLOT(delayedSaveRecoveryState()));

  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)),
    this, SLOT(onServerAdded(pqServer*)));

  this->Timer.setInterval(1000);
  this->Timer.setSingleShot(true);
  QObject::connect(&this->Timer, SIGNAL(timeout()),this,
    SLOT(saveRecoveryState()));
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
  bool recoveryEnabled = settings->value(
    "GeneralSettings.CrashRecovery", false).toBool();
  if (recoveryEnabled)
    {
    pqApplicationCore::instance()->saveState(CrashRecoveryStateFile);
    }
}

//-----------------------------------------------------------------------------
void pqCrashRecoveryBehavior::onServerAdded(pqServer* server)
{
  QObject::connect(server,
                   SIGNAL(serverSideDisconnected()),
                   this, SLOT(onServerDisconnect()));
}

//-----------------------------------------------------------------------------
void pqCrashRecoveryBehavior::onServerDisconnect()
{
  // Prevent re-execution
  QObject::disconnect(this);
  static bool inQuit = false;
  if(inQuit)
    {
    return;
    }
  inQuit = true;

  // Try to handle recovery
  int recover = QMessageBox::question(
        pqCoreUtilities::mainWidget(),
        "ParaView",
        "The server side has disconnected.\n"
        "Would you like to save a ParaView state file?",
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No);
  if (recover==QMessageBox::Yes)
    {
    pqSaveStateReaction::saveState();
    }
  QApplication::instance()->quit();
}
