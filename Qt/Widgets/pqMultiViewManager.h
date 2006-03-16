

#ifndef _pqMultiViewManager_h
#define _pqMultiViewManager_h

class pqMultiViewFrame;
class QSplitter;
class QWidget;
class vtkPVXMLElement;

#include "QtWidgetsExport.h"
#include "pqMultiView.h"

/// multi-view manager
class QTWIDGETS_EXPORT pqMultiViewManager : public pqMultiView
{
  Q_OBJECT
public:
  pqMultiViewManager(QWidget* parent=NULL);
  virtual ~pqMultiViewManager();

  /// \brief
  ///   Resets the multi-view to its original state.
  /// \param removed Used to return all the removed widgets.
  /// \param newWidget Unused. The multi-view manager provides its own widget.
  virtual void reset(QList<QWidget*> &removed, QWidget *newWidget=0);

  void saveState(vtkPVXMLElement *root);
  void loadState(vtkPVXMLElement *root);

signals:
  /// signal for new frame added
  void frameAdded(pqMultiViewFrame*);
  /// signal for frame removed
  void frameRemoved(pqMultiViewFrame*);

public slots:
  void removeWidget(QWidget *widget);
  void splitWidgetHorizontal(QWidget *widget);
  void splitWidgetVertical(QWidget *widget);

protected slots:
  void maximizeWidget(QWidget*);
  void restoreWidget(QWidget*);

protected:

  bool eventFilter(QObject*, QEvent* e);
  void splitWidget(QWidget*, Qt::Orientation);

  void setup(pqMultiViewFrame*);
  void cleanup(pqMultiViewFrame*);

private:
  void saveSplitter(vtkPVXMLElement *element, QSplitter *splitter, int index);
  void restoreSplitter(QWidget *widget, vtkPVXMLElement *element);

};

#endif // _pqMultiViewManager_h

