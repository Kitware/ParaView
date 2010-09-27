/*=========================================================================

   Program: ParaView
   Module:    pqPythonMacroSupervisor.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqPythonMacroSupervisor.h"
#include "pqApplicationCore.h"
#include "pqSettings.h"

#include "pqPythonManager.h"
#include "pqCoreUtilities.h"

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <QSet>
#include <QMenu>
#include <QVariant>
#include <QWidget>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QApplication>


class pqPythonMacroSupervisor::pqInternal {
public:
  // Container widget that have an RunMacro action context
  QList<QPointer<QWidget> > RunWidgetContainers;
  // List of action linked to widget/menuItem used to start a macro
  QMap<QString, QAction*> RunActionMap;

  // Container widget that have an EditMacro action context
  QList<QPointer<QWidget> > EditWidgetContainers;
  // List of action linked to widget/menuItem used to edit a macro
  QMap<QString, QAction*> EditActionMap;

  // Container widget that have an DeleteMacro action context
  QList<QPointer<QWidget> > DeleteWidgetContainers;
  // List of action linked to widget/menuItem used to delete a macro
  QMap<QString, QAction*> DeleteActionMap;
};

//----------------------------------------------------------------------------
pqPythonMacroSupervisor::pqPythonMacroSupervisor(QObject* p) : QObject(p)
{
  this->Internal = new pqInternal;
}

//----------------------------------------------------------------------------
pqPythonMacroSupervisor::~pqPythonMacroSupervisor()
{
  delete this->Internal;
  this->Internal = 0;
}

// Util methods
//----------------------------------------------------------------------------
namespace {
void addPlaceHolderIfNeeded(QWidget* widget)
{
  QMenu* menu = qobject_cast<QMenu*>(widget);
  if (menu && menu->isEmpty())
    {
    menu->addAction("empty")->setEnabled(0);
    }
}
void removePlaceHolderIfNeeded(QWidget* widget)
{
  QMenu* menu = qobject_cast<QMenu*>(widget);
  if (menu && menu->actions().size() == 1)
    {
    QAction* act = menu->actions()[0];
    // It's a placeholder if its name is 'empty' and has empty data().
    if (act->text() == "empty" && act->data().toString().length() == 0)
      {
      menu->removeAction(act);
      delete act;
      }
    }
}
void addActionToWidgets(QAction* action, QList<QPointer<QWidget> >& widgets)
{
  foreach (QWidget* widget, widgets)
    {
    removePlaceHolderIfNeeded(widget);
    if (widget)
      {
      widget->addAction(action);
      }
    }
}
void removeActionFromWidgets(QAction* action, QList<QPointer<QWidget> >& widgets)
{
  foreach (QWidget* widget, widgets)
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
  return NULL;
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
  QList<QPointer<QWidget> > *widgetContainers = NULL;
  switch(actionType)
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
//  pqSettings* settings = pqApplicationCore::instance()->settings();
//  QStringList fileNames = settings->value("PythonMacros/FileNames").toStringList();
//  QStringList macroNames = settings->value("PythonMacros/Names").toStringList();

//  if (fileNames.size() != macroNames.size())
//    {
//    qWarning() << "Lookup of macro filenames is corrupted.  Stored macros will be reset.";
//    settings->remove("PythonMacros");
//    fileNames.clear();
//    macroNames.clear();
//    }

  QStringList fileNames = getMacrosFilePaths();

  QMap<QString, QString> macros;
  for (int i = 0; i < fileNames.size(); ++i)
    {
    macros.insert(fileNames[i], macroNameFromFileName(fileNames[i]));
    }
  return macros;
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::removeStoredMacro(const QString& filename)
{
  QDir dir  = QFileInfo(filename).absoluteDir();
  QString baseName = ".";
  baseName += QFileInfo(filename).fileName().replace(".py", "");

  int index = 1;
  QString newName = baseName;
  newName += ".py";
  while(dir.exists(newName))
    {
    newName = baseName;
    newName.append("-").append(QString::number(index)).append(".py");
    index++;
    }
  QString newFilePath = dir.absolutePath() + QDir::separator() + newName;
  QFile::rename(filename, newFilePath);

//  QMap<QString, QString> macros = pqPythonMacroSupervisor::getStoredMacros();
//  macros.remove(filename);
//  pqSettings* settings = pqApplicationCore::instance()->settings();
//  settings->setValue("PythonMacros/FileNames", QStringList(macros.keys()));
//  settings->setValue("PythonMacros/Names", QStringList(macros.values()));


}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::resetActions()
{
  // Delete RUN
  foreach (QAction* action, this->Internal->RunActionMap.values())
    {
    removeActionFromWidgets(action, this->Internal->RunWidgetContainers);
    delete action;
    }
  this->Internal->RunActionMap.clear();
  // Delete EDIT
  foreach (QAction* action, this->Internal->EditActionMap.values())
    {
    removeActionFromWidgets(action, this->Internal->EditWidgetContainers);
    delete action;
    }
  this->Internal->EditActionMap.clear();
  // Delete DELETE
  foreach (QAction* action, this->Internal->DeleteActionMap.values())
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
  pqPythonMacroSupervisor::addMacro(pqPythonMacroSupervisor::macroNameFromFileName(fileName), fileName);
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
  
  // Run action
  QAction* runAction = new QAction(macroName, this);
  runAction->setData(fileName);
  this->Internal->RunActionMap.insert(fileName, runAction);
  this->connect(runAction, SIGNAL(triggered()), SLOT(onMacroTriggered()));

  // Edit action
  QAction* editAction = new QAction(macroName, this);
  editAction->setData(fileName);
  this->Internal->EditActionMap.insert(fileName, editAction);
  this->connect(editAction, SIGNAL(triggered()), SLOT(onEditMacroTriggered()));

  // Delete action
  QAction* deleteAction = new QAction(macroName, this);
  deleteAction->setData(fileName);
  this->Internal->DeleteActionMap.insert(fileName, deleteAction);
  this->connect(deleteAction, SIGNAL(triggered()), SLOT(onDeleteMacroTriggered()));

  // Add action to widgets
  addActionToWidgets(runAction, this->Internal->RunWidgetContainers);
  addActionToWidgets(editAction, this->Internal->EditWidgetContainers);
  addActionToWidgets(deleteAction, this->Internal->DeleteWidgetContainers);
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
  QMap<QString, QAction*>::const_iterator itr = this->Internal->RunActionMap.constBegin();
  for ( ; itr != this->Internal->RunActionMap.constEnd(); ++itr)
    {
    if (itr.value() == action)
      {
      QString filename = itr.key();
      emit this->executeScriptRequested(filename);
      }
    }
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::onDeleteMacroTriggered()
{
  QObject* action = this->sender();
  QMap<QString, QAction*>::const_iterator itr = this->Internal->DeleteActionMap.constBegin();
  for ( ; itr != this->Internal->DeleteActionMap.constEnd(); ++itr)
    {
    if (itr.value() == action)
      {
      QString filename = itr.key();
      pqPythonMacroSupervisor::removeStoredMacro(filename);
      pqPythonMacroSupervisor::removeMacro(filename);
      }
    }
}
//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::onEditMacroTriggered()
{
  QObject* action = this->sender();
  QMap<QString, QAction*>::const_iterator itr = this->Internal->EditActionMap.constBegin();
  for ( ; itr != this->Internal->EditActionMap.constEnd(); ++itr)
    {
    if (itr.value() == action)
      {
      QString filename = itr.key();
      emit onEditMacro(filename);
      }
    }
}
//----------------------------------------------------------------------------
QString pqPythonMacroSupervisor::macroNameFromFileName(const QString& filename)
{
  QString name = QFileInfo(filename).fileName().replace(".py", "");
  if (!name.length())
    {
    name = "Unnamed macro";
    }
  return name;
}

//----------------------------------------------------------------------------
QStringList pqPythonMacroSupervisor::getMacrosFilePaths()
{
  QStringList macroList;
  QDir dir;
  dir.setFilter(QDir::Files);

  foreach(QString dirPath, pqCoreUtilities::findParaviewPaths(QString("Macros"),
                                                              true, true))
    {
    dir.setPath(dirPath);
    foreach(QString filePath , dir.entryList())
      {
      if(filePath.startsWith("."))
        continue;
      macroList.push_back(dirPath + QDir::separator() + filePath);
      }
    }

  return macroList;
}
