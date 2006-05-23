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
#include <QDockWidget>
#include <QFocusFrame>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QScrollBar>
#include <QSet>
#include <QSignalMapper>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QtDebug>

/// Returns the name of an object.  In some cases, assigns a name as a convenience.  Also strips-out problematic characters (such as '/').
static const QString InternalGetName(QObject& Object)
{
  if(Object.objectName().isEmpty())
    {
    if(qobject_cast<QStackedWidget*>(&Object))
      {
      if(qobject_cast<QTabWidget*>(Object.parent()))
        {
        Object.setObjectName("qt_stacked_widget");
        }
      }
    }
    
  QString result = Object.objectName();
  result.replace("/", "|");
  
  return result;
}

/// Returns true iff the given object needs to have its name validated
static bool ValidateName(QObject& Object)
{
  const QString class_name = Object.metaObject()->className();

  // If you add a button to a QToolBar using addAction(), the button is unnamed,
  // so we have to skip all QToolButtons, which is unsatisfying, since it may
  // cause false negatives for QToolButtons that were manually added without a name.
  if(qobject_cast<QToolButton*>(&Object) && qobject_cast<QToolBar*>(Object.parent()))
    {
    return false;
    }
  
  if(QAction* const action = qobject_cast<QAction*>(&Object))
    {
    // Skip separators ...
    if(action->isSeparator())
      return false;

    // Skip menu implementation details ...
    if(QMenu* const menu = qobject_cast<QMenu*>(Object.parent()))
      {
      if(menu->menuAction() == action)
        {
        return false;
        }
      }

    // Skip dock widget implementation details ...
    if(QDockWidget* const dock = qobject_cast<QDockWidget*>(Object.parent()))
      {
      if(dock->toggleViewAction() == action)
        {
        return false;
        }
      }

    // Skip toolbar implementation details ...
    if(QToolBar* const toolbar = qobject_cast<QToolBar*>(Object.parent()))
      {
      if(toolbar->toggleViewAction() == action)
        {
        return false;
        }
      }
    }
  
  // Skip menubar implementation details ...
  if(qobject_cast<QMenuBar*>(Object.parent()))
    {
    if(QWidget* const widget = qobject_cast<QWidget*>(&Object))
      {
      if(widget->windowType() == Qt::Popup)
        {
        return false;
        }
      }
    }
  
  // Skip tab widget implementation details ...
  if(qobject_cast<QTabWidget*>(Object.parent()))
    {
      if(qobject_cast<QTabBar*>(&Object) || qobject_cast<QToolButton*>(&Object))
        {
        return false;
        }
    }
  
  // Skip QVTKWidget implementation details ...
  if(QString(Object.parent()->metaObject()->className()) == "QVTKWidget")
    {
    return false;
    }
  
  // Skip line edit implementation details ...
  if(qobject_cast<QLineEdit*>(Object.parent()))
    {
    return false;
    }

  // Skip objects that aren't normally used for user input ...  
  if(
    qobject_cast<QAbstractItemDelegate*>(&Object)
    || qobject_cast<QAbstractItemModel*>(&Object)
    || qobject_cast<QFocusFrame*>(&Object)
    || qobject_cast<QHeaderView*>(&Object)
    || qobject_cast<QItemSelectionModel*>(&Object)
    || qobject_cast<QLabel*>(&Object)
    || qobject_cast<QLayout*>(&Object)
    || qobject_cast<QScrollBar*>(&Object)
    || qobject_cast<QSignalMapper*>(&Object))
    {
    return false;
    }

  // Skip some weird dock widget implementation details ...
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
    
  // Skip ParaView stuff ...
  if(class_name == "pqPropertyManager")
    {
    return false;
    }
  
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// pqObjectNaming

bool pqObjectNaming::Validate(QObject& Parent)
{
  const QString name = InternalGetName(Parent);
  if(name.isEmpty())
    {
    qWarning() << "Unnamed root widget\n";
    return false;
    }
    
  return pqObjectNaming::Validate(Parent, "/" + name);
}

bool pqObjectNaming::Validate(QObject& Parent, const QString& Path)
{
//  qDebug() << Path << ": " << &Parent;

  bool result = true;
  
  QSet<QString> names;
  const QObjectList children = Parent.children();
  for(int i = 0; i != children.size(); ++i)
    {
    QObject* child = children[i];

    // Only validate names for objects that require them ...
    if(ValidateName(*child))
      {
      const QString name = InternalGetName(*child);
      if(name.isEmpty())
        {
        qWarning() << Path << " - unnamed child object: " << child;
        result = false;
        }
      else
        {
        if(names.contains(name))
          {
          qWarning() << Path << " - duplicate child object name [" << name << "]: " << child;
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
  QString name = InternalGetName(Object);
  if(name.isEmpty())
    {
    qCritical() << "Cannot record event for unnamed object " << &Object;
    return QString();
    }
  
  for(QObject* p = Object.parent(); p; p = p->parent())
    {
    const QString parent_name = InternalGetName(*p);
    
    if(parent_name.isEmpty())
      {
      qCritical() << "Cannot record event for incompletely-named object " << name << " " << &Object << " with parent " << p;
      return QString();
      }
      
    name = parent_name + "/" + name;
    
    if(!p->parent() && !QApplication::topLevelWidgets().contains(qobject_cast<QWidget*>(p)))
      {
      qCritical() << "Object " << p << " is not a top-level widget";
      return QString();
      }
    }

  return name;
}

QObject* pqObjectNaming::GetObject(const QString& Name)
{
  QObject* result = 0;
  const QStringList names = Name.split("/");
  if(names.empty())
    return 0;
  
  const QWidgetList top_level_widgets = QApplication::topLevelWidgets();
  for(int i = 0; i != top_level_widgets.size(); ++i)
    {
    QObject* object = top_level_widgets[i];
    const QString name = InternalGetName(*object);
    
    if(name == names[0])
      {
      result = object;
      break;
      }
    }
    
  for(int j = 1; j < names.size(); ++j)
    {
    const QObjectList& children = result ? result->children() : QObjectList();
    
    result = 0;
    for(int k = 0; k != children.size(); ++k)
      {
      QObject* child = children[k];
      const QString name = InternalGetName(*child);
      
      if(name == names[j])
        {
        result = child;
        break;
        }
      }
    }
    
  if(result)
    return result;
  
  qCritical() << "Couldn't find object " << Name;
  return 0;
}
