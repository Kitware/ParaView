/*=========================================================================

   Program: ParaView
   Module:  pqResetDefaultSettingsReaction.cxx

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
#include "pqResetDefaultSettingsReaction.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqOptions.h"
#include "pqSettings.h"
#include "vtkInitializationHelper.h"
#include "vtkSMSettings.h"

#include <QAbstractButton>
#include <QApplication>
#include <QMessageBox>
#include <QtDebug>

//-----------------------------------------------------------------------------
pqResetDefaultSettingsReaction::pqResetDefaultSettingsReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqResetDefaultSettingsReaction::~pqResetDefaultSettingsReaction() = default;

//-----------------------------------------------------------------------------
void pqResetDefaultSettingsReaction::resetSettingsToDefault()
{
  QMessageBox mbox(pqCoreUtilities::mainWidget());
  mbox.setWindowTitle("Reset to Default Settings");
  mbox.setIcon(QMessageBox::Question);
  mbox.setText("<b>Reset custom settings to default.</b>");
  mbox.setInformativeText(
    "All custom setting will be reset to their default values. Do you want to continue?");
  mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::Save | QMessageBox::Cancel);

  mbox.button(QMessageBox::Yes)->setText("Yes");
  mbox.button(QMessageBox::Save)->setText("Yes, and backup current settings");
  mbox.setDefaultButton(QMessageBox::Cancel);

  QString informativeText;
  QTextStream stream(&informativeText);

  switch (mbox.exec())
  {
    case QMessageBox::Yes:
      this->clearSettings();
      break;

    case QMessageBox::Save:
    {
      auto backups = this->backupSettings();
      if (backups.size() == 0)
      {
        if (QMessageBox::critical(pqCoreUtilities::mainWidget(), tr("Backups failed!"),
              tr("Failed to generate backup files. Do you want to continue?"),
              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        {
          return;
        }
      }
      else
      {
        stream << tr("Following backup files have been generated:\n\n") << backups.join("\n\n")
               << tr("\n\n");
      }
      this->clearSettings();
    }
    break;

    default:
      return;
  }
  stream
    << tr("Please restart %1 for the changes to take effect.").arg(QApplication::applicationName());

  QMessageBox mbox2(pqCoreUtilities::mainWidget());
  mbox2.setWindowTitle(tr("Reset to Default Settings"));
  mbox2.setIcon(QMessageBox::Information);
  mbox2.setText(tr("<b>Settings reset to default</b>"));
  mbox2.setInformativeText(informativeText);
  mbox2.setStandardButtons(QMessageBox::Ok);
  mbox2.setDefaultButton(QMessageBox::Ok);
  mbox2.exec();
}

//-----------------------------------------------------------------------------
QStringList pqResetDefaultSettingsReaction::backupSettings()
{
  auto core = pqApplicationCore::instance();
  if (core->getOptions()->GetDisableRegistry())
  {
    // if registry is disabled, don't bother backing up.
    return QStringList("(`-dr` specified, no backup generated)");
  }

  auto settings = core->settings();
  QString fname = settings->backup();
  if (fname.isEmpty())
  {
    return QStringList();
  }
  qInfo() << "Saved " << fname;

  QStringList retval;
  retval << fname;

  const QString smsettingsFile(vtkInitializationHelper::GetUserSettingsFilePath().c_str());
  if (QFile::exists(smsettingsFile))
  {
    const QString backup(smsettingsFile + ".bak");
    QFile::remove(backup);
    if (!QFile::copy(smsettingsFile, backup))
    {
      return QStringList();
    }
    retval << backup;
    qInfo() << "Saved " << backup;
  }
  return retval;
}

//-----------------------------------------------------------------------------
void pqResetDefaultSettingsReaction::clearSettings()
{
  auto core = pqApplicationCore::instance();
  if (core->getOptions()->GetDisableRegistry())
  {
    // if registry is disabled, there's nothing to do.
    return;
  }

  core->clearSettings();
  vtkSMSettings::GetInstance()->ClearAllSettings();
  if (vtkInitializationHelper::GetLoadSettingsFilesDuringInitialization())
  {
    QFile::remove(vtkInitializationHelper::GetUserSettingsFilePath().c_str());
  }
}
