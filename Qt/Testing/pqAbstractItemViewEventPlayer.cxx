/*=========================================================================

   Program: ParaView
   Module:    pqAbstractItemViewEventPlayer.cxx

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

#include "pqAbstractItemViewEventPlayer.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QtDebug>
#include <QList>
#include <QListWidget>

#include "pqEventDispatcher.h"

/// Converts a string representation of a model index into the real thing
static QModelIndex OldGetIndex(QAbstractItemView& View, const QString& Name)
{
    QStringList rows = Name.split('/', QString::SkipEmptyParts);
    QString column;
    
    if(rows.size())
      {
      column = rows.back().split('|').at(1);
      rows.back() = rows.back().split('|').at(0);
      }
    
    QModelIndex index;
    for(int i = 0; i != rows.size(); ++i)
      {
      index = View.model()->index(rows[i].toInt(), column.toInt(), index);
      }
      
    return index;
}

static QModelIndex GetIndexByItemName(QAbstractItemView& View, const QString& Name)
{
  QListWidget* const listWidget = qobject_cast<QListWidget*>(&View);

  QModelIndex index;
  if(!listWidget)
    {
    return index;
    }
  QList<QListWidgetItem *> findResult = listWidget->findItems(Name,Qt::MatchExactly);
  if(findResult.count() > 0)
    {
    // in theory more than one item could match? Only return the index for
    // the first instance.
    index = View.model()->index(listWidget->row(findResult.first()), 0, index);
    }
    
  return index;
}

static QModelIndex GetIndex(QAbstractItemView* View, const QString& Name)
{
  QStringList idxs = Name.split('/', QString::SkipEmptyParts);
  
  QModelIndex index;
  for(int i = 0; i != idxs.size(); ++i)
    {
    QStringList rowCol = idxs[i].split(':');
    index = View->model()->index(rowCol[0].toInt(), rowCol[1].toInt(), index);
    }
    
  return index;
}

///////////////////////////////////////////////////////////////////////////////
// pqAbstractItemViewEventPlayer

pqAbstractItemViewEventPlayer::pqAbstractItemViewEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqAbstractItemViewEventPlayer::playEvent(QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  QAbstractItemView* const object = qobject_cast<QAbstractItemView*>(Object);
  if(!object)
    {
    return false;
    }
    
  if(Command == "currentChanged")  // left to support old recordings
    {
    const QModelIndex index = OldGetIndex(*object, Arguments);
    if(!index.isValid())
      return false;
      
    object->setCurrentIndex(index);
    return true;
    }
  else if(Command == "currentChangedbyItemName")
    {
    const QModelIndex index = GetIndexByItemName(*object, Arguments);
    if(!index.isValid())
      return false;
      
    object->setCurrentIndex(index);
    return true;
    }
  else if(Command == "keyEvent")
    {
    QStringList data = Arguments.split(',');
    if(data.size() == 6)
      {
      QKeyEvent ke(static_cast<QEvent::Type>(data[0].toInt()),
                   data[1].toInt(),
                   static_cast<Qt::KeyboardModifiers>(data[2].toInt()),
                   data[3],
                   !!data[4].toInt(),
                   data[5].toInt());
      QCoreApplication::sendEvent(object, &ke);
      return true;
      }
    }
  else if(Command.startsWith("mouse"))
    {
    QStringList args = Arguments.split(',');
    if(args.size() == 6)
      {
      Qt::MouseButton button = static_cast<Qt::MouseButton>(args[0].toInt());
      Qt::MouseButtons buttons = static_cast<Qt::MouseButton>(args[1].toInt());
      Qt::KeyboardModifiers keym = static_cast<Qt::KeyboardModifier>(args[2].toInt());
      int x = args[3].toInt();
      int y = args[4].toInt();
      QPoint pt;
      QHeaderView* header = qobject_cast<QHeaderView*>(object);
      if(header)
        {
        int idx = args[5].toInt();
        if(header->orientation() == Qt::Horizontal)
          {
          pt = QPoint(header->sectionPosition(idx)+4, 4);
          }
        else
          {
          pt = QPoint(4, header->sectionPosition(idx)+4);
          }
        }
      else
        {
        QModelIndex idx = GetIndex(object, args[5]);
        object->scrollTo(idx);
        QRect r = object->visualRect(idx);
        pt = r.topLeft() + QPoint(x,y);
        }
      QEvent::Type type = QEvent::MouseButtonPress;
      type = Command == "mouseMove" ? QEvent::MouseMove : type;
      type = Command == "mouseRelease" ? QEvent::MouseButtonRelease : type;
      type = Command == "mouseDblClick" ? QEvent::MouseButtonDblClick : type;
      QMouseEvent e(type, pt, button, buttons, keym);
      QCoreApplication::sendEvent(object->viewport(), &e);
      return true;
      }
    }
    
  qCritical() << "Unknown abstract item command: " << Command << "\n";
  Error = true;
  return true;
}
