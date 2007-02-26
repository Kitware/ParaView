
#ifndef _MyView_h
#define _MyView_h

#include "pqGenericViewModule.h"
#include <QMap>
class QLabel;

/// a simple view that displays a label for each display in the view
class MyView : public pqGenericViewModule
{
  Q_OBJECT
public:
  MyView(const QString& viewtypemodule, 
         const QString& group, 
         const QString& name, 
         vtkSMAbstractViewModuleProxy* viewmodule, 
         pqServer* server, 
         QObject* p);
  ~MyView();

  bool saveImage(int, int, const QString& ) { return false; }
  vtkImageData* captureImage(int) { return NULL; }

  QWidget* getWidget();

  bool canDisplaySource(pqPipelineSource* source) const;

protected slots:
  void onDisplayAdded(pqDisplay*);
  void onDisplayRemoved(pqDisplay*);

protected:

  QWidget* MyWidget;
  QMap<pqDisplay*, QLabel*> Labels;

};

#endif // _MyView_h

