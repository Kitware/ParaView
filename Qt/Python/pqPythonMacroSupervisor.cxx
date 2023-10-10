// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPythonMacroSupervisor.h"
#include "pqApplicationCore.h"
#include "pqServer.h"

#include "pqCoreUtilities.h"
#include "pqPythonManager.h"
#include "vtksys/SystemTools.hxx"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#if defined(_WIN32) && !defined(__CYGWIN__)
const char ENV_PATH_SEP = ';';
#else
const char ENV_PATH_SEP = ':';
#endif

class pqPythonMacroSupervisor::pqInternal
{
public:
  // Container widget that have an RunMacro action context
  QList<QPointer<QWidget>> RunWidgetContainers;
  // List of action linked to widget/menuItem used to start a macro
  QMap<QString, QAction*> RunActionMap;

  // Container widget that have an EditMacro action context
  QList<QPointer<QWidget>> EditWidgetContainers;
  // List of action linked to widget/menuItem used to edit a macro
  QMap<QString, QAction*> EditActionMap;

  // Container widget that have an DeleteMacro action context
  QList<QPointer<QWidget>> DeleteWidgetContainers;
  // List of action linked to widget/menuItem used to delete a macro
  QMap<QString, QPointer<QAction>> DeleteActionMap;
};

//----------------------------------------------------------------------------
pqPythonMacroSupervisor::pqPythonMacroSupervisor(QObject* p)
  : QObject(p)
{
  this->Internal = new pqInternal;
  QObject::connect(pqApplicationCore::instance(), SIGNAL(updateMasterEnableState(bool)), this,
    SLOT(updateMacroList()));
}

//----------------------------------------------------------------------------
pqPythonMacroSupervisor::~pqPythonMacroSupervisor()
{
  delete this->Internal;
  this->Internal = nullptr;
}

// Util methods
//----------------------------------------------------------------------------
namespace
{
void addPlaceHolderIfNeeded(QWidget* widget)
{
  QMenu* menu = qobject_cast<QMenu*>(widget);
  if (menu && menu->isEmpty())
  {
    menu->addAction(QCoreApplication::translate("pqPythonMacroSupervisor", "empty"))->setEnabled(0);
  }
}
void removePlaceHolderIfNeeded(QWidget* widget)
{
  QMenu* menu = qobject_cast<QMenu*>(widget);
  if (menu && menu->actions().size() == 1)
  {
    QAction* act = menu->actions()[0];
    // It's a placeholder if it is disabled and has empty data().
    if (!act->isEnabled() && act->data().toString().length() == 0)
    {
      menu->removeAction(act);
      delete act;
    }
  }
}
void addActionToWidgets(QAction* action, QList<QPointer<QWidget>>& widgets)
{
  Q_FOREACH (QWidget* widget, widgets)
  {
    removePlaceHolderIfNeeded(widget);
    if (widget)
    {
      widget->addAction(action);
    }
  }
}
void removeActionFromWidgets(QAction* action, QList<QPointer<QWidget>>& widgets)
{
  Q_FOREACH (QWidget* widget, widgets)
  {
    if (widget)
    {
      widget->removeAction(action);
    }
    addPlaceHolderIfNeeded(widget);
  }
}
}

