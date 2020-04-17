/*=========================================================================

   Program: ParaView
   Module:    pqFlatTreeViewEventTranslator.cxx

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
