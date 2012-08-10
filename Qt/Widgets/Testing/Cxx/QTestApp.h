
#ifndef QTestApp_h
#define QTestApp_h

#include <QApplication>
#include <QVector>
#include <QByteArray>

class QTestApp
{
public:
  QTestApp(int _argc, char** _argv);
  ~QTestApp();

  static int exec();

  static void messageHandler(QtMsgType type, const char *msg);

  static void delay(int ms);

  static bool simulateEvent(QWidget* w, QEvent* e);

  static void keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyClick(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);
 
  static void mouseDown(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                        Qt::KeyboardModifiers mod, int ms);
  
  static void mouseUp(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                      Qt::KeyboardModifiers mod, int ms);
  
  static void mouseMove(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                        Qt::KeyboardModifiers mod, int ms);

  static void mouseClick(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                         Qt::KeyboardModifiers mod, int ms);
  
  static void mouseDClick(QWidget* w, QPoint pos, Qt::MouseButton btn, 
                         Qt::KeyboardModifiers mod, int ms);

private:
  QApplication* App;
  static int Error;
  QList<QByteArray> Argv;
  QVector<char*> Argvp;
  int Argc;
};

#endif

