// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFlatTreeViewEventPlayer.h"
#include "pqQtDeprecated.h"

#include <QApplication>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtDebug>

#include "pqEventDispatcher.h"
#include "pqFlatTreeView.h"

/// Converts a string representation of a model index into the real thing
static QModelIndex OldGetIndex(pqFlatTreeView& View, const QString& Name)
{
  QStringList rows = Name.split('/', PV_QT_SKIP_EMPTY_PARTS);
  QString column;

  if (!rows.empty())
  {
    column = rows.back().split('|').at(1);
    rows.back() = rows.back().split('|').at(0);
  }

  QModelIndex index;
  for (int i = 0; i < rows.size() - 1; ++i)
  {
    index = View.getModel()->index(rows[i].toInt(), 0, index);
  }

  if (!rows.empty())
  {
    index = View.getModel()->index(rows[rows.size() - 1].toInt(), column.toInt(), index);
  }

  return index;
}

static QModelIndex GetIndex(pqFlatTreeView* View, const QString& Name)
{
  QStringList idxs = Name.split('/', PV_QT_SKIP_EMPTY_PARTS);

  QModelIndex index;
  for (int i = 0; i != idxs.size(); ++i)
  {
    QStringList rowCol = idxs[i].split(':');
    index = View->getModel()->index(rowCol[0].toInt(), rowCol[1].toInt(), index);
  }

  return index;
}

///////////////////////////////////////////////////////////////////////////////
// pqFlatTreeViewEventPlayer

pqFlatTreeViewEventPlayer::pqFlatTreeViewEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqFlatTreeViewEventPlayer::playEvent(
  QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  pqFlatTreeView* object = qobject_cast<pqFlatTreeView*>(Object);
  if (!object)
  {
    // mouse events go to the viewport widget
    object = qobject_cast<pqFlatTreeView*>(Object->parent());
  }
  if (!object)
  {
    return false;
  }

  if (Command == "currentChanged") // left to support old recordings
  {
    const QModelIndex index = OldGetIndex(*object, Arguments);
    if (!index.isValid())
      return false;

    object->setCurrentIndex(index);
    return true;
  }
  else if (Command == "keyEvent")
  {
    QStringList data = Arguments.split(',');
    if (data.size() == 6)
    {
      QKeyEvent ke(static_cast<QEvent::Type>(data[0].toInt()), data[1].toInt(),
        static_cast<Qt::KeyboardModifiers>(data[2].toInt()), data[3], !!data[4].toInt(),
        data[5].toInt());
      qApp->notify(object, &ke);
      return true;
    }
  }
  else if (Command.startsWith("mouse"))
  {
    QStringList args = Arguments.split(',');
    if (args.size() == 6)
    {
      Qt::MouseButton button = static_cast<Qt::MouseButton>(args[0].toInt());
      Qt::MouseButtons buttons = static_cast<Qt::MouseButton>(args[1].toInt());
      Qt::KeyboardModifiers keym = static_cast<Qt::KeyboardModifier>(args[2].toInt());
      int x = args[3].toInt();
      int y = args[4].toInt();
      QPoint pt;
      QModelIndex idx = GetIndex(object, args[5]);
      QRect r;
      object->getVisibleRect(idx, r);
      pt = r.topLeft() + QPoint(x, y);
      QEvent::Type type = QEvent::MouseButtonPress;
      type = Command == "mouseMove" ? QEvent::MouseMove : type;
      type = Command == "mouseRelease" ? QEvent::MouseButtonRelease : type;
      type = Command == "mouseDblClick" ? QEvent::MouseButtonDblClick : type;
      QMouseEvent e(type, pt, button, buttons, keym);
      qApp->notify(object->viewport(), &e);
      pqEventDispatcher::processEventsAndWait(1);
      return true;
    }
  }

  if (!this->Superclass::playEvent(Object, Command, Arguments, Error))
  {
    qCritical() << "Unknown abstract item command: " << Command << "\n";
    Error = true;
  }
  return true;
}
