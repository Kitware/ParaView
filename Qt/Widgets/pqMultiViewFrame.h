
#ifndef _pqMultiViewFrame_h
#define _pqMultiViewFrame_h

#include <QWidget>
#include "ui_pqMultiViewFrameMenu.h"

class pqMultiViewFrame : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(bool autoHide READ autoHide WRITE setAutoHide)
  Q_PROPERTY(bool active WRITE setActive READ active)
  Q_PROPERTY(QColor color WRITE setColor READ color)
public:
  pqMultiViewFrame(QWidget* parent = NULL);
  ~pqMultiViewFrame();

  bool autoHide() const;
  void setAutoHide(bool);

  void setMainWidget(QWidget*);
  QWidget* mainWidget();
  
  bool active() const;
  QColor color() const;

public slots:
  void close();
  void maximize();
  void splitVertical();
  void splitHorizontal();
  void setColor(QColor);
  void setActive(bool);

signals:
  void activeChanged(bool);
  void closePressed();
  void maximizePressed();
  void splitVerticalPressed();
  void splitHorizontalPressed();

protected:
  void paintEvent(QPaintEvent* e);

private:
  QWidget* MainWidget;
  bool AutoHide;
  bool Active;
  QColor Color;
  QWidget* Menu;
  Ui::MultiViewFrameMenu MenuUi;
};

#endif //_pqMultiViewFrame_h

