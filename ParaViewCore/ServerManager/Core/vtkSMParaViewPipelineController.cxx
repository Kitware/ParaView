/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParaViewPipelineController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMParaViewPipelineController.h"

#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"

#include <cassert>
#include <map>
#include <set>
#include <vector>
#include <vtksys/ios/sstream>

class vtkSMParaViewPipelineController::vtkInternals
{
public:
  typedef std::map<void*, vtkTimeStamp> TimeStampsMap;
  TimeStampsMap InitializationTimeStamps;
};

namespace
{
  // Used to monitor properties whose domains change.
  class vtkDomainObserver
    {
    std::vector<std::pair<vtkSMProperty*, unsigned long> > MonitoredProperties;
    std::set<vtkSMProperty*> PropertiesWithModifiedDomains;

    void DomainModified(vtkObject* sender, unsigned long, void*)
      {
      vtkSMProperty* prop = vtkSMProperty::SafeDownCast(sender);
      if (prop)
        {
        this->PropertiesWithModifiedDomains.insert(prop);
        }
      }

  public:
    vtkDomainObserver()
      {
      }
    ~vtkDomainObserver()
      {
      for (size_t cc=0; cc < this->MonitoredProperties.size(); cc++)
        {
        this->MonitoredProperties[cc].first->RemoveObserver(
          this->MonitoredProperties[cc].second);
        }
      }
    void Monitor(vtkSMProperty* prop)
      {
      assert(prop != NULL);
      unsigned long oid = prop->AddObserver(vtkCommand::DomainModifiedEvent,
        this, &vtkDomainObserver::DomainModified);
      this->MonitoredProperties.push_back(
        std::pair<vtkSMProperty*, unsigned long>(prop, oid));
      }

    const std::set<vtkSMProperty*>& GetPropertiesWithModifiedDomains() const
      {
      return this->PropertiesWithModifiedDomains;
      }
    };
}

