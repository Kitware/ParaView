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

#include <QAction>
#include <QHeaderView>
#include <QLayout>
#include <QSet>
#include <QSignalMapper>
#include <QStandardItemModel>
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
  if(qobject_cast<QStandardItemModel*>(&Object))
    {
    return false;
    }

  // Skip some weird docking implementation widgets ...
  if(
    class_name == "QDockSeparator"
    || class_name == "QDockWidgetSeparator"
    || class_name == "QDockWidgetTitleButton"
    || class_name == "QWidgetResizeHandler")
    {
    return false;
    }

  // Skip some weird toolbar implementation widgets ...
  if(
    class_name == "QToolBarHandle"
    || class_name == "QToolBarWidgetAction")
    {
    return false;
    }  
  
  return true;
}

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
        qWarning() << Path << " - unnamed child widget: " << child << "\n";
        result = false;
        }
      else
        {
        if(names.contains(name))
          {
          qWarning() << Path << " - duplicate child widget name [" << name << "]: " << child << "\n";
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
