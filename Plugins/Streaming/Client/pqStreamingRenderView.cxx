
#include "pqStreamingRenderView.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <vtkSMProxy.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMStreamingViewProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>

#include <pqOutputPort.h>
#include <pqPipelineSource.h>
#include <pqRepresentation.h>
#include <pqServer.h>
#include <pqApplicationCore.h>
#include <pqSettings.h>

pqStreamingRenderView::pqStreamingRenderView(
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
pqStreamingRenderView::~pqStreamingRenderView()
{
}

//-----------------------------------------------------------------------------
vtkSMStreamingViewProxy* pqStreamingRenderView::getStreamingViewProxy() const
{
  return vtkSMStreamingViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
vtkSMRenderViewProxy* pqStreamingRenderView::getRenderViewProxy() const
{
  return vtkSMRenderViewProxy::SafeDownCast(
    this->getStreamingViewProxy()->GetRootView());
}

