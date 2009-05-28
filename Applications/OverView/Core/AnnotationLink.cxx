/*=========================================================================

   Program: ParaView
   Module:    AnnotationLink.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
#include "AnnotationLink.h"

#include "vtkAnnotationLayers.h"
#include "vtkAnnotationLink.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

#include "pqApplicationCore.h"
#include "pqMultiViewFrame.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqView.h"

#include <vtksys/stl/set>

class AnnotationLinkInternals
{
public:
  AnnotationLinkInternals() 
    {
    this->Link = 0;
    this->InSelectionChanged = false;
    }

  ~AnnotationLinkInternals()
    {
    }

  vtksys_stl::set<vtkSmartPointer<vtkSMSourceProxy> > Sources;
  vtksys_stl::set<pqView*> Views;
  vtkSmartPointer<vtkSMSourceProxy> Link;
  bool InSelectionChanged;
};

class AnnotationLinkCommand : public vtkCommand
{
public:
  static AnnotationLinkCommand* New()
    { return new AnnotationLinkCommand(); }
  void Execute(vtkObject* caller, unsigned long id, void* callData);
  AnnotationLink* Target;
};

void AnnotationLinkCommand::Execute(
  vtkObject* caller, unsigned long, void*)
{
  this->Target->selectionChanged(vtkSMSourceProxy::SafeDownCast(caller));
}

//-----------------------------------------------------------------------------
AnnotationLink& AnnotationLink::instance()
{
  static AnnotationLink the_instance;
  return the_instance;
}

//-----------------------------------------------------------------------------
AnnotationLink::AnnotationLink()
{
  this->Internals = new AnnotationLinkInternals();
  this->Command = AnnotationLinkCommand::New();
  this->Command->Target = this;

  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(sourceAdded(pqPipelineSource*)),
    this, SLOT(onSourceAdded(pqPipelineSource*)));
  QObject::connect(
    pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(sourceRemoved(pqPipelineSource*)),
    this, SLOT(onSourceRemoved(pqPipelineSource*)));
  QObject::connect(
    pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(viewCreated(pqView*)),
    this, SLOT(onViewCreated(pqView*)));
  QObject::connect(
    pqApplicationCore::instance()->getObjectBuilder(),
    SIGNAL(destroying(pqView*)),
    this, SLOT(onViewDestroyed(pqView*)));
}

//-----------------------------------------------------------------------------
AnnotationLink::~AnnotationLink()
{
  delete this->Internals;
  this->Command->Delete();
}

//-----------------------------------------------------------------------------
void AnnotationLink::initialize(pqServer* server)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkSMProxy* p = pxm->NewProxy("selection_helpers", "AnnotationLink");
  this->Internals->Link.TakeReference(static_cast<vtkSMSourceProxy*>(p));
  this->Internals->Link->SetConnectionID(server->GetConnectionID());
  pxm->RegisterProxy("selection_helpers", "AnnotationLink", this->Internals->Link);
  this->Internals->Link->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
vtkAnnotationLink* AnnotationLink::getLink()
{
  if (this->Internals->Link)
    {
    return static_cast<vtkAnnotationLink*>(this->Internals->Link->GetClientSideObject());
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkSMSourceProxy* AnnotationLink::getLinkProxy()
{
  return this->Internals->Link;
}

//-----------------------------------------------------------------------------
void AnnotationLink::onSourceAdded(pqPipelineSource* source)
{
  vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(source->getProxy());
  this->Internals->Sources.insert(proxy);
  proxy->AddObserver(vtkCommand::SelectionChangedEvent, this->Command);
}

//-----------------------------------------------------------------------------
void AnnotationLink::onSourceRemoved(pqPipelineSource* source)
{
  vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(source->getProxy());
  this->Internals->Sources.erase(proxy);
  proxy->RemoveObserver(this->Command);
}

//-----------------------------------------------------------------------------
void AnnotationLink::onViewCreated(pqView* view)
{
  this->Internals->Views.insert(view);
  view->setAnnotationLink(this->Internals->Link);
}

//-----------------------------------------------------------------------------
void AnnotationLink::onViewDestroyed(pqView* view)
{
  this->Internals->Views.erase(view);
}

//-----------------------------------------------------------------------------
void AnnotationLink::selectionChanged(vtkSMSourceProxy* source)
{
  // Avoid infinite loops
  if (!this->Internals->InSelectionChanged)
    {
    this->Internals->InSelectionChanged = true;
    vtksys_stl::set<vtkSmartPointer<vtkSMSourceProxy> >::iterator it, itEnd;
    it = this->Internals->Sources.begin();
    itEnd = this->Internals->Sources.end();
    for (; it != itEnd; ++it)
      {
      if (it->GetPointer() != source)
        {
        (*it)->SetSelectionInput(0, source->GetSelectionInput(0), 0);
        }
      }

    this->updateViews();

    this->Internals->InSelectionChanged = false;
    }
}
/*
vtkSelection* AnnotationLink::getSelection()
{
  vtksys_stl::set<vtkSmartPointer<vtkSMSourceProxy> >::iterator it, itEnd;
  it = this->Internals->Sources.begin();
  if(it == this->Internals->Sources.end())
    return 0;
  vtkSMSourceProxy* source = *it;

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMClientDeliveryStrategyProxy* strategy = 
    vtkSMClientDeliveryStrategyProxy::SafeDownCast(
      pxm->NewProxy("strategies", "ClientDeliveryStrategy"));
  strategy->AddInput(source->GetSelectionInput(0), 0);
  strategy->SetPostGatherHelper("vtkAppendSelection");
  strategy->Update();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(strategy->GetOutput()->GetID()));
  return vtkSelection::SafeDownCast(alg->GetOutputDataObject(0));
}
*/
void AnnotationLink::updateViews()
{
  vtksys_stl::set<pqView*>::iterator it, itEnd;
  itEnd = this->Internals->Views.end();
  for (it = this->Internals->Views.begin(); it != itEnd; ++it)
    {
    (*it)->render();
    }
}
