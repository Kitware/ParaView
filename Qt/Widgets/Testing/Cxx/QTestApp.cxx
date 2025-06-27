// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "QTestApp.h"

#include <cstdio>

#include <QDebug>
#include <QTimer>
#include <QWidget>
#include <QtTest/QTest>

int QTestApp::Error = 0;

QTestApp::QTestApp(int _argc, char** _argv)
{
  qInstallMessageHandler(QTestApp::messageHandler);

  // CMake generated driver removes argv[0],
  // so let's put a dummy back in
  this->Argv.append("qTestApp");
  for (int i = 0; i < _argc; i++)
  {
    this->Argv.append(_argv[i]);
  }
  for (int j = 0; j < this->Argv.size(); j++)
  {
    this->Argvp.append(this->Argv[j].data());
  }
  this->Argc = this->Argvp.size();
  App = new QApplication(this->Argc, this->Argvp.data());
}

QTestApp::~QTestApp()
{
  delete App;
  qInstallMessageHandler(nullptr);
}

int QTestApp::exec()
{
  if (QCoreApplication::arguments().contains("--exit"))
  {
    QTimer::singleShot(100, QApplication::instance(), SLOT(quit()));
  }
  return Error + QApplication::exec();
}

void QTestApp::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
  QByteArray localMsg = msg.toUtf8();
  switch (type)
  {
    case QtDebugMsg:
      fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.data(), context.file, context.line,
        context.function);
      break;
    case QtInfoMsg:
      fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.data(), context.file, context.line,
        context.function);
      break;
    case QtWarningMsg:
      fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.data(), context.file, context.line,
        context.function);
      break;
    case QtCriticalMsg:
      fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.data(), context.file, context.line,
        context.function);
      Error++;
      break;
    case QtFatalMsg:
      fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.data(), context.file, context.line,
        context.function);
      abort();
  }
}

void QTestApp::delay(int ms)
{
  if (ms > 0)
  {
    QTest::qWait(ms);
  }
}

bool QTestApp::simulateEvent(QWidget* w, QEvent* e)
{
  bool status = QApplication::sendEvent(w, e);
  QApplication::processEvents();
  return status;
}

QString QTestApp::keyToAscii(Qt::Key key, Qt::KeyboardModifiers mod)
{
  QString text;
  char off = 'a';
  if (mod & Qt::ShiftModifier)
    off = 'A';
  if (key >= Qt::Key_A && key <= Qt::Key_Z)
  {
    text.append(QLatin1Char(key - Qt::Key_A + off));
  }
  return text;
}

void QTestApp::keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::keyRelease(w, key, mod, ms);
}

void QTestApp::keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::keyPress(w, key, mod, ms);
}

void QTestApp::keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::keyClick(w, key, mod, ms);
}

void QTestApp::keyClicks(QWidget* w, const QString& text, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::keyClicks(w, text, mod, ms);
}

void QTestApp::mouseDown(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::mousePress(w, btn, mod, pos, ms);
}

void QTestApp::mouseUp(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::mouseRelease(w, btn, mod, pos, ms);
}

void QTestApp::mouseMove(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  Q_UNUSED(btn);
  Q_UNUSED(mod);
  QTest::mouseMove(w, pos, ms);
}

void QTestApp::mouseClick(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::mouseClick(w, btn, mod, pos, ms);
}

void QTestApp::mouseDClick(
  QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms)
{
  if (!w)
  {
    return;
  }
  QTest::mouseDClick(w, btn, mod, pos, ms);
}
