/*=========================================================================

   Program: ParaView
   Module:    pqPipelineBuilder.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/

#include "pqPipelineBuilder.h"

// Qt includes.
#include <QtDebug>
#include <QString>

// vtk includes
#include <vtksys/ios/sstream>

// Paraview Server Manager includes
#include "vtkProcessModule.h"
#include "vtkSMCompoundProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSourceProxy.h"

// ParaView includes
#include "pqApplicationCore.h"
#include "pqNameCount.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqUndoStack.h"

//-----------------------------------------------------------------------------
pqPipelineBuilder* pqPipelineBuilder::Instance = 0;
pqPipelineBuilder* pqPipelineBuilder::instance()
{
  return pqPipelineBuilder::Instance;
}

//-----------------------------------------------------------------------------
pqPipelineBuilder::pqPipelineBuilder(QObject* p/*=0*/):
  QObject(p)
{
  this->NameGenerator = new pqNameCount(); 
  this->UndoStack = 0;
  if (!pqPipelineBuilder::Instance)
    {
    pqPipelineBuilder::Instance = this;
    }
}

//-----------------------------------------------------------------------------
pqPipelineBuilder::~pqPipelineBuilder()
{
  delete this->NameGenerator;
  this->UndoStack = 0;
  if (pqPipelineBuilder::Instance == this)
    {
    pqPipelineBuilder::Instance = 0;
    }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqPipelineBuilder::createSource(const char* xmlgroup,
    const char* xmlname, pqServer* server, pqRenderModule* renModule)
{
  vtkSMProxy* proxy = this->createPipelineProxy(xmlgroup, xmlname,
    server, renModule);
  if (proxy)
    {
    return pqServerManagerModel::instance()->getPQSource(proxy);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::addConnection(pqPipelineSource* source, 
  pqPipelineSource* sink)
{
  if(!source || !sink)
    {
    qCritical() << "Cannot addConnection. source or sink missing.";
    return;
    }

  if (this->UndoStack)
    {
    this->UndoStack->BeginUndoSet(QString("Add Connection"));
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
    sink->getProxy()->GetProperty("Input"));
  if(inputProp)
    {
    // If the sink already has an input, the previous connection
    // needs to be broken if it doesn't support multiple inputs.
    if(!inputProp->GetMultipleInput() && inputProp->GetNumberOfProxies() > 0)
      {
      inputProp->RemoveAllProxies();
      }

    // Add the input to the proxy in the server manager.
    inputProp->AddProxy(source->getProxy());
    }
  else
    {
    qCritical() << "Failed to locate property Input on proxy:" 
      << source->getProxy()->GetXMLGroup() << ", " << source->getProxy()->GetXMLName();
    }

  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::removeConnection(pqPipelineSource* pqsource,
  pqPipelineSource* pqsink)
{
  vtkSMCompoundProxy *compoundProxy =
    vtkSMCompoundProxy::SafeDownCast(pqsource->getProxy());

  vtkSMProxy* source = pqsource->getProxy();
  vtkSMProxy* sink = pqsink->getProxy();

  if(compoundProxy)
    {
    // TODO: How to find the correct output proxy?
    source = 0;
    for(int i = compoundProxy->GetNumberOfProxies(); source == 0 && i > 0; i--)
      {
      source = vtkSMSourceProxy::SafeDownCast(compoundProxy->GetProxy(i-1));
      }
    }

  compoundProxy = vtkSMCompoundProxy::SafeDownCast(sink);
  if(compoundProxy)
    {
    // TODO: How to find the correct input proxy?
    sink = compoundProxy->GetMainProxy();
    }

  if(!source || !sink)
    {
    qCritical() << "Cannot removeConnection. source or sink missing.";
    return;
    }

  if (this->UndoStack)
    {
    this->UndoStack->BeginUndoSet(QString("Remove Connection"));
    }

  vtkSMInputProperty *inputProp = vtkSMInputProperty::SafeDownCast(
    sink->GetProperty("Input"));
  if(inputProp)
    {
    // Remove the input from the server manager.
    inputProp->RemoveProxy(source);
    }

  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineBuilder::createProxy(const char* xmlgroup, 
  const char* xmlname, const char* register_group, pqServer* server,
  bool is_undoable/*=true*/)
{
  if(!server)
    {
    qDebug() << "Cannot create proxy, no server specified.";
    return NULL;
    }

  if (!register_group)
    {
    qDebug() << "register_group cannot be null.";
    return NULL;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* proxy = NULL;
  if (xmlgroup)
    {
    proxy = pxm->NewProxy(xmlgroup, xmlname);
    }
  else
    {
    proxy = pxm->NewCompoundProxy(xmlname);
    }
  if (!proxy)
    {
    qCritical() << "Failed to create proxy: " 
      << (xmlgroup? xmlgroup: "") << "," << xmlname;
    return NULL;
    }
  proxy->SetConnectionID(server->GetConnectionID());

  if (this->UndoStack && is_undoable)
    {
    vtksys_ios::ostringstream label;
    label << "Create " << xmlname;
    this->UndoStack->BeginUndoSet(QString(label.str().c_str()));
    }

  vtksys_ios::ostringstream name_stream;
  name_stream << proxy->GetXMLName() <<
    this->NameGenerator->GetCountAndIncrement(
      proxy->GetXMLName());

  pxm->RegisterProxy(register_group, name_stream.str().c_str(), proxy);
  proxy->Delete();

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->EndUndoSet();
    }
  return proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineBuilder::createPipelineProxy(const char* xmlgroup,
    const char* xmlname, pqServer* server, pqRenderModule* renModule)
{
  vtkSMProxy* proxy = this->createProxy(xmlgroup, xmlname, "sources",
    server, true);

  if (proxy && renModule)
    {
    if (this->UndoStack)
      {
      this->UndoStack->BeginUndoSet("Connect display");
      }

    this->createDisplayProxyInternal(proxy, renModule->getRenderModuleProxy());
    if (this->UndoStack)
      {
      this->UndoStack->EndUndoSet();
      }
    }
  return proxy;
}

//-----------------------------------------------------------------------------
pqPipelineDisplay* pqPipelineBuilder::createDisplayProxy(pqPipelineSource* src,
  pqRenderModule* renModule)
{
  if (!src || !renModule )
    {
    qCritical() <<"Missing required attribute.";
    return NULL;
    }

  vtkSMProxy *proxy = src->getProxy();
  if (!proxy)
    {
    qDebug() << "Failed to locate proxy to connect display to.";
    return NULL;
    }

  if (this->UndoStack)
    {
    vtksys_ios::ostringstream label;
    label << "Display " << (proxy->GetXMLName()? proxy->GetXMLName() : "" );
    this->UndoStack->BeginUndoSet(QString(label.str().c_str()));
    }
  vtkSMDisplayProxy* display = 
    this->createDisplayProxyInternal(proxy, renModule->getRenderModuleProxy());
  if (this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }

  if (display)
    {
    return pqServerManagerModel::instance()->getPQDisplay(display);
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy* pqPipelineBuilder::createDisplayProxyInternal(
  vtkSMProxy* proxy, vtkSMRenderModuleProxy* renModule)
{
  if (!proxy)
    {
    qDebug() << "Cannot connect display to NULL source proxy.";
    return NULL;
    }
  //proxy->CreateParts();
  vtkSMAbstractDisplayProxy* adisplay = renModule->CreateDisplayProxy();
  vtkSMDisplayProxy* display = vtkSMDisplayProxy::SafeDownCast(adisplay);
  if ( !display )
    {
    adisplay->Delete();
    qDebug() << "Cannot create display.";
    return NULL;
    }
 
  // Register the proxy -- must be done first before any property changes 
  // (of undo/redo to work).
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  pxm->RegisterProxy("displays", display->GetSelfIDAsString(), display);
  display->Delete();
  
  vtkSMProxyProperty* pp;
  
  // Set the display proxy input.
  pp = vtkSMProxyProperty::SafeDownCast(
    display->GetProperty("Input"));
  pp->AddProxy(proxy);

  // Add the display proxy to render module.
  pp = vtkSMProxyProperty::SafeDownCast(
    renModule->GetProperty("Displays"));
  pp->AddProxy(display);
  renModule->UpdateVTKObjects();

  display->UpdateVTKObjects();

  pqPipelineDisplay* dispObject = 
    pqServerManagerModel::instance()->getPQDisplay(display);
  dispObject->setDefaultColorParametes();
  return display;
}

//-----------------------------------------------------------------------------
int pqPipelineBuilder::removeInternal(pqPipelineDisplay* display)
{
  if (!display)
    {
    qDebug() << "Cannot remove null display.";
    return 0;
    }

  // 1) Remove display from the render module.
  // eventually, the pqPipelineDisplay can tell us which render module
  // it belongs to. For now, we just use the active render module.
  unsigned int numRenModules = display->getNumberOfRenderModules();
  for(unsigned int i=0; i<numRenModules; i++)
    {
    pqRenderModule* renModule = display->getRenderModule(i);
    
    vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
      renModule->getProxy()->GetProperty("Displays"));
    pp->RemoveProxy(display->getProxy());
    renModule->getProxy()->UpdateVTKObjects();

    }

  /*
  pp = vtkSMProxyProperty::SafeDownCast(
    display->getProxy()->GetProperty("Input"));
  pp->RemoveAllProxies();
  */

  // Unregister display.
  vtkSMProxyManager::GetProxyManager()->UnRegisterProxy(
    display->getSMGroup().toStdString().c_str(), 
    display->getSMName().toStdString().c_str());
  return 1;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::remove(pqPipelineDisplay* display, 
  bool is_undoable/*=true*/)
{
  if (is_undoable && this->UndoStack)
    {
    this->UndoStack->BeginUndoSet(QString("Remove Display"));
    }

  this->removeInternal(display);

  if (is_undoable && this->UndoStack)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::remove(pqPipelineSource* source,
  bool is_undoable/*=false*/)
{
  if (!source)
    {
    qDebug() << "Cannot remove null source.";
    return;
    }

  if (source->getNumberOfConsumers())
    {
    qDebug() << "Cannot remove source with consumers.";
    return;
    }

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->BeginUndoSet(QString("Remove Source"));
    }



  // 2) remove inputs.
  // TODO: this step should not be necessary, but it currently
  // is :(. Needs some looking into.
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    source->getProxy()->GetProperty("Input"));
  if (pp)
    {
    pp->RemoveAllProxies();
    }

  // 1) remove all displays.
  while (source->getDisplayCount())
    {
    if (!this->removeInternal(source->getDisplay(0)))
      {
      qDebug() << "Failed to remove display!";
      return;
      }
    }

  // 3) Unregister proxy.
  vtkSMProxyManager::GetProxyManager()->UnRegisterProxy(
    source->getSMGroup().toStdString().c_str(), 
    source->getSMName().toStdString().c_str());

  if (this->UndoStack && is_undoable)
    {
    this->UndoStack->EndUndoSet();
    }
}

//-----------------------------------------------------------------------------
pqRenderModule* pqPipelineBuilder::createWindow(pqServer* server)
{
  if (!server)
    {
    qDebug() << "Cannot createWindow on null server.";
    return NULL;
    }

  // This is not an undo-able operation (atleast for now).
  vtkSMRenderModuleProxy* renModule = server->newRenderModule();

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->RegisterProxy("render_modules", renModule->GetSelfIDAsString(),
    renModule);
  renModule->Delete();

  // as a side effect of the registeration, pqServerManagerModel will
  // have created a nice new pqRenderModule, obtain it.
  pqServerManagerModel* smModel = pqServerManagerModel::instance();
  pqRenderModule* pqRM = smModel->getRenderModule(renModule);
  if (!pqRM)
    {
    qDebug() << "Failed to create pqRenderModule.";
    }

  // Now set up default RM properties.
  // if this property exists (server/client mode), render remotely
  // this should change to a user controlled setting, but this is here for testing
  vtkSMProperty* prop = renModule->GetProperty("CompositeThreshold");
  if(prop)
    {
    pqSMAdaptor::setElementProperty(prop, 0.0);  // remote render
    }
  // disable LOD.
  pqSMAdaptor::setElementProperty(renModule->GetProperty("LODThreshold"),
      VTK_LARGE_FLOAT);

  // Now load default values from the QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  QList<QString> propertyNames;
  propertyNames.push_back("CameraParallelProjection");
  propertyNames.push_back("UseTriangleStrips");
  propertyNames.push_back("UseImmediateMode");
  propertyNames.push_back("LODThreshold");
  propertyNames.push_back("LODResolution");
  propertyNames.push_back("RenderInterruptsEnabled");
  propertyNames.push_back("CompositeThreshold");
  propertyNames.push_back("ReductionFactor");
  propertyNames.push_back("SquirtLevel");
  propertyNames.push_back("OrderedCompositing");
  foreach(QString property_name, propertyNames)
    {
    QString key = QString("renderModule/") + property_name;
    if (renModule->GetProperty(property_name.toAscii().data()) && settings->contains(key))
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty(property_name.toAscii().data()),
        settings->value("renderModule/" + property_name));
      }
    }
  if (settings->contains("renderModule/Background"))
    {
    pqSMAdaptor::setMultipleElementProperty(
      renModule->GetProperty("Background"),
      settings->value("renderModule/Background").value<QList<QVariant> >());
    }

  renModule->UpdateVTKObjects();

#if 0
  // turn on vtk light kit
  renModule->SetUseLight(1);
  // turn off main light
  pqSMAdaptor::setElementProperty(renModule->GetProperty("LightSwitch"), 0);
#endif

  renModule->UpdateVTKObjects();
  return pqRM;

}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::removeWindow(pqRenderModule* rm)
{
  if (!rm)
    {
    qDebug() << "Nothing to remove.";
    return;
    }

  // Get a list of all displays belonging to this render module. We delete
  // all the displays that belong only to this render module.
  QList<pqPipelineDisplay*> displays = rm->getDisplays();

  // Unregister the proxy....the rest of the GUI will(rather should) manage itself!
  QString name = rm->getProxyName();
  vtkSMMultiViewRenderModuleProxy* multiRM = rm->getServer()->GetRenderModule();
  vtkSMRenderModuleProxy* rmProxy = rm->getRenderModuleProxy();

  // This need to be done since multiRM adds all created rendermodules to itself.
  // This may need revisiting once we fully support multi-view.
  // This removal is necessary,as otherwise the vtkSMRenderModuleProxy lingers
  // after this call -- which is not good, since the vtkSMRenderModuleProxy 
  // is as such not useful.
  for (unsigned int cc=0; cc < multiRM->GetNumberOfProxies(); cc++)
    {
    if (multiRM->GetProxy(cc) == rmProxy)
      {
      multiRM->RemoveProxy(multiRM->GetProxyName(cc));
      break;
      }
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxy("render_modules", name.toStdString().c_str());
  // rm is invalid at this point.
 
  // Now clean up any orphan displays.
  foreach (pqPipelineDisplay* disp, displays)
    {
    if (disp->getNumberOfRenderModules() == 0)
      {
      this->removeInternal(disp);      
      }
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqPipelineBuilder::createLookupTable(pqPipelineDisplay* display)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  
  vtkSMProxy* lut = pxm->NewProxy("lookup_tables", "LookupTable");
  lut->SetConnectionID(display->getProxy()->GetConnectionID());
  lut->SetServers(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  
  // register it.
  pxm->RegisterProxy("lookup_tables", lut->GetSelfIDAsString(), lut);
  lut->Delete();

  // LUT must go from blue to red.
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    lut->GetProperty("HueRange"));
  dvp->SetElement(0, 0.6667);
  dvp->SetElement(1, 0.0);
  lut->UpdateVTKObjects();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    display->getProxy()->GetProperty("LookupTable"));
  if (pp)
    {
    pp->RemoveAllProxies();
    pp->AddProxy(lut);
    }

  return lut;
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::deleteProxies(pqServer* server)
{
  if (!server)
    {
    qDebug() << "Server cannot be NULL.";
    return;
    }
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  pxm->UnRegisterProxies(server->GetConnectionID());
}

//-----------------------------------------------------------------------------
void pqPipelineBuilder::getSupportedProxies(const QString& xmlgroup, 
  pqServer* vtkNotUsed(server), QList<QString>& names)
{
  names.clear();
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  unsigned int numProxies = pxm->GetNumberOfXMLProxies(
    xmlgroup.toStdString().c_str());
  for (unsigned int cc=0; cc <numProxies; cc++)
    {
    const char* name = pxm->GetXMLProxyName(xmlgroup.toStdString().c_str(),
      cc);
    if (name)
      {
      names.push_back(name);
      }
    }
}

