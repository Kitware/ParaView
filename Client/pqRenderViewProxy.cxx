
#include "pqRenderViewProxy.h"
#include "vtkSMRenderModuleProxy.h"

pqRenderViewProxy* pqRenderViewProxy::New()
{
  return new pqRenderViewProxy;
}

void pqRenderViewProxy::EventuallyRender()
{
  this->Render();
}

void pqRenderViewProxy::Render()
{
  // render LOD's
  //RenderModule->InteractiveRender();

  // do not render LOD's
  RenderModule->StillRender();
}

vtkRenderWindow* pqRenderViewProxy::GetRenderWindow()
{
  return RenderModule->GetRenderWindow();
}

void pqRenderViewProxy::SetRenderModule(vtkSMRenderModuleProxy* rm)
{
  RenderModule = rm;
}

