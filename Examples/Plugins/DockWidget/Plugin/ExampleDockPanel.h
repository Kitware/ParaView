
#include <QDockWidget>

class ExampleDockPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;

public:
  ExampleDockPanel(
    const QString& title, QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::WindowFlags())
    : Superclass(title, parent, flag)
  {
    this->constructor();
  }
  ExampleDockPanel(QWidget* parent = nullptr, Qt::WindowFlags flag = Qt::WindowFlags())
    : Superclass(parent, flag)
  {
    this->constructor();
  }

private:
  void constructor();
};
