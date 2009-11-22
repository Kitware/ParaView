#include <QApplication>
#include "pqPVApplicationCore.h"
#include "myMainWindow.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  pqPVApplicationCore appCore(argc, argv);
  myMainWindow window;
  window.show();
  return app.exec();
}