vtkObjectFactoryNewMacro(vtkSMParaViewPipelineController);
//----------------------------------------------------------------------------
vtkSMParaViewPipelineController::vtkSMParaViewPipelineController()
  : Internals(new vtkSMParaViewPipelineController::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMParaViewPipelineController::~vtkSMParaViewPipelineController()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::FindProxy(
  vtkSMSessionProxyManager* pxm, const char* reggroup, const char* xmlgroup, const char* xmltype)
{
  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(pxm);
  iter->SetModeToOneGroup();

  for (iter->Begin(reggroup); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy != NULL &&
      proxy->GetXMLGroup() &&
      proxy->GetXMLName() &&
      strcmp(proxy->GetXMLGroup(), xmlgroup) == 0 &&
      strcmp(proxy->GetXMLName(), xmltype) == 0)
      {
      return proxy;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::CreateProxiesForProxyListDomains(
  vtkSMProxy* proxy)
{
  assert(proxy != NULL);
  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyListDomain* pld = iter->GetProperty()?
      vtkSMProxyListDomain::SafeDownCast(iter->GetProperty()->FindDomain("vtkSMProxyListDomain"))
      : NULL;
    if (pld)
      {
      pld->CreateProxies(proxy->GetSessionProxyManager());
      for (unsigned int cc=0, max=pld->GetNumberOfProxies(); cc < max; cc++)
        {
        this->InitializeProxy(pld->GetProxy(cc));
        }
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineController::RegisterProxiesForProxyListDomains(vtkSMProxy* proxy)
{
  assert(proxy != NULL);
  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();

  vtksys_ios::ostringstream groupnamestr;
  groupnamestr << "pq_helper_proxies." << proxy->GetGlobalIDAsString();
  std::string groupname = groupnamestr.str();

  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxyListDomain* pld = iter->GetProperty()?
      vtkSMProxyListDomain::SafeDownCast(iter->GetProperty()->FindDomain("vtkSMProxyListDomain"))
      : NULL;
    if (!pld)
      {
      continue;
      }
    for (unsigned int cc=0, max=pld->GetNumberOfProxies(); cc < max; cc++)
      {
      pxm->RegisterProxy(groupname.c_str(), iter->GetKey(), pld->GetProxy(cc));
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::CreateAnimationHelpers(vtkSMProxy* proxy)
{
  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!source)
    {
    return false;
    }
  assert(proxy != NULL);
  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();

  vtksys_ios::ostringstream groupnamestr;
  groupnamestr << "pq_helper_proxies." << proxy->GetGlobalIDAsString();
  std::string groupname = groupnamestr.str();

  for (unsigned int cc=0, max=source->GetNumberOfOutputPorts(); cc < max; cc++)
    {
    vtkSmartPointer<vtkSMProxy> helper;
    helper.TakeReference(pxm->NewProxy("misc", "RepresentationAnimationHelper"));
    if (helper) // since this is optional
      {
      this->PreInitializeProxy(helper);
      vtkSMPropertyHelper(helper, "Source").Set(proxy);
      this->PostInitializeProxy(helper);
      helper->UpdateVTKObjects();

      // yup, all are registered under same name.
      pxm->RegisterProxy(
        groupname.c_str(), "RepresentationAnimationHelper", helper);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::InitializeSession(vtkSMSession* session)
{
  assert(session != NULL);

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  assert(pxm);

  //---------------------------------------------------------------------------
  // If the session is a collaborative session, we need to fetch the state from
  // server before we start creating "essential" proxies. This is a no-op if no
  // a collaborative session.
  pxm->UpdateFromRemote();

  //---------------------------------------------------------------------------
  // Setup selection models used to track active view/active proxy.
  vtkSMProxySelectionModel* selmodel = pxm->GetSelectionModel("ActiveSources");
  if (selmodel == NULL)
    {
    selmodel = vtkSMProxySelectionModel::New();
    pxm->RegisterSelectionModel("ActiveSources", selmodel);
    selmodel->FastDelete();
    }

  selmodel = pxm->GetSelectionModel("ActiveView");
  if (selmodel == NULL)
    {
    selmodel = vtkSMProxySelectionModel::New();
    pxm->RegisterSelectionModel("ActiveView", selmodel);
    selmodel->FastDelete();
    }

  //---------------------------------------------------------------------------
  // Create the timekeeper if none exists.
  vtkSmartPointer<vtkSMProxy> timeKeeper = this->FindTimeKeeper(session);
  if (!timeKeeper)
    {
    timeKeeper.TakeReference(pxm->NewProxy("misc", "TimeKeeper"));
    if (!timeKeeper)
      {
      vtkErrorMacro("Failed to create 'TimeKeeper' proxy. ");
      return false;
      }
    this->InitializeProxy(timeKeeper);
    timeKeeper->UpdateVTKObjects();

    pxm->RegisterProxy("timekeeper", timeKeeper);
    }

  //---------------------------------------------------------------------------
  // Create the animation-scene (optional)
  vtkSMProxy* animationScene = this->GetAnimationScene(session);
  if (animationScene)
    {
    // create time-animation track (optional)
    this->GetTimeAnimationTrack(animationScene);
    }

  //---------------------------------------------------------------------------
  // Setup global settings/state for the visualization state.

  // Create the GlobalMapperPropertiesProxy (optional)
  // FIXME: these probably should not be created in collaboration mode on
  // non-master nodes.
  vtkSMProxy* proxy = pxm->NewProxy("misc", "GlobalMapperProperties");
  if (proxy)
    {
    this->InitializeProxy(proxy);
    proxy->UpdateVTKObjects();
    proxy->Delete();
    }

  // Create Strict Load Balancing Proxy
  proxy = pxm->NewProxy("misc", "StrictLoadBalancing");
  if (proxy)
    {
    this->InitializeProxy(proxy);
    proxy->UpdateVTKObjects();
    proxy->Delete();
    }

  // Animation properties.
  proxy = pxm->NewProxy("misc", "GlobalAnimationProperties");
  if (proxy)
    {
    this->InitializeProxy(proxy);
    proxy->UpdateVTKObjects();
    proxy->Delete();
    }

  return true;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::FindTimeKeeper(vtkSMSession* session)
{
  assert(session != NULL);

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  assert(pxm);

  return this->FindProxy(pxm, "timekeeper", "misc", "TimeKeeper");
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::FindAnimationScene(vtkSMSession* session)
{
  assert(session != NULL);

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  assert(pxm);

  return this->FindProxy(pxm, "animation", "animation", "AnimationScene");
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::GetAnimationScene(vtkSMSession* session)
{
  assert(session != NULL);

  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  assert(pxm);

  vtkSMProxy* timeKeeper = this->FindTimeKeeper(session);
  if (!timeKeeper)
    {
    return NULL;
    }

  vtkSmartPointer<vtkSMProxy> animationScene = this->FindAnimationScene(session);
  if (!animationScene)
    {
    animationScene.TakeReference(pxm->NewProxy("animation", "AnimationScene"));
    if (animationScene)
      {
      this->PreInitializeProxy(animationScene);
      vtkSMPropertyHelper(animationScene, "TimeKeeper").Set(timeKeeper);
      this->PostInitializeProxy(animationScene);
      animationScene->UpdateVTKObjects();
      pxm->RegisterProxy("animation", animationScene);
      }
    }
  return animationScene.GetPointer();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::FindTimeAnimationTrack(vtkSMProxy* scene)
{
  if (!scene)
    {
    return NULL;
    }

  vtkSMProxy* timeKeeper = this->FindTimeKeeper(scene->GetSession());
  if (!timeKeeper)
    {
    return NULL;
    }

  vtkSMSessionProxyManager* pxm = scene->GetSessionProxyManager();
  assert(pxm);

  vtkSMPropertyHelper helper(scene, "Cues", /*quiet*/ true);
  for (unsigned int cc=0, max=helper.GetNumberOfElements(); cc < max; cc++)
    {
    vtkSMProxy* cue = helper.GetAsProxy(cc);
    if (cue && cue->GetXMLName() &&
      strcmp(cue->GetXMLName(), "TimeAnimationCue") == 0 &&
      vtkSMPropertyHelper(cue, "AnimatedProxy").GetAsProxy() == timeKeeper)
      {
      vtkSMPropertyHelper pnameHelper(cue, "AnimatedPropertyName");
      if (pnameHelper.GetAsString(0) &&
        strcmp(pnameHelper.GetAsString(0), "Time") == 0)
        {
        return cue;
        }
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::GetTimeAnimationTrack(vtkSMProxy* scene)
{
  vtkSmartPointer<vtkSMProxy> cue = this->FindTimeAnimationTrack(scene);
  if (cue || !scene)
    {
    return cue;
    }

  vtkSMProxy* timeKeeper = this->FindTimeKeeper(scene->GetSession());
  if (!timeKeeper)
    {
    return NULL;
    }

  vtkSMSessionProxyManager* pxm = scene->GetSessionProxyManager();
  assert(pxm);

  cue.TakeReference(pxm->NewProxy("animation", "TimeAnimationCue"));
  if (!cue)
    {
    return NULL;
    }

  this->PreInitializeProxy(cue);
  vtkSMPropertyHelper(cue, "AnimatedProxy").Set(timeKeeper);
  vtkSMPropertyHelper(cue, "AnimatedPropertyName").Set("Time");
  this->PostInitializeProxy(cue);
  cue->UpdateVTKObjects();
  pxm->RegisterProxy("animation", cue);

  vtkSMPropertyHelper(scene, "Cues").Add(cue);
  scene->UpdateVTKObjects();

  return cue;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMParaViewPipelineController::CreateView(
    vtkSMSession* session, const char* xmlgroup, const char* xmltype)
{
  if (!session)
    {
    return NULL;
    }

  vtkSmartPointer<vtkSMProxy> view;
  view.TakeReference(session->GetSessionProxyManager()->NewProxy(xmlgroup, xmltype));
  return this->InitializeView(view)? view : NULL;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PreInitializeRepresentation(
  vtkSMProxy* proxy)
{
  return proxy? this->PreInitializeProxyInternal(proxy) : false;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PostInitializeRepresentation(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  vtkInternals::TimeStampsMap::iterator titer =
    this->Internals->InitializationTimeStamps.find(proxy);
  if (titer == this->Internals->InitializationTimeStamps.end())
    {
    vtkErrorMacro(
      "PostInitializePipelineProxy must only be called after "
      "PreInitializePipelineProxy");
    return false;
    }

  this->PostInitializeProxyInternal(proxy);

  // Now register the proxy itself.
  proxy->GetSessionProxyManager()->RegisterProxy("representations", proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PreInitializePipelineProxy(vtkSMProxy* proxy)
{
  return proxy? this->PreInitializeProxyInternal(proxy) : false;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PostInitializePipelineProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  vtkInternals::TimeStampsMap::iterator titer =
    this->Internals->InitializationTimeStamps.find(proxy);
  if (titer == this->Internals->InitializationTimeStamps.end())
    {
    vtkErrorMacro(
      "PostInitializePipelineProxy must only be called after "
      "PreInitializePipelineProxy");
    return false;
    }

  this->PostInitializeProxyInternal(proxy);

  // Create animation helpers for this proxy.
  this->CreateAnimationHelpers(proxy);

  // Now register the proxy itself.
  proxy->GetSessionProxyManager()->RegisterProxy("sources", proxy);

  // Register proxy with TimeKeeper.
  vtkSMProxy* timeKeeper = this->FindTimeKeeper(proxy->GetSession());
  vtkSMPropertyHelper(timeKeeper, "TimeSources").Add(proxy);
  timeKeeper->UpdateVTKObjects();

  // Make the proxy active.
  vtkSMProxySelectionModel* selmodel =
    proxy->GetSessionProxyManager()->GetSelectionModel("ActiveSources");
  assert(selmodel != NULL);
  selmodel->SetCurrentProxy(proxy, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::InitializeView(vtkSMProxy* proxy)
{
  if (!this->InitializeProxy(proxy))
    {
    return false;
    }

  // Now register the proxy itself.
  proxy->GetSessionProxyManager()->RegisterProxy("views", proxy);

  // Register proxy with TimeKeeper.
  vtkSMProxy* timeKeeper = this->FindTimeKeeper(proxy->GetSession());
  vtkSMPropertyHelper(timeKeeper, "Views").Add(proxy);
  timeKeeper->UpdateVTKObjects();

  // Register proxy with AnimationScene (optional)
  vtkSMProxy* scene = this->GetAnimationScene(proxy->GetSession());
  if (scene)
    {
    vtkSMPropertyHelper(scene, "ViewModules").Add(proxy);
    scene->UpdateVTKObjects();
    }

  // Make the proxy active.
  vtkSMProxySelectionModel* selmodel =
    proxy->GetSessionProxyManager()->GetSelectionModel("ActiveView");
  assert(selmodel != NULL);
  selmodel->SetCurrentProxy(proxy, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PreInitializeProxy(vtkSMProxy* proxy)
{
  return proxy? this->PreInitializeProxyInternal(proxy) : false;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PostInitializeProxy(vtkSMProxy* proxy)
{
  return proxy? this->PostInitializeProxyInternal(proxy) : false;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PreInitializeProxyInternal(vtkSMProxy* proxy)
{
  assert(proxy != NULL);

  // 1. Load XML defaults
  //    (already done by NewProxy() call).

  // 2. Create proxies for proxy-list domains.
  if (!this->CreateProxiesForProxyListDomains(proxy))
    {
    return false;
    }

  // 3. Load property values from Settings (FIXME)

  // proxy->LoadPropertyValuesFromSettings();

  // Now, update the initialization time.
  this->Internals->InitializationTimeStamps[proxy].Modified();

  // Note: we need to be careful with *** vtkSMCompoundSourceProxy ***
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PostInitializeProxyInternal(vtkSMProxy* proxy)
{
  assert(proxy != NULL);
  vtkInternals::TimeStampsMap::iterator titer =
    this->Internals->InitializationTimeStamps.find(proxy);
  assert(titer != this->Internals->InitializationTimeStamps.end());

  vtkTimeStamp& ts = titer->second;
  this->Internals->InitializationTimeStamps.erase(titer);

  // ensure everything is up-to-date.
  proxy->UpdateVTKObjects();

  // FIXME: need to figure out how we should really deal with this.
  // We don't reset properties on custom filter.
  if (!proxy->IsA("vtkSMCompoundSourceProxy"))
    {
    if (vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(proxy))
      {
      // Since domains depend on information properties, it's essential we update
      // property information first.
      sourceProxy->UpdatePipelineInformation();
      }

    // Reset property values using domains. However, this should only be done for
    // properties that were not modified between the PreInitializePipelineProxy and
    // PostInitializePipelineProxy calls.

    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(proxy->NewPropertyIterator());

    // iterate over properties and reset them to default. if any property says its
    // domain is modified after we reset it, we need to reset it again since its
    // default may have changed.
    vtkDomainObserver observer;

    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      vtkSMProperty* smproperty = iter->GetProperty();

      if ((smproperty->GetMTime() > ts) ||
        !smproperty->GetInformationOnly())
        {
        vtkPVXMLElement* propHints = iter->GetProperty()->GetHints();
        if (propHints && propHints->FindNestedElementByName("NoDefault"))
          {
          // Don't reset properties that request overriding of the default mechanism.
          continue;
          }

        observer.Monitor(iter->GetProperty());
        iter->GetProperty()->ResetToDomainDefaults();
        }
      }

    const std::set<vtkSMProperty*> &props =
      observer.GetPropertiesWithModifiedDomains();
    for (std::set<vtkSMProperty*>::const_iterator iter = props.begin(); iter != props.end(); ++iter)
      {
      (*iter)->ResetToDomainDefaults();
      }
    proxy->UpdateVTKObjects();
    }

  // Register proxies created for proxy list domains.
  this->RegisterProxiesForProxyListDomains(proxy);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
