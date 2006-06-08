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

/** Returns the name of an object.  If the object doesn't have an explicit name,
assigns a name as a convenience.  Also replaces problematic characters such as '/'.
*/
static const QString InternalGetName(QObject& Object)
{
  QString result = Object.objectName();

  if(result.isEmpty())
    {
    QObjectList siblings;
    if(Object.parent())
      {
      siblings = Object.parent()->children();
      }
    else
      {
      QWidgetList widgets = QApplication::topLevelWidgets();
      for(int i = 0; i != widgets.size(); ++i)
        {
        siblings.push_back(widgets[i]);
        }
      }
      
    const QString type = Object.metaObject()->className();
    
    int index = 0;
    for(int i = 0; i != siblings.size(); ++i)
      {
      QObject* test = siblings[i];
      if(test == &Object)
        {
        break;
        }
      else if(
        type == test->metaObject()->className()
        && test->objectName().isEmpty())
        {
        ++index;
        }
      }
      
    if(QWidget* const widget = qobject_cast<QWidget*>(&Object))
      {
      result += QString::number(widget->isVisible());
      }
    result += type + QString::number(index);
    }

  result.replace("/", "|");
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

const QString pqObjectNaming::DumpHierarchy()
{
  QString result;
  
  const QWidgetList widgets = QApplication::topLevelWidgets();
  for(int i = 0; i != widgets.size(); ++i)
    {
    result += DumpHierarchy(*widgets[i]);
    }
  
  return result;
}

const QString pqObjectNaming::DumpHierarchy(QObject& Object)
{
  QString result;

  result += GetName(Object) + "\n";
  
  const QObjectList children = Object.children();
  for(int i = 0; i != children.size(); ++i)
    {
    result += DumpHierarchy(*children[i]);
    }
  
  return result;
}
