
#ifndef _pqMultiView_h
#define _pqMultiView_h

#include <QFrame>
#include <QList>

/// class to manage locations of multiple view widgets
class pqMultiView : public QFrame
{
  Q_OBJECT
public:
  typedef QList<int> Index;
  
  pqMultiView(QWidget* parent = NULL);
  ~pqMultiView();

  /// replace a widget at index with a new widget, returns the old one
  virtual QWidget* replaceView(Index index, QWidget* widget);

  /// split a location in a direction
  /// a dummy widget is inserted and an index for it is returned
  virtual Index splitView(Index index, Qt::Orientation orientation);
  
  /// remove a widget inserted by replaceWidget or splitView
  virtual void removeView(QWidget* widget);

  /// get the index of the widget in its parent's layout
  Index indexOf(QWidget*) const;
  
  /// get the widget from an index
  QWidget* widgetOfIndex(Index index);

};


#endif //_pqMultiView_h

