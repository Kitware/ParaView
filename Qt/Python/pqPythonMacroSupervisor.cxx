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

#include <QAction>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <QSet>
#include <QMenu>
#include <QVariant>
#include <QWidget>

class pqPythonMacroSupervisor::pqInternal {
public:
  QList<QPointer<QWidget> > ActionContainers;
  QMap<QString, QAction*> ActionMap;
};

//----------------------------------------------------------------------------
pqPythonMacroSupervisor::pqPythonMacroSupervisor(QObject* p) : QObject(p)
{
  this->Internal = new pqInternal;
}

//----------------------------------------------------------------------------
pqPythonMacroSupervisor::~pqPythonMacroSupervisor()
{
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
  if (this->Internal->ActionMap.contains(fileName))
    {
    return this->Internal->ActionMap[fileName];
    }
  return NULL;
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::addWidgetForMacros(QWidget* widget)
{
  if (widget && !this->Internal->ActionContainers.contains(widget))
    {
    addPlaceHolderIfNeeded(widget);
    this->Internal->ActionContainers.append(widget);
    }
  this->resetActions();
}

//----------------------------------------------------------------------------
QMap<QString, QString> pqPythonMacroSupervisor::getStoredMacros()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QStringList fileNames = settings->value("PythonMacros/FileNames").toStringList();
  QStringList macroNames = settings->value("PythonMacros/Names").toStringList();

  if (fileNames.size() != macroNames.size())
    {
    qWarning() << "Lookup of macro filenames is corrupted.  Stored macros will be reset.";
    settings->remove("PythonMacros");
    fileNames.clear();
    macroNames.clear();
    }

  QMap<QString, QString> macros;
  for (int i = 0; i < macroNames.size(); ++i)
    {
    macros.insert(fileNames[i], macroNames[i]);
    }
  return macros;
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::storeMacro(const QString& macroName, const QString& filename)
{
  QMap<QString, QString> macros = pqPythonMacroSupervisor::getStoredMacros();
  macros[filename] = macroName;
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("PythonMacros/FileNames", QStringList(macros.keys()));
  settings->setValue("PythonMacros/Names", QStringList(macros.values()));
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::removeStoredMacro(const QString& filename)
{
  QMap<QString, QString> macros = pqPythonMacroSupervisor::getStoredMacros();
  macros.remove(filename);
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("PythonMacros/FileNames", QStringList(macros.keys()));
  settings->setValue("PythonMacros/Names", QStringList(macros.values()));
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::resetActions()
{
  foreach (QAction* action, this->Internal->ActionMap.values())
    {
    removeActionFromWidgets(action, this->Internal->ActionContainers);
    delete action;
    }
  this->Internal->ActionMap.clear();

  // Key is filename, value is macroname
  QMap<QString, QString> macros = pqPythonMacroSupervisor::getStoredMacros();
  QMap<QString, QString>::const_iterator itr;
  for (itr = macros.constBegin(); itr != macros.constEnd(); ++itr)
    {
    this->addMacro(itr.value(), itr.key());
    }
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
  
  action = new QAction(macroName, this);
  action->setData(fileName);
  this->Internal->ActionMap.insert(fileName, action);
  this->connect(action, SIGNAL(triggered()), SLOT(onMacroTriggered()));

  // Add action to widgets
  addActionToWidgets(action, this->Internal->ActionContainers);
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::removeMacro(const QString& fileName)
{
  QAction* action = this->getMacro(fileName);
  if (!action)
    {
    return;
    }

  removeActionFromWidgets(action, this->Internal->ActionContainers);
  this->Internal->ActionMap.remove(fileName);
  delete action;
}

//----------------------------------------------------------------------------
void pqPythonMacroSupervisor::onMacroTriggered()
{
  QObject* action = this->sender();
  QMap<QString, QAction*>::const_iterator itr = this->Internal->ActionMap.constBegin();
  for ( ; itr != this->Internal->ActionMap.constEnd(); ++itr)
    {
    if (itr.value() == action)
      {
      QString filename = itr.key();
      emit this->executeScriptRequested(filename);
      }
    }
}
