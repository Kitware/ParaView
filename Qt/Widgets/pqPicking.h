

#ifndef _pqPicking_h
#define _pqPicking_h

#include <QObject>
#include <vtkType.h>
class vtkObject;
class vtkCommand;
class vtkRenderWindowInteractor;
class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkSMDisplayProxy;
class vtkSMPointLabelDisplayProxy;
class vtkUnstructuredGrid;

/// class to do picking
class pqPicking : public QObject
{
  Q_OBJECT
public:
  pqPicking(vtkSMRenderModuleProxy* rm, QObject* p);
  ~pqPicking();

public slots:

  /// compute selection when given the render window interactor
  void computeSelection(vtkObject* style, unsigned long, void*, void*, vtkCommand*);
  
  /// pick a cell on a current source proxy with given screen coordinates
  void computeSelection(vtkRenderWindowInteractor* iren, int X, int Y);

signals:
  /// emit selection changed, proxy and dataset is given
  void selectionChanged(vtkSMSourceProxy* p, vtkUnstructuredGrid* selections);

private:
  vtkSMRenderModuleProxy* RenderModule;
  vtkSMSourceProxy* PickFilter;
  vtkSMDisplayProxy* PickDisplay;
  vtkSMPointLabelDisplayProxy* PickRetriever;

};

#endif

