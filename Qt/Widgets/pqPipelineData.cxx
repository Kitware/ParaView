
#include "pqPipelineData.h"
#include "pqServer.h"

#include "vtkSMProxy.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxyManager.h"

pqPipelineData::pqPipelineData(pqServer* server)
  : QObject(server), CurrentProxy(NULL), Server(server)
{
}

pqPipelineData::~pqPipelineData()
{
}

void pqPipelineData::setCurrentProxy(vtkSMSourceProxy* proxy)
{
  this->CurrentProxy = proxy;
  emit this->currentProxyChanged(this->CurrentProxy);
}

vtkSMSourceProxy * pqPipelineData::currentProxy() const
{
  return this->CurrentProxy;
}

vtkSMProxy* pqPipelineData::newSMProxy(const char* groupname, const char* proxyname)
{
  vtkSMProxy* proxy = this->Server->GetProxyManager()->NewProxy(groupname, proxyname);
  proxy->UpdateVTKObjects();
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
  this->setCurrentProxy(source);
  return proxy;
}

void pqPipelineData::addInput(vtkSMSourceProxy* proxy, vtkSMSourceProxy* input)
{
  vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("Input"))->AddProxy(input);
  emit this->newPipelineObjectInput(proxy, input);
}


