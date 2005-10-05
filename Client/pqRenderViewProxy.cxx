
#include "pqRenderViewProxy.h"
#include "QVTKWidget.h"
#include "vtkRenderWindow.h"

pqRenderViewProxy* pqRenderViewProxy::New()
{
  return new pqRenderViewProxy;
}

// what is EventuallyRender for??  is that a Tk thing?
void pqRenderViewProxy::EventuallyRender()
{
  this->Render();
}

vtkRenderWindow* pqRenderViewProxy::GetRenderWindow()
{
  return mRenWin->GetRenderWindow();
}

void pqRenderViewProxy::Render()
{
  // bypass Qt and tell the VTK window to render
  // this may/may not work on the Mac where
  // Qt works with the composite window manager to render at the right time
  mRenWin->GetRenderWindow()->Render();
}

void pqRenderViewProxy::SetRenderWindow(QVTKWidget* win)
{
  mRenWin = win;
}

