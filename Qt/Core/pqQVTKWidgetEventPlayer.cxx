// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqQVTKWidgetEventPlayer.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QRegularExpression>
#include <QtDebug>

#include "QVTKOpenGLNativeWidget.h"
#include "QVTKOpenGLStereoWidget.h"
#include "pqEventDispatcher.h"
#include "pqQVTKWidget.h"

pqQVTKWidgetEventPlayer::pqQVTKWidgetEventPlayer(QObject* p)
  : pqWidgetEventPlayer(p)
{
}

bool pqQVTKWidgetEventPlayer::playEvent(
  QObject* Object, const QString& Command, const QString& Arguments, bool& Error)
{
  QVTKOpenGLStereoWidget* qvtkStereoWidget = qobject_cast<QVTKOpenGLStereoWidget*>(Object);
  QVTKOpenGLNativeWidget* qvtkNativeWidget = qobject_cast<QVTKOpenGLNativeWidget*>(Object);
  pqQVTKWidget* qvtkWidget = qobject_cast<pqQVTKWidget*>(Object);
  if (qvtkStereoWidget || qvtkNativeWidget || qvtkWidget)
  {
    if (Command == "mousePress" || Command == "mouseRelease" || Command == "mouseMove" ||
      Command == "mouseDblClick")
    {
      QRegularExpression mouseRegExp("\\(([^,]*),([^,]*),([^,]),([^,]),([^,]*)\\)");
      QRegularExpressionMatch match = mouseRegExp.match(Arguments);
      if (match.hasMatch())
      {
        QWidget* widget = qobject_cast<QWidget*>(Object);
        QVariant v = match.captured(1);
        int x = static_cast<int>(v.toDouble() * widget->size().width());
        v = match.captured(2);
        int y = static_cast<int>(v.toDouble() * widget->size().height());
        v = match.captured(3);
        Qt::MouseButton button = static_cast<Qt::MouseButton>(v.toInt());
        v = match.captured(4);
        Qt::MouseButtons buttons = static_cast<Qt::MouseButton>(v.toInt());
        v = match.captured(5);
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

        if (qvtkStereoWidget != nullptr)
        {
          // Due to QTBUG-61836 (see QVTKOpenGLStereoWidget::testingEvent()), events should
          // be propagated back to the internal QVTKOpenGLWindow when being fired
          // explicitly on the widget instance. We have to use a custom event
          // callback in this case to ensure that events are passed to the window.
          qApp->notify(qvtkStereoWidget->embeddedOpenGLWindow(), &e);
        }

        if (qvtkNativeWidget != nullptr)
        {
          qApp->notify(qvtkNativeWidget, &e);
        }

        if (qvtkWidget != nullptr)
        {
          qvtkWidget->notifyQApplication(&e);
        }
      }
      return true;
    }
  }
  return this->Superclass::playEvent(Object, Command, Arguments, Error);
}
