
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
  // Listen for changes to application settings
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QObject::connect(settings, SIGNAL(modified()), this, SLOT(restoreSettings()));

  // Set streaming settings
  this->restoreSettings();
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

//-----------------------------------------------------------------------------
void pqStreamingRenderView::restoreSettings()
{
  vtkSMProxy * prx = this->getProxy();
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->beginGroup("streamingView");
  QVariant val = settings->value("StreamedPasses", 16);
  vtkSMPropertyHelper(prx, "StreamedPasses").Set(val.toInt());
//  val = settings->value("EnableStreamMessages", false);
//  vtkSMPropertyHelper(prx, "EnableStreamMessages").Set(val.toBool());
  val = settings->value("UseCulling", true);
  vtkSMPropertyHelper(prx, "UseCulling").Set(val.toBool());
  val = settings->value("UseViewOrdering", true);
  vtkSMPropertyHelper(prx, "UseViewOrdering").Set(val.toBool());
  val = settings->value("PieceCacheLimit", 16);
  vtkSMPropertyHelper(prx, "PieceCacheLimit").Set(val.toInt());
  val = settings->value("PieceRenderCutoff", -1);
  vtkSMPropertyHelper(prx, "PieceRenderCutoff").Set(val.toInt());


  vtkSMProxy * helper = vtkSMStreamingViewProxy::GetHelperProxy();
  val = settings->value("EnableStreamMessages", false);
  vtkSMPropertyHelper(helper, "EnableStreamMessages").Set(val.toBool());
  
  settings->endGroup();
}

