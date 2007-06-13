/*=========================================================================

   Program: ParaView
   Module:    pqView.cxx

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
#include "pqView.h"

// ParaView Server Manager includes.
#include "vtkProcessModule.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyProperty.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QtDebug>
#include <QTimer>

// ParaView includes.
#include "pqApplicationCore.h"
#include "pqRepresentation.h"
#include "pqServer.h"
#include "pqServerManagerModel2.h"
#include "pqTimeKeeper.h"

template<class T>
inline uint qHash(QPointer<T> p)
{
  return qHash(static_cast<T*>(p));
}

class pqViewInternal
{
public:
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  // List of representation shown by this view.
  QList<QPointer<pqRepresentation> > Representations;
  vtkSmartPointer<vtkSMPropertyLink> ViewTimeLink;

  pqViewInternal()
    {
    this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
    }

  QTimer RenderTimer;
};

//-----------------------------------------------------------------------------
pqView::pqView( const QString& type,
                const QString& group, 
                const QString& name, 
                vtkSMViewProxy* view, 
                pqServer* server, 
                QObject* _parent/*=null*/) : 
  pqProxy(group, name, view, server, _parent)
{
  this->ViewType = type;
  this->Internal = new pqViewInternal();

  // Listen to updates on the Representations property.
  this->Internal->VTKConnect->Connect(
    view->GetProperty("Representations"),
    vtkCommand::ModifiedEvent, this, SLOT(onRepresentationsChanged()));

  // Fire start/end render signals when the underlying proxy
  // fires appropriate events.
  this->Internal->VTKConnect->Connect(view,
    vtkCommand::StartEvent, this, SIGNAL(beginRender()));
  this->Internal->VTKConnect->Connect(view,
    vtkCommand::EndEvent, this, SIGNAL(endRender()));

  // If the render module already has some representations in it when it is
  // registered, this method will detect them and sync the GUI state with the 
  // SM state.
  this->onRepresentationsChanged();

  // Link ViewTime with global time.
  vtkSMProxy* timekeeper = this->getServer()->getTimeKeeper()->getProxy();

  vtkSMPropertyLink* link = vtkSMPropertyLink::New();
  link->AddLinkedProperty(timekeeper->GetProperty("Time"), vtkSMLink::INPUT);
  link->AddLinkedProperty(view->GetProperty("ViewTime"), vtkSMLink::OUTPUT);
  view->GetProperty("ViewTime")->Copy(timekeeper->GetProperty("Time"));
  this->Internal->ViewTimeLink = link;
  link->Delete();

  this->Internal->RenderTimer.setSingleShot(true);
  this->Internal->RenderTimer.setInterval(1);
  QObject::connect(&this->Internal->RenderTimer, SIGNAL(timeout()),
    this, SLOT(forceRender()));


  pqServerManagerModel2* smModel = 
    pqApplicationCore::instance()->getServerManagerModel2();
  QObject::connect(smModel, SIGNAL(representationAdded(pqRepresentation*)),
    this, SLOT(representationCreated(pqRepresentation*)));
}

//-----------------------------------------------------------------------------
pqView::~pqView()
{
  foreach(pqRepresentation* disp, this->Internal->Representations)
    {
    if (disp)
      {
      disp->setView(0);
      }
    }

  delete this->Internal;
}

//-----------------------------------------------------------------------------
vtkSMViewProxy* pqView::getViewProxy() const
{
  return vtkSMViewProxy::SafeDownCast(this->getProxy());
}

//-----------------------------------------------------------------------------
void pqView::render()
{
  this->Internal->RenderTimer.start();
}

//-----------------------------------------------------------------------------
void pqView::forceRender()
{
  vtkSMViewProxy* view = this->getViewProxy();
  if (view)
    {
    // FIXME:UDA We are managing progess in View module, 
    // do we need it here?
    //vtkProcessModule::GetProcessModule()->SendPrepareProgress(
    //  view->GetConnectionID());
    view->StillRender();
    //vtkProcessModule::GetProcessModule()->SendCleanupPendingProgress(
    //  view->GetConnectionID());
    }
}

