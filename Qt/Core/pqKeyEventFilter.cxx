// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqKeyEventFilter.h"

#include <QEvent>
#include <QKeyEvent>

namespace
{
QList<int> AcceptList = { Qt::Key_Enter, Qt::Key_Return };
QList<int> RejectList = { Qt::Key_Escape };
QList<int> MotionList = { Qt::Key_Home, Qt::Key_End, Qt::Key_Left, Qt::Key_Up, Qt::Key_Right,
  Qt::Key_Down, Qt::Key_PageUp, Qt::Key_PageDown };
};

//-----------------------------------------------------------------------------
pqKeyEventFilter::pqKeyEventFilter(QObject* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
pqKeyEventFilter::~pqKeyEventFilter() = default;

//-----------------------------------------------------------------------------
void pqKeyEventFilter::forwardTypes(QObject* monitored, QList<int> types)
{
  if (!this->ForwardTypes.keys().contains(monitored))
  {
    this->filter(monitored);
    this->ForwardTypes[monitored] = QList<int>();
  }

  this->ForwardTypes[monitored] << types;
}

//-----------------------------------------------------------------------------
void pqKeyEventFilter::forwardType(QObject* monitored, int type)
{
  this->forwardTypes(monitored, QList<int>() << type);
}

//-----------------------------------------------------------------------------
void pqKeyEventFilter::filter(QObject* monitored)
{
  if (!this->Monitored.contains(monitored))
  {
    this->Monitored << monitored;
    monitored->installEventFilter(this);
  }
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::shouldHandle(QObject* monitored, int type)
{
  return !(this->ForwardTypes.contains(monitored) && this->ForwardTypes[monitored].contains(type));
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::isAcceptType(int key)
{
  return ::AcceptList.contains(key);
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::isRejectType(int key)
{
  return ::RejectList.contains(key);
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::isTextUpdateType(QChar key)
{
  return key.isLetterOrNumber() || key.isSpace();
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::isMotionType(int key)
{
  return ::MotionList.contains(key);
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::isFocusType(int key)
{
  return key == Qt::Key_Tab || key == Qt::Key_Backtab;
}

//-----------------------------------------------------------------------------
bool pqKeyEventFilter::eventFilter(QObject* obj, QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    int key = keyEvent->key();

    if (this->shouldHandle(obj, Reject) && this->isRejectType(key))
    {
      Q_EMIT this->rejected();
      return true;
    }
    if (this->shouldHandle(obj, Accept) && this->isAcceptType(key))
    {
      Q_EMIT this->accepted(keyEvent->modifiers() & Qt::ShiftModifier);
      return true;
    }
    if (this->shouldHandle(obj, Motion) && this->isMotionType(key))
    {
      Q_EMIT this->motion(key);
      return true;
    }
    if (this->shouldHandle(obj, Focus) && this->isFocusType(key))
    {
      Q_EMIT this->focusChanged();
      return true;
    }
    QString text = keyEvent->text();
    QChar keyChar = text.isEmpty() ? QChar() : text.front();
    if (this->shouldHandle(obj, TextInput) && this->isTextUpdateType(keyChar) &&
      (keyEvent->modifiers() == Qt::NoModifier))
    {
      Q_EMIT this->textChanged(key);
      return true;
    }
  }

  return Superclass::eventFilter(obj, event);
}
