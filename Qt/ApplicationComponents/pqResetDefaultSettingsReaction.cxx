// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqResetDefaultSettingsReaction.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqSettings.h"
#include "vtkInitializationHelper.h"
#include "vtkPVStandardPaths.h"
#include "vtkRemotingCoreConfiguration.h"
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
  mbox.setWindowTitle(tr("Reset to Default Settings"));
  mbox.setIcon(QMessageBox::Question);
  mbox.setText(QString("<b>%1</b>").arg(tr("Reset custom settings to default.")));
  mbox.setInformativeText(
    tr("All custom setting will be reset to their default values. Do you want to continue?"));
  mbox.setStandardButtons(QMessageBox::Yes | QMessageBox::Save | QMessageBox::Cancel);

  mbox.button(QMessageBox::Yes)->setText(tr("Yes"));
  mbox.button(QMessageBox::Save)->setText(tr("Yes, and backup current settings"));
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
      if (backups.empty())
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
  mbox2.setText(QString("<b>%1</b>").arg(tr("Settings reset to default")));
  mbox2.setInformativeText(informativeText);
  mbox2.setStandardButtons(QMessageBox::Ok);
  mbox2.setDefaultButton(QMessageBox::Ok);
  mbox2.exec();
}

//-----------------------------------------------------------------------------
QStringList pqResetDefaultSettingsReaction::backupSettings()
{
  if (vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry())
  {
    // if registry is disabled, don't bother backing up.
    return QStringList("(`-dr` specified, no backup generated)");
  }

  auto core = pqApplicationCore::instance();
  auto settings = core->settings();
  QString fname = settings->backup();
  if (fname.isEmpty())
  {
    return QStringList();
  }
  qInfo() << "Saved " << fname;

  QStringList retval;
  retval << fname;

  const QString smsettingsFile(vtkPVStandardPaths::GetUserSettingsFilePath().c_str());
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
  if (vtkRemotingCoreConfiguration::GetInstance()->GetDisableRegistry())
  {
    // if registry is disabled, there's nothing to do.
    return;
  }

  auto core = pqApplicationCore::instance();
  core->clearSettings();
  vtkSMSettings::GetInstance()->ClearAllSettings();
  if (vtkInitializationHelper::GetLoadSettingsFilesDuringInitialization())
  {
    QFile::remove(vtkPVStandardPaths::GetUserSettingsFilePath().c_str());
  }
}