//-----------------------------------------------------------------------------
bool pqView::hasRepresentation(pqRepresentation* repr) const
{
  return this->Internal->Representations.contains(repr);
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfRepresentations() const
{
  return this->Internal->Representations.size();
}

//-----------------------------------------------------------------------------
int pqView::getNumberOfVisibleRepresentations() const
{
  int count = 0;
  for (int i=0; i<this->Internal->Representations.size(); i++)
    {
    pqRepresentation *repr = this->Internal->Representations[i];
    if(repr->isVisible())
      {
      count++;
      }
    }
  return count;
}

//-----------------------------------------------------------------------------
pqRepresentation* pqView::getRepresentation(int index) const
{
  if(index >= 0 && index < this->Internal->Representations.size())
    {
    return this->Internal->Representations[index];
    }

  return 0;
}

//-----------------------------------------------------------------------------
QList<pqRepresentation*> pqView::getRepresentations() const
{
  QList<pqRepresentation*> list;
  foreach (pqRepresentation* disp, this->Internal->Representations)
    {
    if (disp)
      {
      list.push_back(disp);
      }
    }
  return list;
}

//-----------------------------------------------------------------------------
void pqView::onRepresentationsChanged()
{
  // Determine what changed. Add the new Representations and remove the old
  // ones. Make sure new Representations have a reference to this render module.
  // Remove the reference to this render module in the removed Representations.
  QList<QPointer<pqRepresentation> > currentReprs;
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Representations"));
  pqServerManagerModel2* smModel = 
    pqApplicationCore::instance()->getServerManagerModel2();

  unsigned int max = prop->GetNumberOfProxies();
  for (unsigned int cc=0; cc < max; ++cc)
    {
    vtkSMProxy* proxy = prop->GetProxy(cc);
    if (!proxy)
      {
      continue;
      }
    pqRepresentation* repr =  smModel->findItem<pqRepresentation*>(proxy);
    if (!repr)
      {
      continue;
      }
    currentReprs.append(QPointer<pqRepresentation>(repr));
    if(!this->Internal->Representations.contains(repr))
      {
      // Update the render module pointer in the repr.
      repr->setView(this);
      this->Internal->Representations.append(QPointer<pqRepresentation>(repr));
      QObject::connect(repr, SIGNAL(visibilityChanged(bool)),
        this, SLOT(onRepresentationVisibilityChanged(bool)));
      emit this->representationAdded(repr);
      }
    }

  QList<QPointer<pqRepresentation> >::Iterator iter =
      this->Internal->Representations.begin();
  while(iter != this->Internal->Representations.end())
    {
    if(*iter && !currentReprs.contains(*iter))
      {
      pqRepresentation* repr = (*iter);
      // Remove the render module pointer from the repr.
      repr->setView(0);
      iter = this->Internal->Representations.erase(iter);
      QObject::disconnect(repr, 0, this, 0);
      emit this->representationRemoved(repr);
      }
    else
      {
      ++iter;
      }
    }
}

//-----------------------------------------------------------------------------
void pqView::representationCreated(pqRepresentation* repr)
{
  vtkSMProxyProperty* prop = vtkSMProxyProperty::SafeDownCast(
    this->getProxy()->GetProperty("Representations"));
  if (prop->IsProxyAdded(repr->getProxy()))
    {
    repr->setView(this);
    this->Internal->Representations.append(repr);
    QObject::connect(repr, SIGNAL(visibilityChanged(bool)),
      this, SLOT(onRepresentationVisibilityChanged(bool)));
    emit this->representationAdded(repr);
    }
}

//-----------------------------------------------------------------------------
void pqView::onRepresentationVisibilityChanged(bool visible)
{
  pqRepresentation* disp = qobject_cast<pqRepresentation*>(this->sender());
  if (disp)
    {
    emit this->representationVisibilityChanged(disp, visible);
    }
}
