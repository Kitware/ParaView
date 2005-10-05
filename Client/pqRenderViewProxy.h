
#ifndef VTK_PC_RENDERVIEWPROXY_H
#define VTK_PC_RENDERVIEWPROXY_H

#include "vtkPVRenderViewProxy.h"

class QVTKWidget;

class pqRenderViewProxy : public vtkPVRenderViewProxy
{
  public:
    static pqRenderViewProxy* New();
    vtkTypeMacro(pqRenderViewProxy,vtkPVRenderViewProxy)
    
    virtual void Render();
    virtual void EventuallyRender();
    
    virtual vtkRenderWindow* GetRenderWindow();
    void SetRenderWindow(QVTKWidget* win);

  private:
    QVTKWidget* mRenWin;
};

#endif

