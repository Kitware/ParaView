
#ifndef pqAnimationWidget_h
#define pqAnimationWidget_h

#include <QWidget>

class QGraphicsView;
class pqAnimationModel;

class pqAnimationWidget : public QWidget
{
  Q_OBJECT
public:
  pqAnimationWidget(QWidget* p = 0);
  ~pqAnimationWidget();

  pqAnimationModel* animationModel() const;

private:
  QGraphicsView* View;
  pqAnimationModel* Model;

};

#endif //pqAnimationWidget_h

