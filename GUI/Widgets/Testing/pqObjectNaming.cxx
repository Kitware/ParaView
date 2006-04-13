/*=========================================================================

   Program:   ParaQ
   Module:    pqObjectNaming.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqObjectNaming.h"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLayout>
#include <QMenu>
#include <QScrollBar>
#include <QSet>
#include <QSignalMapper>
#include <QToolBar>
#include <QtDebug>

/// Returns true iff the given object needs to have its name validated
static bool ValidateName(QObject& Object)
{
  const QString class_name = Object.metaObject()->className();
  
  // Skip separators ...
  if(QAction* const action = qobject_cast<QAction*>(&Object))
    {
    if(action->isSeparator())
      return false;
    }
  
  // Skip layouts ...
  if(qobject_cast<QLayout*>(&Object))
    {
    return false;
    }

  // Skip mappers ...
  if(qobject_cast<QSignalMapper*>(&Object))
    {
    return false;
    }

  // Skip headers ...
  if(qobject_cast<QHeaderView*>(&Object))
    {
    return false;
    }

  // Skip MVC models ...
  if(qobject_cast<QAbstractItemModel*>(&Object))
    {
    return false;
    }

  // Skip MVC implementation objects ...
  if(qobject_cast<QAbstractItemDelegate*>(&Object) || qobject_cast<QItemSelectionModel*>(&Object))
    {
    return false;
    }

  // Skip scrollbars ...
  if(qobject_cast<QScrollBar*>(&Object))
    {
    return false;
    }

  // Skip menu implementation details ...
  if(QMenu* const menu = qobject_cast<QMenu*>(Object.parent()))
    {
    if(menu->menuAction() == &Object)
      {
      return false;
      }
    }

  // Skip toolbar implementation details ...
  if(QToolBar* const toolbar = qobject_cast<QToolBar*>(Object.parent()))
    {
    if(toolbar->toggleViewAction() == &Object)
      {
      return false;
      }
    }

  // Skip some weird dockwindow implementation details ...
  if(
    class_name == "QDockSeparator"
    || class_name == "QDockWidgetSeparator"
    || class_name == "QDockWidgetTitleButton"
    || class_name == "QWidgetResizeHandler")
    {
    return false;
    }

  // Skip some weird toolbar implementation details ...
  if(
    class_name == "QToolBarHandle"
    || class_name == "QToolBarWidgetAction")
    {
    return false;
    }  
  
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// pqObjectNaming

bool pqObjectNaming::Validate(QObject& Parent)
{
  const QString name = Parent.objectName();
  if(name.isEmpty())
    {
    qWarning() << "Unnamed root widget\n";
    return false;
    }
    
  return pqObjectNaming::Validate(Parent, "/" + name);
}

bool pqObjectNaming::Validate(QObject& Parent, const QString& Path)
{
//  qDebug() << Path << ": " << &Parent << "\n";

  bool result = true;
  
  QSet<QString> names;
  const QObjectList children = Parent.children();
  for(int i = 0; i != children.size(); ++i)
    {
    QObject* child = children[i];

    // Only validate names for objects that require them ...
    if(ValidateName(*child))
      {
      const QString name = child->objectName();
      if(name.isEmpty())
        {
        qWarning() << Path << " - unnamed child object: " << child << "\n";
        result = false;
        }
      else
        {
        if(names.contains(name))
          {
          qWarning() << Path << " - duplicate child object name [" << name << "]: " << child << "\n";
          result = false;
          }
          
        names.insert(name);
        }
      
      if(!pqObjectNaming::Validate(*child, Path + "/" + name))
        {
        result = false;
        }
      }
    }
  
  return result;
}

const QString pqObjectNaming::GetName(QObject& Object)
{
  QString name = Object.objectName();
  if(name.isEmpty())
    {
    qCritical() << "Cannot record event for unnamed object " << &Object;
    return QString();
    }
  
  for(QObject* p = Object.parent(); p; p = p->parent())
    {
    if(p->objectName().isEmpty())
      {
      qCritical() << "Cannot record event for incompletely-named object " << name << " " << &Object << " with parent " << p;
      return QString();
      }
      
    name = p->objectName() + "/" + name;
    
    if(!p->parent() && !QApplication::topLevelWidgets().contains(qobject_cast<QWidget*>(p)))
      {
      qCritical() << "Object " << p << " is not a top-level widget\n";
      return QString();
      }
    }

  return name;
}


QObject* pqObjectNaming::GetObject(const QString& Name)
{
  /// Given a slash-delimited "path", lookup a Qt object hierarchically
  const QWidgetList top_level_widgets = QApplication::topLevelWidgets();
  for(int i = 0; i != top_level_widgets.size(); ++i)
    {
    QObject* object = top_level_widgets[i];
    const QStringList paths = Name.split("/");
    for(int i = 1; i < paths.size(); ++i) // Note - we're ignoring the top-level path, since it already represents the root node
      {
      if(object)
        object = object->findChild<QObject*>(paths[i]);
      }
      
    if(object)
      return object;
    }
  
  qCritical() << "Couldn't find object " << Name << "\n";
  return 0;
}
