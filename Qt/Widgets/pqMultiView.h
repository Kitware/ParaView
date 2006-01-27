
#ifndef _pqMultiView_h
#define _pqMultiView_h

#include "QtWidgetsExport.h"
#include <QFrame>
#include <QList>

class QSplitter;

/// class to manage locations of multiple view widgets
class QTWIDGETS_EXPORT pqMultiView : public QFrame
{
  Q_OBJECT
public:
  typedef QList<int> Index;
  
  pqMultiView(QWidget* parent = NULL);
  virtual ~pqMultiView();

  /// \brief
  ///   Resets the multi-view to its original state.
  /// \param removed Used to return all the removed widgets.
  /// \param newWidget The new main widget for the reset multi-view.
  ///   If no widget is passed in, the default frame will be used.
  virtual void reset(QList<QWidget*> &removed, QWidget *newWidget=0);

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

private:
  void cleanSplitter(QSplitter *splitter, QList<QWidget*> &removed);
};


#endif //_pqMultiView_h

