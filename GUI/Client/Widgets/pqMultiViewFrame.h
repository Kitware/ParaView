
#ifndef _pqMultiViewFrame_h
#define _pqMultiViewFrame_h

#include <QWidget>
#include "QtWidgetsExport.h"
#include "ui_pqMultiViewFrameMenu.h"

/// a holder for a widget in a multiview
class QTWIDGETS_EXPORT pqMultiViewFrame : public QWidget, public Ui::MultiViewFrameMenu
{
  Q_OBJECT
  Q_PROPERTY(bool menuAutoHide READ menuAutoHide WRITE setMenuAutoHide)
  Q_PROPERTY(bool active WRITE setActive READ active)
  Q_PROPERTY(QColor borderColor WRITE setBorderColor READ borderColor)
public:
  pqMultiViewFrame(QWidget* parent = NULL);
  ~pqMultiViewFrame();

  /// sets the window title in the title bar and the widget.
  void setTitle(const QString& title);

  /// whether the menu is auto hidden
  bool menuAutoHide() const;
  /// whether the menu is auto hidden
  void setMenuAutoHide(bool);

  /// set the main widget for this holder
  void setMainWidget(QWidget*);
  /// get the main widget for this holder
  QWidget* mainWidget();
  
  /// get whether active, if active, a border is drawn
  bool active() const;
  /// get the color of the border
  QColor borderColor() const;

public slots:

  /// close this frame, emits closePressed() so receiver does the actual remove
  void close();
  /// maximize this frame, emits closePressed() so receiver does the actual maximize
  void maximize();
  /// split this frame vertically, emits splitVerticalPressed so receiver does the actual split
  void splitVertical();
  /// split this frame horizontally, emits splitVerticalPressed so receiver does the actual split
  void splitHorizontal();
  /// sets the border color
  void setBorderColor(QColor);
  /// sets whether this frame is active.  if active, a border is drawn
  void setActive(bool);

signals:
  /// signal active state changed
  void activeChanged(bool);
  /// signal close pressed
  void closePressed();
  /// signal maximize pressed
  void maximizePressed();
  /// signal split vertical pressed
  void splitVerticalPressed();
  /// signal split horizontal pressed
  void splitHorizontalPressed();

protected:
  void paintEvent(QPaintEvent* e);

private:
  QWidget* MainWidget;
  bool AutoHide;
  bool Active;
  QColor Color;
  QWidget* Menu;
};

#endif //_pqMultiViewFrame_h

