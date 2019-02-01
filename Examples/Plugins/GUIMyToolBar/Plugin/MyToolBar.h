#include <QToolBar>

class MyToolBar : public QToolBar
{
  Q_OBJECT;
  using Superclass = QToolBar;

public:
  MyToolBar(const QString& title, QWidget* parent = nullptr);
  MyToolBar(QWidget* parent = nullptr);
  ~MyToolBar() override;

private:
  Q_DISABLE_COPY(MyToolBar);
  void constructor();
};
