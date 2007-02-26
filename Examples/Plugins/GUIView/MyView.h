
#ifndef _MyView_h
#define _MyView_h

#include "pqGenericViewModule.h"
#include <QMap>
class QLabel;

/// a simple view that shows a QLabel with the display's name in the view
class MyView : public pqGenericViewModule
{
  Q_OBJECT
public:
    /// constructor takes a bunch of init stuff and must have this signature to 
    /// satisfy pqGenericViewModule
  MyView(const QString& viewtypemodule, 
         const QString& group, 
         const QString& name, 
         vtkSMAbstractViewModuleProxy* viewmodule, 
         pqServer* server, 
         QObject* p);
  ~MyView();

  /// don't support save images
  bool saveImage(int, int, const QString& ) { return false; }
  vtkImageData* captureImage(int) { return NULL; }

  /// return the QWidget to give to ParaView's view manager
  QWidget* getWidget();

  /// returns whether this view can display the given source
  bool canDisplaySource(pqPipelineSource* source) const;

protected slots:
  /// helper slots to create labels
  void onDisplayAdded(pqDisplay*);
  void onDisplayRemoved(pqDisplay*);

protected:

  QWidget* MyWidget;
  QMap<pqDisplay*, QLabel*> Labels;

};

#endif // _MyView_h

