// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFlatTreeViewEventTranslator.h"

#include "pqFlatTreeView.h"
#include <QEvent>
#include <QKeyEvent>

static QString toIndexStr(QModelIndex index)
{
  QString result;
  for (QModelIndex i = index; i.isValid(); i = i.parent())
  {
    result = "/" + QString("%1:%2").arg(i.row()).arg(i.column()) + result;
  }
  return result;
}

pqFlatTreeViewEventTranslator::pqFlatTreeViewEventTranslator(QObject* p)
  : pqWidgetEventTranslator(p)
{
}

bool pqFlatTreeViewEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& Error)
{
  pqFlatTreeView* object = qobject_cast<pqFlatTreeView*>(Object);
  if (!object)
  {
    // mouse events go to the viewport widget
    object = qobject_cast<pqFlatTreeView*>(Object->parent());
  }
  if (!object)
    return false;

  switch (Event->type())
  {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    {
      QKeyEvent* ke = static_cast<QKeyEvent*>(Event);
      QString data = QString("%1,%2,%3,%4,%5,%6")
                       .arg(ke->type())
                       .arg(ke->key())
                       .arg(static_cast<int>(ke->modifiers()))
                       .arg(ke->text())
                       .arg(ke->isAutoRepeat())
                       .arg(ke->count());
      Q_EMIT recordEvent(object, "keyEvent", data);
      return true;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    {
      if (Object == object)
      {
        return false;
      }
      QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(Event);
      if (Event->type() != QEvent::MouseButtonRelease)
      {
        this->LastPos = mouseEvent->pos();
      }
      QString idxStr;
      QPoint relPt = QPoint(0, 0);
      QModelIndex idx = object->getIndexCellAt(mouseEvent->pos());
      idxStr = toIndexStr(idx);
      QRect r;
      object->getVisibleRect(idx, r);
      relPt = mouseEvent->pos() - r.topLeft();

      QString info = QString("%1,%2,%3,%4,%5,%6")
                       .arg(mouseEvent->button())
                       .arg(mouseEvent->buttons())
                       .arg(mouseEvent->modifiers())
                       .arg(relPt.x())
                       .arg(relPt.y())
                       .arg(idxStr);
      if (Event->type() == QEvent::MouseButtonPress)
      {
        Q_EMIT recordEvent(object, "mousePress", info);
      }
      else if (Event->type() == QEvent::MouseButtonDblClick)
      {
        Q_EMIT recordEvent(object, "mouseDblClick", info);
      }
      else if (Event->type() == QEvent::MouseButtonRelease)
      {
        if (this->LastPos != mouseEvent->pos())
        {
          Q_EMIT recordEvent(object, "mouseMove", info);
        }
        Q_EMIT recordEvent(object, "mouseRelease", info);
      }
      return true;
    }
    default:
      break;
  }
  return this->Superclass::translateEvent(Object, Event, Error);
}
