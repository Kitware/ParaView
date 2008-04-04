/*=========================================================================

   Program: ParaView
   Module:    pqBasicWidgetEventPlayer.cxx

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

#include "pqBasicWidgetEventPlayer.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QWidget>
#include <QtDebug>

pqBasicWidgetEventPlayer::pqBasicWidgetEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqBasicWidgetEventPlayer::playEvent(QObject* Object, 
         const QString& Command, const QString& Arguments, 
         bool& /*Error*/)
{
  QWidget* widget = qobject_cast<QWidget*>(Object);
  if(widget)
    {
    if(Command == "contextMenu")
      {
      QPoint pt(widget->x(), widget->y());
      QPoint globalPt = widget->mapToGlobal(pt);
      QContextMenuEvent e(QContextMenuEvent::Other, pt, globalPt);
      QCoreApplication::sendEvent(widget, &e);
      return true;
      }
    else if(Command == "key")
      {
      QKeyEvent kd(QEvent::KeyPress, Arguments.toInt(), Qt::NoModifier);
      QKeyEvent ku(QEvent::KeyRelease, Arguments.toInt(), Qt::NoModifier);
      QCoreApplication::sendEvent(widget, &kd);
      QCoreApplication::sendEvent(widget, &ku);
      return true;
      }
    else if(Command == "keyEvent")
      {
      QStringList data = Arguments.split(':');
      QKeyEvent ke(static_cast<QEvent::Type>(data[0].toInt()),
                   data[1].toInt(),
                   static_cast<Qt::KeyboardModifiers>(data[2].toInt()),
                   data[3],
                   !!data[4].toInt(),
                   data[5].toInt());
      QCoreApplication::sendEvent(widget, &ke);
      return true;
      }
    else if(Command.startsWith("mouse"))
      {
      QStringList args = Arguments.split(',');
      if(args.size() == 5)
        {
        Qt::MouseButton button = static_cast<Qt::MouseButton>(args[0].toInt());
        Qt::MouseButtons buttons = static_cast<Qt::MouseButton>(args[1].toInt());
        Qt::KeyboardModifiers keym = static_cast<Qt::KeyboardModifier>(args[2].toInt());
        int x = args[3].toInt();
        int y = args[4].toInt();
        QPoint pt(x,y);
        QEvent::Type type = QEvent::MouseButtonPress;
        type = Command == "mouseMove" ? QEvent::MouseMove : type;
        type = Command == "mouseRelease" ? QEvent::MouseButtonRelease : type;
        type = Command == "mouseDblClick" ? QEvent::MouseButtonDblClick : type;
        QMouseEvent e(type, pt, button, buttons, keym);
        QCoreApplication::sendEvent(widget, &e);
        return true;
        }
      }
    else
      {
      return false;
      }
    }
  return false;
}

