/*=========================================================================

   Program: ParaView
   Module:  pqQVTKWidgetEventPlayer.cxx

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
#include "pqQVTKWidgetEventPlayer.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QRegExp>
#include <QtDebug>

#include "QVTKOpenGLNativeWidget.h"
#include "QVTKOpenGLWidget.h"
#include "QVTKOpenGLWindow.h"
#include "pqEventDispatcher.h"

pqQVTKWidgetEventPlayer::pqQVTKWidgetEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqQVTKWidgetEventPlayer::playEvent(
  QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  QVTKOpenGLWidget* qvtkWidget = qobject_cast<QVTKOpenGLWidget*>(Object);
  QVTKOpenGLNativeWidget* qvtkNativeWidget = qobject_cast<QVTKOpenGLNativeWidget*>(Object);
  if (qvtkWidget || qvtkNativeWidget)
  {
    if (Command == "mousePress" || Command == "mouseRelease" || Command == "mouseMove" ||
      Command == "mouseDblClick")
    {
      QRegExp mouseRegExp("\\(([^,]*),([^,]*),([^,]),([^,]),([^,]*)\\)");
      if (mouseRegExp.indexIn(Arguments) != -1)
      {
        QWidget* widget = qobject_cast<QWidget*>(Object);
        QVariant v = mouseRegExp.cap(1);
        int x = static_cast<int>(v.toDouble() * widget->size().width());
        v = mouseRegExp.cap(2);
        int y = static_cast<int>(v.toDouble() * widget->size().height());
        v = mouseRegExp.cap(3);
        Qt::MouseButton button = static_cast<Qt::MouseButton>(v.toInt());
        v = mouseRegExp.cap(4);
        Qt::MouseButtons buttons = static_cast<Qt::MouseButton>(v.toInt());
        v = mouseRegExp.cap(5);
        Qt::KeyboardModifiers keym = static_cast<Qt::KeyboardModifier>(v.toInt());

        QEvent::Type type = QEvent::None;
        if (Command == "mousePress")
        {
          type = QEvent::MouseButtonPress;
        }
        else if (Command == "mouseRelease")
        {
          type = QEvent::MouseButtonRelease;
        }
        else if (Command == "mouseMove")
        {
          type = QEvent::MouseMove;
        }
        else if (Command == "mouseDblClick")
        {
          type = QEvent::MouseButtonDblClick;
        }
        QMouseEvent e(type, QPoint(x, y), button, buttons, keym);

        if (qvtkWidget != nullptr)
        {
          // Due to QTBUG-61836 (see QVTKOpenGLWidget::testingEvent()), events should
          // be propagated back to the internal QVTKOpenGLWindow when being fired
          // explicitly on the widget instance. We have to use a custom event
          // callback in this case to ensure that events are passed to the window.
          qApp->notify(qvtkWidget->embeddedOpenGLWindow(), &e);
        }

        if (qvtkNativeWidget != nullptr)
        {
          qApp->notify(qvtkNativeWidget, &e);
        }
      }
      return true;
    }
  }
  return this->Superclass::playEvent(Object, Command, Arguments, Error);
}