//----------------------------------------------------------------------------
QAction* pqPythonMacroSupervisor::getMacro(const QString& fileName)
{
  if (this->Internal->RunActionMap.contains(fileName))
  {
    return this->Internal->RunActionMap[fileName];
  }
  return nullptr;
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addWidgetForRunMacros(QWidget* widget)
{
  this->addWidgetForMacros(widget, 0);
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addWidgetForEditMacros(QWidget* widget)
{
  this->addWidgetForMacros(widget, 1);
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addWidgetForDeleteMacros(QWidget* widget)
{
  this->addWidgetForMacros(widget, 2);
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addWidgetForMacros(QWidget* widget, int actionType)
{
  QList<QPointer<QWidget>>* widgetContainers = nullptr;
  switch (actionType)
  {
    case 0: // run
      widgetContainers = &this->Internal->RunWidgetContainers;
      break;
    case 1: // edit
      widgetContainers = &this->Internal->EditWidgetContainers;
      break;
    case 2: // delete
      widgetContainers = &this->Internal->DeleteWidgetContainers;
      break;
  }

  if (widget && !widgetContainers->contains(widget))
  {
    addPlaceHolderIfNeeded(widget);
    widgetContainers->append(widget);
  }
  this->resetActions();
}
//----------------------------------------------------------------------------
QMap<QString, QString> pqPythonMacroSupervisor::getStoredMacros()
{
  QStringList fileNames = getMacrosFilePaths();

  QMap<QString, QString> macros;
  for (int i = 0; i < fileNames.size(); ++i)
  {
    macros.insert(fileNames[i], macroNameFromFileName(fileNames[i]));
  }
  return macros;
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::removeStoredMacro(const QString& fileName)
{
  pqPythonMacroSupervisor::hideFile(fileName);
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::hideFile(const QString& fileName)
{
  QFileInfo file = QFileInfo(fileName);
  QDir dir = file.absoluteDir();
  QString baseName = file.baseName();
  QString suffix = file.completeSuffix();

  int index = 1;
  QString pattern = "." + baseName + "%1%2." + suffix;
  QString newName = pattern.arg("").arg("");
  while (dir.exists(newName))
  {
    newName = pattern.arg("-").arg(QString::number(index));
    index++;
  }
  QString newFilePath = dir.absolutePath() + QDir::separator() + newName;
  QFile::rename(fileName, newFilePath);
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::updateMacroList()
{
  this->resetActions();
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::resetActions()
{
  // Delete RUN
  Q_FOREACH (QAction* action, this->Internal->RunActionMap.values())
  {
    removeActionFromWidgets(action, this->Internal->RunWidgetContainers);
    delete action;
  }
  this->Internal->RunActionMap.clear();
  // Delete EDIT
  Q_FOREACH (QAction* action, this->Internal->EditActionMap.values())
  {
    removeActionFromWidgets(action, this->Internal->EditWidgetContainers);
    delete action;
  }
  this->Internal->EditActionMap.clear();
  // Delete DELETE
  Q_FOREACH (QAction* action, this->Internal->DeleteActionMap.values())
  {
    removeActionFromWidgets(action, this->Internal->DeleteWidgetContainers);
    delete action;
  }
  this->Internal->DeleteActionMap.clear();

  // Key is filename, value is macroname
  QMap<QString, QString> macros = pqPythonMacroSupervisor::getStoredMacros();
  QMap<QString, QString>::const_iterator itr;
  for (itr = macros.constBegin(); itr != macros.constEnd(); ++itr)
  {
    this->addMacro(itr.value(), itr.key());
  }
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addMacro(const QString& fileName)
{
  this->addMacro(pqPythonMacroSupervisor::macroNameFromFileName(fileName), fileName);
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addMacro(const QString& macroName, const QString& fileName)
{
  QAction* action = this->getMacro(fileName);

  // If the macro already exists we just need to update it's name
  if (action)
  {
    action->setText(macroName);
    return;
  }

  // If we are master we allow macros
  bool enable = pqApplicationCore::instance()->getActiveServer()
    ? pqApplicationCore::instance()->getActiveServer()->isMaster()
    : true;

  // Run action
  QAction* runAction = new QAction(macroName, this);
  runAction->setData(fileName);
  runAction->setEnabled(enable);

  QString iconPath = pqPythonMacroSupervisor::iconPathFromFileName(fileName);
  if (!iconPath.isEmpty())
  {
    runAction->setIcon(QIcon(iconPath));
  }

  this->Internal->RunActionMap.insert(fileName, runAction);
  this->connect(runAction, SIGNAL(triggered()), SLOT(onMacroTriggered()));

  // Edit action
  QAction* editAction = new QAction(macroName, this);
  editAction->setData(fileName);
  editAction->setEnabled(enable);
  this->Internal->EditActionMap.insert(fileName, editAction);
  this->connect(editAction, SIGNAL(triggered()), SLOT(onEditMacroTriggered()));

  // Delete action
  QAction* deleteAction = new QAction(macroName, this);
  deleteAction->setData(fileName);
  deleteAction->setEnabled(enable);
  this->Internal->DeleteActionMap.insert(fileName, deleteAction);
  this->connect(deleteAction, SIGNAL(triggered()), SLOT(onDeleteMacroTriggered()));

  // Add action to widgets
  addActionToWidgets(runAction, this->Internal->RunWidgetContainers);
  addActionToWidgets(editAction, this->Internal->EditWidgetContainers);
  addActionToWidgets(deleteAction, this->Internal->DeleteWidgetContainers);

  Q_EMIT this->onAddedMacro();
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::removeMacro(const QString& fileName)
{
  QAction* action = this->getMacro(fileName);
  if (!action)
  {
    return;
  }

  removeActionFromWidgets(action, this->Internal->RunWidgetContainers);
  this->Internal->RunActionMap.remove(fileName);
  delete action;

  action = this->Internal->EditActionMap[fileName];
  removeActionFromWidgets(action, this->Internal->EditWidgetContainers);
  this->Internal->EditActionMap.remove(fileName);
  delete action;

  action = this->Internal->DeleteActionMap[fileName];
  removeActionFromWidgets(action, this->Internal->DeleteWidgetContainers);
  this->Internal->DeleteActionMap.remove(fileName);
  delete action;
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::onMacroTriggered()
{
  QObject* action = this->sender();
  // Get the filenames before executing the corresponding scripts. (See #18261)
  QList<QString> filenames;
  QMap<QString, QAction*>::const_iterator itr = this->Internal->RunActionMap.constBegin();
  for (; itr != this->Internal->RunActionMap.constEnd(); ++itr)
  {
    if (itr.value() == action)
    {
      filenames.append(itr.key());
    }
  }
  Q_FOREACH (QString filename, filenames)
  {
    Q_EMIT this->executeScriptRequested(filename);
  }
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::onDeleteMacroTriggered()
{
  QObject* action = this->sender();
  QList<QString> listOfMacroToDelete;
  QMap<QString, QPointer<QAction>>::const_iterator itr =
    this->Internal->DeleteActionMap.constBegin();
  for (; itr != this->Internal->DeleteActionMap.constEnd(); ++itr)
  {
    if (itr.value() == action)
    {
      const QString& filename = itr.key();
      listOfMacroToDelete.append(filename);
    }
  }

  for (const QString& filename : listOfMacroToDelete)
  {
    // look for related file with same basename (as icon file)
    QDir dir = QFileInfo(filename).absoluteDir();
    QStringList filter;
    filter << QFileInfo(filename).completeBaseName() + ".py";
    for (auto extension : pqPythonMacroSupervisor::getSupportedIconFormats())
    {
      filter << QFileInfo(filename).completeBaseName() + extension;
    }
    auto fileList = dir.entryInfoList(filter);

    for (auto file : fileList)
    {
      pqPythonMacroSupervisor::hideFile(file.absoluteFilePath());
    }

    this->removeMacro(filename);
  }
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::onEditMacroTriggered()
{
  QObject* action = this->sender();
  QMap<QString, QAction*>::const_iterator itr = this->Internal->EditActionMap.constBegin();
  for (; itr != this->Internal->EditActionMap.constEnd(); ++itr)
  {
    if (itr.value() == action)
    {
      const QString& filename = itr.key();
      Q_EMIT onEditMacro(filename);
    }
  }
}

//----------------------------------------------------------------------------
QString pqPythonMacroSupervisor::macroNameFromFileName(const QString& fileName)
{
  QString name = QFileInfo(fileName).fileName().replace(".py", "");
  if (!name.length())
  {
    name = "Unnamed macro";
  }
  return name;
}

//----------------------------------------------------------------------------
QString pqPythonMacroSupervisor::iconPathFromFileName(const QString& fileName)
{
  for (auto extension : pqPythonMacroSupervisor::getSupportedIconFormats())
  {
    QString iconPath = QFileInfo(fileName).absoluteFilePath().replace(".py", extension);
    if (QFileInfo::exists(iconPath))
    {
      return iconPath;
    }
  }

  return QString();
}

//----------------------------------------------------------------------------
QStringList pqPythonMacroSupervisor::getMacrosFilePaths()
{
  QStringList macroList;
  QDir dir;
  dir.setFilter(QDir::Files);

  QStringList macroDirs = pqCoreUtilities::findParaviewPaths(QString("Macros"), true, true);

  // add user defined macro paths from environment variable PV_MACRO_PATH
  const char* env = vtksys::SystemTools::GetEnv("PV_MACRO_PATH");
  if (env)
  {
    QStringList macroPathDirs = QString::fromUtf8(env).split(ENV_PATH_SEP);
    macroDirs << macroPathDirs;
  }

  Q_FOREACH (QString dirPath, macroDirs)
  {
    if (QFile::exists(dirPath))
    {
      dir.setPath(dirPath);
      for (int i = 0; i < dir.entryList().size(); ++i)
      {
        const QString filePath = dir.entryList().at(i);
        if (filePath.startsWith("."))
        {
          continue;
        }
        // allows only ".py"
        if (filePath.endsWith(".py"))
        {
          macroList.push_back(dirPath + QDir::separator() + filePath);
        }
      }
    }
  }

  return macroList;
}
