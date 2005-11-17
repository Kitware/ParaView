

#ifndef _pqMultiViewManager_h
#define _pqMultiViewManager_h

class pqMultiViewFrame;

#include "pqMultiView.h"

/// multi-view manager
class pqMultiViewManager : public pqMultiView
{
  Q_OBJECT
public:
  pqMultiViewManager(QWidget* parent=NULL);
  ~pqMultiViewManager();

signals:
  /// signal for new frame added
  void frameAdded(pqMultiViewFrame*);
  /// signal for frame removed
  void frameRemoved(pqMultiViewFrame*);

protected slots:
  void removeWidget(QWidget*);
  void splitWidgetHorizontal(QWidget*);
  void splitWidgetVertical(QWidget*);
  void maximizeWidget(QWidget*);
  void restoreWidget(QWidget*);

protected:

  bool eventFilter(QObject*, QEvent* e);
  void splitWidget(QWidget*, Qt::Orientation);

  void setup(pqMultiViewFrame*);
  void cleanup(pqMultiViewFrame*);

};

#endif // _pqMultiViewManager_h

