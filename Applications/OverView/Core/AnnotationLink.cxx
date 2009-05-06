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
#include "pqPipelineSource.h"
#include "pqServerManagerModel.h"
#include "pqView.h"
#include "pqViewManager.h"

#include <vtksys/stl/set>

class AnnotationLinkInternals
{
public:
  AnnotationLinkInternals() 
    {
    this->VTKLink = vtkSmartPointer<vtkAnnotationLink>::New();
    this->InSelectionChanged = false;
    this->ViewManager = 0;
    }

  ~AnnotationLinkInternals()
    {
    }

  vtksys_stl::set<vtkSmartPointer<vtkSMSourceProxy> > Sources;
  vtkSmartPointer<vtkAnnotationLink> VTKLink;
  bool InSelectionChanged;
  pqViewManager* ViewManager;
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
}

//-----------------------------------------------------------------------------
AnnotationLink::~AnnotationLink()
{
  delete this->Internals;
  this->Command->Delete();
}

//-----------------------------------------------------------------------------
void AnnotationLink::setViewManager(pqViewManager* mgr)
{
  this->Internals->ViewManager = mgr;
}

//-----------------------------------------------------------------------------
void AnnotationLink::setAnnotationLayers(vtkAnnotationLayers* layers)
{
  this->Internals->VTKLink->SetAnnotationLayers(layers);
  this->updateViews();
}

//-----------------------------------------------------------------------------
vtkAnnotationLayers* AnnotationLink::getAnnotationLayers()
{
  return this->Internals->VTKLink->GetAnnotationLayers();
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

void AnnotationLink::updateViews()
{
  if (this->Internals->ViewManager)
    {
    QList<pqMultiViewFrame*> list =
      qFindChildren<pqMultiViewFrame*>(this->Internals->ViewManager);
    for (int i = 0; i < list.size(); ++i)
      {
      pqView* view = this->Internals->ViewManager->getView(list[i]);
      if (view)
        {
        view->render();
        }
      }
    }
}
