
#ifndef QTestApp_h
#define QTestApp_h

#include <QApplication>
#include <QByteArray>
#include <QVector>

class QTestApp
{
public:
  QTestApp(int _argc, char** _argv);
  ~QTestApp();

  static int exec();

  static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

  static void delay(int ms);

  static bool simulateEvent(QWidget* w, QEvent* e);

  static QString keyToAscii(Qt::Key key, Qt::KeyboardModifiers mod);

  static void keyUp(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  static void keyDown(QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod, int ms);

  // Simulate a key clicked (Down then Up) to the given widget. If any delay is
  // given (delay > 0), the function will wait before sending the keyes to the
  // widget.
  static void keyClick(
    QWidget* w, Qt::Key key, Qt::KeyboardModifiers mod = Qt::NoModifier, int ms = -1);

  // Simulate each letter of the given text clicked to the widget. If any delay
  // is given (delay > 0), the function will delay before each key of the text.
  static void keyClicks(
    QWidget* w, const QString& text, Qt::KeyboardModifiers mod = Qt::NoModifier, int ms = -1);

  static void mouseDown(
    QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms);

  static void mouseUp(
    QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms);

  static void mouseMove(
    QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms);

  static void mouseClick(
    QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms);

  static void mouseDClick(
    QWidget* w, QPoint pos, Qt::MouseButton btn, Qt::KeyboardModifiers mod, int ms);

private:
  QApplication* App;
  static int Error;
  QList<QByteArray> Argv;
  QVector<char*> Argvp;
  int Argc;
};

#endif
