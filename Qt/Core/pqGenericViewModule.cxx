/*=========================================================================

   Program: ParaView
   Module:    pqGenericViewModule.cxx

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

=========================================================================*/
#include "pqGenericViewModule.h"

// ParaView Server Manager includes.
#include "vtkProcessModule.h"
#include "vtkSMAbstractViewModuleProxy.h"
#include "vtkSmartPointer.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QtDebug>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqDisplay.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqTimeKeeper.h"

template<class T>
inline uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

class pqGenericViewModuleInternal
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  // List of displays shown by this render module.
  QList<QPointer<pqDisplay> > Displays;
  vtkSmartPointer<vtkSMPropertyLink> ViewTimeLink;

  pqGenericViewModuleInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }
};

//-----------------------------------------------------------------------------
pqGenericViewModule::pqGenericViewModule(
  const QString& type,
  const QString& group, const QString& name, 
  vtkSMAbstractViewModuleProxy* renModule, pqServer* server, 
  QObject* _parent/*=null*/)
: pqProxy(group, name, renModule, server, _parent)
{
  this->ViewType = type;
  this->Internal = new pqGenericViewModuleInternal();

  // Listen to updates on the displays property.
  this->Internal->VTKConnect->Connect(renModule->GetProperty("Displays"),
    vtkCommand::ModifiedEvent, this, SLOT(displaysChanged()));

  // Fire start/end render signals when the underlying proxy
  // fires appropriate events.
  this->Internal->VTKConnect->Connect(renModule,
    vtkCommand::StartEvent, this, SIGNAL(beginRender()));
  this->Internal->VTKConnect->Connect(renModule,
    vtkCommand::EndEvent, this, SIGNAL(endRender()));

  // If the render module already has some displays in it when it is registered,
  // this method will detect them and sync the GUI state with the SM state.
  this->displaysChanged();

  // Link ViewTime with global time.
  vtkSMProxy* timekeeper = this->getServer()->getTimeKeeper()->getProxy();

  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  link->AddLinkedProperty(timekeeper->GetProperty("Time"), vtkSMLink::INPUT);
  link->AddLinkedProperty(renModule->GetProperty("ViewTime"), vtkSMLink::OUTPUT);
  renModule->GetProperty("ViewTime")->Copy(timekeeper->GetProperty("Time"));
  this->Internal->ViewTimeLink = link;
  link->Delete();

}

//-----------------------------------------------------------------------------
pqGenericViewModule::~pqGenericViewModule()
{
  foreach(pqDisplay* disp, this->Internal->Displays)
    {
    if (disp)
      {
      disp->removeRenderModule(this);
      }
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMAbstractViewModuleProxy* pqGenericViewModule::getViewModuleProxy() const
{
  return vtkSMAbstractViewModuleProxy::SafeDownCast(this->getProxy());
}


//-----------------------------------------------------------------------------
void pqGenericViewModule::forceRender()
{
  vtkSMAbstractViewModuleProxy* view = this->getViewModuleProxy();
  if (view)
    {
    vtkProcessModule::GetProcessModule()->SendPrepareProgress(
      view->GetConnectionID());
    view->StillRender();
    vtkProcessModule::GetProcessModule()->SendCleanupPendingProgress(
      view->GetConnectionID());
    }
}

//-----------------------------------------------------------------------------
bool pqGenericViewModule::hasDisplay(pqDisplay* display)
{
  return this->Internal->Displays.contains(display);
}

//-----------------------------------------------------------------------------
int pqGenericViewModule::getDisplayCount() const
{
  return this->Internal->Displays.size();
}

//-----------------------------------------------------------------------------
pqDisplay* pqGenericViewModule::getDisplay(int index) const
{
  if(index >= 0 && index < this->Internal->Displays.size())
    {
    return this->Internal->Displays[index];
    }

  return 0;
}

//-----------------------------------------------------------------------------
QList<pqDisplay*> pqGenericViewModule::getDisplays() const
{
  QList<pqDisplay*> list;
  foreach (pqDisplay* disp, this->Internal->Displays)
    {
    if (disp)
      {
      list.push_back(disp);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void pqGenericViewModule::displaysChanged()
{
  // Determine what changed. Add the new displays and remove the old
  // ones. Make sure new displays have a reference to this render module.
  // Remove the reference to this render module in the removed displays.
  QList<QPointer<pqDisplay> > currentDisplays;
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Displays"));
  pqServerManagerModel* smModel = 
    pqApplicationCore::instance()->getServerManagerModel();

  bool unknown_display = false;
  unsigned int max = prop->GetNumberOfProxies();
  for (unsigned int cc=0; cc < max; ++cc)
    {
    vtkSMProxy* proxy = prop->GetProxy(cc);
    if (!proxy)
      {
      continue;
      }
    pqDisplay* display = qobject_cast<pqDisplay*>(
      smModel->getPQProxy(proxy));
    if (!display)
      {
      unknown_display = true;
      continue;
      }
    currentDisplays.append(QPointer<pqDisplay>(display));
    if(!this->Internal->Displays.contains(display))
      {
      // Update the render module pointer in the display.
      display->addRenderModule(this);
      this->Internal->Displays.append(QPointer<pqDisplay>(display));
      QObject::connect(display, SIGNAL(visibilityChanged(bool)),
        this, SLOT(onDisplayVisibilityChanged(bool)));
      emit this->displayAdded(display);
      }
    }

  QList<QPointer<pqDisplay> >::Iterator iter =
      this->Internal->Displays.begin();
  while(iter != this->Internal->Displays.end())
    {
    if(*iter && !currentDisplays.contains(*iter))
      {
      pqDisplay* display = (*iter);
      // Remove the render module pointer from the display.
      display->removeRenderModule(this);
      iter = this->Internal->Displays.erase(iter);
      QObject::disconnect(display, 0, this, 0);
      emit this->displayRemoved(display);
      }
    else
      {
      ++iter;
      }
    }

  // If the "Displays" property has some proxy which 
  // hasn't been registered, we will listen to the SMModel
  // to known when it gets registered.
  if (unknown_display)
    {
    QObject::connect(smModel, SIGNAL(displayAdded(pqDisplay*)),
      this, SLOT(displayCreated(pqDisplay*)));
    }
  else
    {
    QObject::disconnect(smModel, SIGNAL(displayAdded(pqDisplay*)),
      this, SLOT(displayCreated(pqDisplay*)));
    }
}

//-----------------------------------------------------------------------------
void pqGenericViewModule::displayCreated(pqDisplay* display)
{
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Displays"));
  if (prop->IsProxyAdded(display->getProxy()))
    {
    display->addRenderModule(this);
    this->Internal->Displays.append(display);
    QObject::connect(display, SIGNAL(visibilityChanged(bool)),
      this, SLOT(onDisplayVisibilityChanged(bool)));
    emit this->displayAdded(display);
    }
}

//-----------------------------------------------------------------------------
void pqGenericViewModule::onDisplayVisibilityChanged(bool visible)
{
  pqDisplay* disp = qobject_cast<pqDisplay*>(this->sender());
  if (disp)
    {
    emit this->displayVisibilityChanged(disp, visible);
    }
}
