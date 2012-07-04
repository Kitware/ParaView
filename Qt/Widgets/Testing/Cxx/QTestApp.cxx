
#include "QTestApp.h"

#include <stdio.h>

#include <QTimer>
#include <QWidget>
#include <QKeyEvent>
#include <QMouseEvent>

int QTestApp::Error = 0;

QTestApp::QTestApp(int _argc, char** _argv)
{
  qInstallMsgHandler(QTestApp::messageHandler);
  
  // CMake generated driver removes argv[0], 
  // so let's put a dummy back in
  this->Argv.append("qTestApp");
  for(int i=0; i<_argc; i++)
    {
    this->Argv.append(_argv[i]);
    }
  for(int j=0; j<this->Argv.size(); j++)
    {
    this->Argvp.append(this->Argv[j].data());
    }
  this->Argc = this->Argvp.size();
  App = new QApplication(this->Argc, this->Argvp.data());
}
  
QTestApp::~QTestApp()
{
  delete App;
  qInstallMsgHandler(0);
}

int QTestApp::exec()
{
  if(QCoreApplication::arguments().contains("--exit"))
    {
    QTimer::singleShot(100, QApplication::instance(), 
                       SLOT(quit()));
    }
  return Error + QApplication::exec();
}

void QTestApp::messageHandler(QtMsgType type, const char *msg)
{
  switch(type)
  {
  case QtDebugMsg:
    fprintf(stderr, "Debug: %s\n", msg);
    break;
  case QtWarningMsg:
    fprintf(stderr, "Warning: %s\n", msg);
    Error++;
    break;
  case QtCriticalMsg:
    fprintf(stderr, "Critical: %s\n", msg);
    Error++;
    break;
  case QtFatalMsg:
    fprintf(stderr, "Fatal: %s\n", msg);
    abort();
  }
}

void QTestApp::delay(int ms)
{
  if(ms > 0)
  {
    QTimer::singleShot(ms, QApplication::instance(), SLOT(quit()));
    QApplication::exec();
  }
}

bool QTestApp::simulateEvent(QWidget* w, QEvent* e)
{
  bool status = QApplication::sendEvent(w, e);
  QApplication::processEvents();
  return status;
}

void QTestApp::keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if(!w)
    return;
  delay(ms);
  QString text;
  char off = 'a';
  if(mod & Qt::ShiftModifier)
    off = 'A';
  if(key >= Qt::Key_A && key <= Qt::Key_Z)
    {
    text.append(QChar::fromAscii(key - Qt::Key_A + off));
    }
  QKeyEvent e(QEvent::KeyRelease, key, mod, text);
  if(!simulateEvent(w, &e))
    {
    qWarning("keyUp not handled\n");
    }
}

void QTestApp::keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  if(!w)
    return;
  delay(ms);
  QString text;
  char off = 'a';
  if(mod & Qt::ShiftModifier)
    off = 'A';
  if(key >= Qt::Key_A && key <= Qt::Key_Z)
    {
    text.append(QChar::fromAscii(key - Qt::Key_A + off));
    }
  QKeyEvent e(QEvent::KeyPress, key, mod, text);
  if(!simulateEvent(w, &e))
    {
    qWarning("keyDown not handled\n");
    }
}

void QTestApp::keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  keyDown(w, key, mod, 0);
  keyUp(w, key, mod, 0);
}

void QTestApp::mouseDown(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                        Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonPress, pos, btn, btn, mod);
  if(!simulateEvent(w, &e))
    {
    qWarning("mouseDown not handled\n");
    }
}
  
void QTestApp::mouseUp(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                      Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonRelease, pos, btn, btn, mod);
  if(!simulateEvent(w, &e))
    {
    qWarning("mouseUp not handled\n");
    }
}
  
void QTestApp::mouseMove(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                        Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseMove, pos, btn, btn, mod);
  if(!simulateEvent(w, &e))
    {
    qWarning("mouseMove not handled\n");
    }
}

void QTestApp::mouseClick(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                         Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  mouseDown(w, pos, btn, mod, 0);
  mouseUp(w, pos, btn, mod, 0);
}

void QTestApp::mouseDClick(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                         Qt::KeyboardModifiers mod, int ms)
{
  delay(ms);
  QMouseEvent e(QEvent::MouseButtonDblClick, pos, btn, btn, mod);
  if(!simulateEvent(w, &e))
    {
    qWarning("mouseMove not handled\n");
    }
}

