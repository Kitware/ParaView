
#include "pqAdaptiveRenderView.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include "QVTKWidget.h"
#include <vtkSMProxy.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMAdaptiveViewProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>

#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>
#include <pqApplicationCore.h>
#include <pqSettings.h>

pqAdaptiveRenderView::pqAdaptiveRenderView(
  const QString& viewType, 
  const QString& group, 
  const QString& name, 
  vtkSMViewProxy* viewProxy, 
  pqServer* server, 
  QObject* p)
  : pqRenderView(viewType, group, name, viewProxy, server, p)
{

}

//-----------------------------------------------------------------------------
pqAdaptiveRenderView::~pqAdaptiveRenderView()
{
}

//-----------------------------------------------------------------------------
vtkSMAdaptiveViewProxy* pqAdaptiveRenderView::getAdaptiveViewProxy() const
{
  return vtkSMAdaptiveViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqAdaptiveRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(
    this->getAdaptiveViewProxy()->GetRootView());
}

//-----------------------------------------------------------------------------
QWidget* pqAdaptiveRenderView::createWidget() 
{
  QVTKWidget* vtkwidget = dynamic_cast<QVTKWidget*>(this->Superclass::createWidget());
  if (vtkwidget)
    {
    vtkwidget->setAutomaticImageCacheEnabled(false);
    }
  return vtkwidget;
}
