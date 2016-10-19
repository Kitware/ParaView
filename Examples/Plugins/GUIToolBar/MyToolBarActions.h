
#include <QActionGroup>

class MyToolBarActions : public QActionGroup
{
  Q_OBJECT
public:
  MyToolBarActions(QObject* p);
  ~MyToolBarActions();

public slots:
  void onAction();
};
