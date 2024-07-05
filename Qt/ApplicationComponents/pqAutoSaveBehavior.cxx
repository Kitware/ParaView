// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAutoSaveBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqSaveStateReaction.h"
#include "pqSettings.h"

#include "vtkPVSession.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <QApplication>
#include <QFile>
#include <QtDebug>

namespace
{
QString AutoSaveBaseName()
{
  return QString("%1-%3").arg(QApplication::applicationName()).arg("autosave");
}

static const QString AUTOSAVE_KEY = "GeneralSettings.AutoSave";
static const QString AUTOSAVE_DIR_KEY = "GeneralSettings.AutoSaveDirectory";
static const QString AUTOSAVE_FORMAT_KEY = "GeneralSettings.AutoSaveStateFormat";
};

//-----------------------------------------------------------------------------
pqAutoSaveBehavior::pqAutoSaveBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  auto core = pqApplicationCore::instance();
  QObject::connect(
    core, &pqApplicationCore::undoStackChanged, this, &pqAutoSaveBehavior::updateConnections);
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QObject::connect(settings, &pqSettings::modified, this, &pqAutoSaveBehavior::updateConnections);
  QObject::connect(pqApplicationCore::instance(), &pqApplicationCore::clientEnvironmentDone, this,
    &pqAutoSaveBehavior::updateConnections);
}

//-----------------------------------------------------------------------------
pqAutoSaveBehavior::~pqAutoSaveBehavior() = default;

//-----------------------------------------------------------------------------
QString pqAutoSaveBehavior::formatToExtension(StateFormat format)
{
  switch (format)
  {
    case StateFormat::Python:
      return "py";
    case StateFormat::PVSM:
    default:
      return "pvsm";
  }
}

//-----------------------------------------------------------------------------
void pqAutoSaveBehavior::updateConnections()
{
  if (this->ObservedStack)
  {
    this->ObservedStack->disconnect(this);
  }
  pqActiveObjects::instance().disconnect(this);

  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool autoSaveEnabled = settings->value(::AUTOSAVE_KEY, false).toBool();

  if (autoSaveEnabled)
  {
    this->ObservedStack = pqApplicationCore::instance()->getUndoStack();
    if (this->ObservedStack)
    {
      QObject::connect(
        this->ObservedStack, &pqUndoStack::canUndoChanged, this, [this](bool canUndo) {
          this->HasChanges = canUndo;
          if (canUndo)
          {
            this->saveState();
          }
        });
    }

    QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::viewUpdated, this, [this]() {
      if (this->HasChanges)
      {
        this->HasChanges = false;
        this->saveState();
      }
    });

    QDir directory = this->getStateDirectory();
    if (!directory.exists())
    {
      QString warningTitle = tr("AutoSave directory was not found.");
      QString warningText =
        QString("%1 (%2). %3")
          .arg(tr("Cannot find the autosave directory"))
          .arg(directory.absolutePath())
          .arg(tr("AutoSave will not work until an existing directory is provided."));
      qWarning() << warningText;
    }
  }
}

//-----------------------------------------------------------------------------
QString pqAutoSaveBehavior::getLastStatePath()
{
  return pqAutoSaveBehavior::getStatePath(false);
}

//-----------------------------------------------------------------------------
QString pqAutoSaveBehavior::getBakStatePath()
{
  return pqAutoSaveBehavior::getStatePath(true);
}

//-----------------------------------------------------------------------------
pqAutoSaveBehavior::StateFormat pqAutoSaveBehavior::getStateFormat()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  int value = settings->value(::AUTOSAVE_FORMAT_KEY, static_cast<int>(StateFormat::PVSM)).toInt();

  return StateFormat(value);
}

//-----------------------------------------------------------------------------
QString pqAutoSaveBehavior::getStatePath(bool bak)
{
  QDir directory = pqAutoSaveBehavior::getStateDirectory();
  QString rootFilePath = directory.filePath(::AutoSaveBaseName());
  QString state = QString("%1.%2").arg(rootFilePath);
  if (bak)
  {
    state = state.arg("bak.%1");
  }

  StateFormat format = pqAutoSaveBehavior::getStateFormat();
  QString stateExtension = pqAutoSaveBehavior::formatToExtension(format);
  state = state.arg(stateExtension);
  return state;
}

//-----------------------------------------------------------------------------
QDir pqAutoSaveBehavior::getStateDirectory()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QString dirPath = settings->value(::AUTOSAVE_DIR_KEY, "").toString();

  if (dirPath.isEmpty())
  {
    dirPath = pqCoreUtilities::getParaViewApplicationDataDirectory();
  }

  return QDir(dirPath);
}

//-----------------------------------------------------------------------------
void pqAutoSaveBehavior::saveState()
{
  if (!pqActiveObjects::instance().proxyManager())
  {
    return;
  }

  QDir directory = pqAutoSaveBehavior::getStateDirectory();
  if (!directory.exists())
  {
    return;
  }

  QString rootFilePath = directory.filePath(::AutoSaveBaseName());
  QString bakState = pqAutoSaveBehavior::getBakStatePath();
  QString newState = pqAutoSaveBehavior::getLastStatePath();

  if (QFileInfo::exists(newState))
  {
    QFile::remove(bakState);
    QFile::copy(newState, bakState);
  }

  StateFormat format = pqAutoSaveBehavior::getStateFormat();
  if (format == StateFormat::Python)
  {
    vtkSmartPointer<vtkSMProxy> options;
    options.TakeReference(pqSaveStateReaction::createPythonStateOptions(false));
    pqSaveStateReaction::savePythonState(newState, options, vtkPVSession::CLIENT);
  }
  else
  {
    pqSaveStateReaction::saveState(newState);
  }
}
