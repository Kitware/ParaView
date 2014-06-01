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
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMGlobalPropertiesProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMPropertyLink.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTimeKeeperProxy.h"
#include "vtkWeakPointer.h"

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

  // method to create a new proxy safely i.e. not produce warnings if definition
  // is not available.
  inline vtkSMProxy* vtkSafeNewProxy(vtkSMSessionProxyManager* pxm,
    const char* group, const char* name)
    {
    if (pxm && pxm->GetPrototypeProxy(group, name))
      {
      return pxm->NewProxy(group, name);
      }
    return NULL;
    }
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
vtkStdString vtkSMParaViewPipelineController::GetHelperProxyGroupName(vtkSMProxy* proxy)
{
  assert(proxy != NULL);
  vtksys_ios::ostringstream groupnamestr;
  groupnamestr << "pq_helper_proxies." << proxy->GetGlobalIDAsString();
  return groupnamestr.str();
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
        this->PreInitializeProxy(pld->GetProxy(cc));
        }
      // this is unnecessary here. only done for CompoundSourceProxy instances.
      // those proxies, we generally skip calling "reset" on in
      // PostInitializeProxy(). However, for properties that have proxy
      // list domains, we need to reset them (e.g ProbeLine filter).
      iter->GetProperty()->ResetToDomainDefaults();
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineController::RegisterProxiesForProxyListDomains(
  vtkSMProxy* proxy)
{
  assert(proxy != NULL);
  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();

  std::string groupname = this->GetHelperProxyGroupName(proxy);

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
      vtkSMProxy* plproxy = pld->GetProxy(cc);

      // Handle proxy-list hints.
      this->ProcessProxyListProxyHints(proxy, plproxy);

      this->PostInitializeProxy(plproxy);
      pxm->RegisterProxy(groupname.c_str(), iter->GetKey(), plproxy);
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::SetupGlobalPropertiesLinks(vtkSMProxy* proxy)
{
  assert(proxy != NULL);

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();

  vtkSmartPointer<vtkSMPropertyIterator> iter;
  iter.TakeReference(proxy->NewPropertyIterator());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* prop = iter->GetProperty();
    assert(prop);

    vtkPVXMLElement* linkHint = prop->GetHints()?
      prop->GetHints()->FindNestedElementByName("GlobalPropertyLink") : NULL;
    if (linkHint)
      {
      std::string type = linkHint->GetAttributeOrEmpty("type");
      std::string property = linkHint->GetAttributeOrEmpty("property");
      if (type.empty() || property.empty())
        {
        vtkWarningMacro("Invalid GlobalPropertyLink hint.");
        continue;
        }

      vtkSMGlobalPropertiesProxy* gpproxy = vtkSMGlobalPropertiesProxy::SafeDownCast(
        pxm->GetProxy("global_properties", type.c_str()));
      if (!gpproxy)
        {
        continue;
        }
      if (!gpproxy->Link(property.c_str(), proxy, iter->GetKey()))
        {
        vtkWarningMacro("Failed to setup GlobalPropertyLink.");
        }
      }

    // Check for PropertyLinks
    linkHint = prop->GetHints()?
      prop->GetHints()->FindNestedElementByName("PropertyLink") : NULL;
    if (linkHint)
      {
      const char* sourceGroupName = linkHint->GetAttributeOrEmpty("group");
      const char* sourceProxyName = linkHint->GetAttributeOrEmpty("proxy");
      const char* sourcePropertyName = linkHint->GetAttributeOrEmpty("property");
      if (!sourceGroupName || !sourceProxyName || !sourcePropertyName)
        {
        continue;
        }

      vtkSMProxy* sourceProxy = pxm->GetProxy(sourceGroupName, sourceProxyName);
      if (!sourceProxy)
        {
        continue;
        }

      vtkSMProperty* sourceProperty =  sourceProxy->GetProperty(sourcePropertyName);
      if (sourceProperty)
        {
        sourceProperty->AddLinkedProperty(prop);
        }
      else
        {
        vtkWarningMacro(<< "No source property with group \"" << sourceGroupName
                        << "\", proxy \"" << sourceProxyName
                        << "\", property \"" << sourcePropertyName
                        << "\" exists. Linking not performed.")
        }
      }
    }
  return true;
}

namespace vtkSMParaViewPipelineControllerNS
{
  //---------------------------------------------------------------------------
  class vtkSMLinkObserver : public vtkCommand
  {
public:
  vtkWeakPointer<vtkSMProperty> Output;
  typedef vtkCommand Superclass;
  virtual const char* GetClassNameInternal() const
    { return "vtkSMLinkObserver"; }
  static vtkSMLinkObserver* New()
    {
    return new vtkSMLinkObserver();
    }
  virtual void Execute(vtkObject* caller, unsigned long event, void* calldata)
    {
    (void)event;
    (void)calldata;
    vtkSMProperty* input = vtkSMProperty::SafeDownCast(caller);
    if (input && this->Output)
      {
      // this will copy both checked and unchecked property values.
      this->Output->Copy(input);
      }
    }

  static void LinkProperty(vtkSMProperty* input,
    vtkSMProperty* output)
    {
    if (input && output)
      {
      vtkSMLinkObserver* observer = vtkSMLinkObserver::New();
      observer->Output = output;
      input->AddObserver(vtkCommand::PropertyModifiedEvent, observer);
      input->AddObserver(vtkCommand::UncheckedPropertyModifiedEvent, observer);
      observer->FastDelete();
      output->Copy(input);
      }
    }
  };

}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineController::ProcessProxyListProxyHints(
  vtkSMProxy* parent, vtkSMProxy* plproxy)
{
  vtkPVXMLElement* proxyListElement = plproxy->GetHints()?
    plproxy->GetHints()->FindNestedElementByName("ProxyList") : NULL;
  if (!proxyListElement)
    {
    return;
    }
  for (unsigned int cc=0, max = proxyListElement->GetNumberOfNestedElements();
    cc < max; ++cc)
    {
    vtkPVXMLElement* child = proxyListElement->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Link") == 0)
      {
      const char* name = child->GetAttribute("name");
      const char* linked_with = child->GetAttribute("with_property");
      if (name && linked_with)
        {
        vtkSMParaViewPipelineControllerNS::vtkSMLinkObserver::LinkProperty(
          parent->GetProperty(linked_with), plproxy->GetProperty(name));
        }
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

  std::string groupname = this->GetHelperProxyGroupName(proxy);

  for (unsigned int cc=0, max=source->GetNumberOfOutputPorts(); cc < max; cc++)
    {
    vtkSmartPointer<vtkSMProxy> helper;
    helper.TakeReference(vtkSafeNewProxy(pxm, "misc", "RepresentationAnimationHelper"));
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
  // server before we start creating "essential" proxies. This is a no-op if not
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
    timeKeeper.TakeReference(vtkSafeNewProxy(pxm, "misc", "TimeKeeper"));
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
  vtkSMProxy* proxy = vtkSafeNewProxy(pxm, "misc", "GlobalMapperProperties");
  if (proxy)
    {
    this->InitializeProxy(proxy);
    proxy->UpdateVTKObjects();
    proxy->Delete();
    }

  // Create Strict Load Balancing Proxy
  proxy = vtkSafeNewProxy(pxm, "misc", "StrictLoadBalancing");
  if (proxy)
    {
    this->InitializeProxy(proxy);
    proxy->UpdateVTKObjects();
    proxy->Delete();
    }

  this->UpdateSettingsProxies(session);

  //---------------------------------------------------------------------------
  // Setup color palette and proxies for other global property groups (optional)
  proxy = pxm->GetProxy("global_properties", "ColorPalette");
  if (!proxy)
    {
    proxy = vtkSafeNewProxy(pxm, "misc", "ColorPalette");
    if (proxy)
      {
      this->InitializeProxy(proxy);
      pxm->RegisterProxy("global_properties", "ColorPalette", proxy);
      proxy->UpdateVTKObjects();
      proxy->Delete();
      }
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
    animationScene.TakeReference(vtkSafeNewProxy(pxm, "animation", "AnimationScene"));
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

  cue.TakeReference(vtkSafeNewProxy(pxm, "animation", "TimeAnimationCue"));
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
bool vtkSMParaViewPipelineController::RegisterPipelineProxy(
  vtkSMProxy* proxy, const char* proxyname)
{
  if (!proxy)
    {
    return false;
    }

  // Create animation helpers for this proxy.
  this->CreateAnimationHelpers(proxy);

  // Now register the proxy itself.
  // If proxyname is NULL, the proxy manager makes up a name.
  proxy->GetSessionProxyManager()->RegisterProxy("sources", proxyname, proxy);

  // Register proxy with TimeKeeper.
  vtkSMProxy* timeKeeper = this->FindTimeKeeper(proxy->GetSession());
  vtkSMTimeKeeperProxy::AddTimeSource(timeKeeper, proxy,
    /*suppress_input*/ (
      proxy->GetProperty("TimestepValues") != NULL ||
      proxy->GetProperty("TimeRange") != NULL));

  // Make the proxy active.
  vtkSMProxySelectionModel* selmodel =
    proxy->GetSessionProxyManager()->GetSelectionModel("ActiveSources");
  assert(selmodel != NULL);
  selmodel->SetCurrentProxy(proxy, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::UnRegisterPipelineProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  const char* _proxyname = pxm->GetProxyName("sources", proxy);
  if (_proxyname == NULL)
    {
    return false;
    }
  const std::string proxyname(_proxyname);

  // ensure proxy is no longer active.
  vtkSMProxySelectionModel* selmodel = pxm->GetSelectionModel("ActiveSources");
  assert(selmodel != NULL);
  if (selmodel->GetCurrentProxy() == proxy)
    {
    selmodel->SetCurrentProxy(NULL, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
    }

  // remove proxy from TimeKeeper.
  vtkSMProxy* timeKeeper = this->FindTimeKeeper(proxy->GetSession());
  vtkSMTimeKeeperProxy::RemoveTimeSource(timeKeeper, proxy,
    /*unsuppress_input*/ (
      proxy->GetProperty("TimestepValues") != NULL ||
      proxy->GetProperty("TimeRange") != NULL));

  // unregister dependencies.
  this->UnRegisterDependencies(proxy);

  // this will remove both proxy-list-domain helpers and animation helpers.
  this->FinalizeProxy(proxy);

  // unregister the proxy.
  pxm->UnRegisterProxy("sources", proxyname.c_str(), proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::RegisterViewProxy(vtkSMProxy* proxy)
{
  if (!proxy)
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
bool vtkSMParaViewPipelineController::UnRegisterViewProxy(
  vtkSMProxy* proxy, bool unregister_representations/*=true*/)
{
  if (!proxy)
    {
    return false;
    }

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  const char* _proxyname = pxm->GetProxyName("views", proxy);
  if (_proxyname == NULL)
    {
    return false;
    }
  const std::string proxyname(_proxyname);

  // ensure proxy is no longer active.
  vtkSMProxySelectionModel* selmodel = pxm->GetSelectionModel("ActiveView");
  assert(selmodel != NULL);
  if (selmodel->GetCurrentProxy() == proxy)
    {
    selmodel->SetCurrentProxy(NULL, vtkSMProxySelectionModel::CLEAR_AND_SELECT);
    }

  // remove proxy from AnimationScene (optional)
  vtkSMProxy* scene = this->GetAnimationScene(proxy->GetSession());
  if (scene)
    {
    vtkSMPropertyHelper(scene, "ViewModules").Remove(proxy);
    scene->UpdateVTKObjects();
    }

  // remove proxy from TimeKeeper.
  vtkSMProxy* timeKeeper = this->FindTimeKeeper(proxy->GetSession());
  vtkSMPropertyHelper(timeKeeper, "Views").Remove(proxy);
  timeKeeper->UpdateVTKObjects();

  // remove all representation proxies.
  const char* pnames[]={"Representations", "HiddenRepresentations",
    "Props", "HiddenProps", NULL};
  for (int index=0; unregister_representations && (pnames[index] != NULL); ++index)
    {
    vtkSMProperty* prop = proxy->GetProperty(pnames[index]);
    if (prop == NULL)
      {
      continue;
      }

    typedef std::vector<vtkWeakPointer<vtkSMProxy> > proxyvectortype;
    proxyvectortype reprs;

    vtkSMPropertyHelper helper(prop);
    for (unsigned int cc=0, max=helper.GetNumberOfElements(); cc < max ; ++cc)
      {
      reprs.push_back(helper.GetAsProxy(cc));
      }

    for (proxyvectortype::iterator iter=reprs.begin(), end=reprs.end(); iter!=end; ++iter)
      {
      if (iter->GetPointer())
        {
        this->UnRegisterProxy(iter->GetPointer());
        }
      }
    }

  // unregister dependencies.
  this->UnRegisterDependencies(proxy);

  // this will remove both proxy-list-domain helpers and animation helpers.
  this->FinalizeProxy(proxy);

  pxm->UnRegisterProxy("views", proxyname.c_str(), proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::RegisterRepresentationProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  // Register the proxy itself.
  proxy->GetSessionProxyManager()->RegisterProxy("representations", proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::UnRegisterRepresentationProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  std::string groupname("representations");
  const char* _proxyname = pxm->GetProxyName(groupname.c_str(), proxy);
  if (_proxyname == NULL)
    {
    groupname = "scalar_bars";
    _proxyname = pxm->GetProxyName(groupname.c_str(), proxy);
    if (_proxyname == NULL)
      {
      return false;
      }
    }
  const std::string proxyname(_proxyname);

  //---------------------------------------------------------------------------
  // remove the representation from any views.
  typedef std::vector<std::pair<vtkSMProxy*, vtkSMProperty*> > viewsvector;
  viewsvector views;
  for (unsigned int cc=0, max=proxy->GetNumberOfConsumers(); cc < max; cc++)
    {
    vtkSMProxy* consumer = proxy->GetConsumerProxy(cc);
    consumer = consumer? consumer->GetTrueParentProxy() : NULL;
    if (consumer && consumer->IsA("vtkSMViewProxy") && proxy->GetConsumerProperty(cc))
      {
      views.push_back(
        std::pair<vtkSMProxy*, vtkSMProperty*>(
          consumer, proxy->GetConsumerProperty(cc)));
      }
    }
  for (viewsvector::iterator iter=views.begin(), max=views.end(); iter != max; ++iter)
    {
    if (iter->first && iter->second)
      {
      vtkSMPropertyHelper(iter->second).Remove(proxy);
      iter->first->UpdateVTKObjects();
      }
    }

  // unregister dependencies.
  this->UnRegisterDependencies(proxy);

  // this will remove both proxy-list-domain helpers and animation helpers.
  this->FinalizeProxy(proxy);

  proxy->GetSessionProxyManager()->UnRegisterProxy(
    groupname.c_str(), proxyname.c_str(), proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::RegisterColorTransferFunctionProxy(
  vtkSMProxy* proxy, const char* proxyname)
{
  if (!proxy)
    {
    return false;
    }

  proxy->GetSessionProxyManager()->RegisterProxy(
    "lookup_tables", proxyname, proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::RegisterOpacityTransferFunction(
  vtkSMProxy* proxy, const char* proxyname)
{
  if (!proxy)
    {
    return false;
    }

  proxy->GetSessionProxyManager()->RegisterProxy(
    "piecewise_functions", proxyname, proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::RegisterAnimationProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  proxy->GetSessionProxyManager()->RegisterProxy("animation", proxy);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::UnRegisterAnimationProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  const char* _proxyname = pxm->GetProxyName("animation", proxy);
  if (_proxyname == NULL)
    {
    return false;
    }
  const std::string proxyname(_proxyname);

  //---------------------------------------------------------------------------
  // Animation proxies are typically added to some other animation proxy. We
  // need to remove it from that proxy.
  typedef std::pair<vtkWeakPointer<vtkSMProxy>, vtkWeakPointer<vtkSMProperty> > proxypairitemtype;
  typedef std::vector<proxypairitemtype> proxypairvectortype;
  proxypairvectortype consumers;
  for (unsigned int cc=0, max=proxy->GetNumberOfConsumers(); cc<max; ++cc)
    {
    vtkSMProxy* consumer = proxy->GetConsumerProxy(cc);
    consumer = consumer? consumer->GetTrueParentProxy() : NULL;
    if (proxy->GetConsumerProperty(cc) &&
      consumer && consumer->GetXMLGroup() &&
      strcmp(consumer->GetXMLGroup(), "animation") ==0)
      {
      consumers.push_back(proxypairitemtype(consumer, proxy->GetConsumerProperty(cc)));
      }
    }
  for (proxypairvectortype::iterator iter=consumers.begin(), max=consumers.end();
    iter != max; ++iter)
    {
    if (iter->first && iter->second)
      {
      vtkSMPropertyHelper(iter->second).Remove(proxy);
      iter->first->UpdateVTKObjects();
      }
    }

  //---------------------------------------------------------------------------
  // destroy keyframes, if there are any.
  typedef std::vector<vtkWeakPointer<vtkSMProxy> > proxyvectortype;
  proxyvectortype keyframes;
  if (vtkSMProperty* kfProperty = proxy->GetProperty("KeyFrames"))
    {
    vtkSMPropertyHelper helper(kfProperty);
    for (unsigned int cc=0, max=helper.GetNumberOfElements(); cc<max;++cc)
      {
      keyframes.push_back(helper.GetAsProxy(cc));
      }
    }

  // unregister dependencies.
  this->UnRegisterDependencies(proxy);

  // this will remove both proxy-list-domain helpers and animation helpers.
  this->FinalizeProxy(proxy);

  pxm->UnRegisterProxy("animation", proxyname.c_str(), proxy);

  // now destroy all keyframe proxies.
  for (proxyvectortype::iterator iter=keyframes.begin(), max=keyframes.end();
    iter != max;++iter)
    {
    if (iter->GetPointer())
      {
      this->UnRegisterProxy(iter->GetPointer());
      }
    }
  return true;
}

void vtkSMParaViewPipelineController::UpdateSettingsProxies(vtkSMSession* session)
{
  // Set up the settings proxies
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkSMProxyDefinitionManager* pdm = pxm->GetProxyDefinitionManager();
  vtkPVProxyDefinitionIterator* iter = pdm->NewSingleGroupIterator( "settings" );
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMProxy* proxy = pxm->GetProxy(iter->GetGroupName(), iter->GetProxyName());
    if (!proxy)
      {
      proxy = vtkSafeNewProxy(pxm, iter->GetGroupName(), iter->GetProxyName());
      if (proxy)
        {
        this->InitializeProxy(proxy);
        pxm->RegisterProxy(iter->GetGroupName(), iter->GetProxyName(), proxy);
        proxy->UpdateVTKObjects();
        proxy->Delete();
        }
      }
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PreInitializeProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  // 1. Load XML defaults
  //    (already done by NewProxy() call).

  // 2. Create proxies for proxy-list domains.
  if (!this->CreateProxiesForProxyListDomains(proxy))
    {
    return false;
    }

  // 3. Process global properties hints.
  if (!this->SetupGlobalPropertiesLinks(proxy))
    {
    return false;
    }

  // 4. Load property values from Settings.
  vtkSMSettings * settings = vtkSMSettings::GetInstance();
  settings->GetProxySettings(proxy);

  // Now, update the initialization time.
  this->Internals->InitializationTimeStamps[proxy].Modified();

  // Note: we need to be careful with *** vtkSMCompoundSourceProxy ***
  return true;
}

//----------------------------------------------------------------------------
unsigned long vtkSMParaViewPipelineController::GetInitializationTime(vtkSMProxy* proxy)
{
  vtkInternals::TimeStampsMap::iterator titer =
    this->Internals->InitializationTimeStamps.find(proxy);
  return (titer == this->Internals->InitializationTimeStamps.end()? 0: titer->second);
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::PostInitializeProxy(vtkSMProxy* proxy)
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
      "PostInitializeProxy must only be called after PreInitializeProxy");
    return false;
    }

  assert(titer != this->Internals->InitializationTimeStamps.end());

  vtkTimeStamp ts = titer->second;
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
      if (sourceProxy->GetNumberOfOutputPorts() > 0)
        {
        // This is only done for non-writers, or non-sinks.
        sourceProxy->UpdatePipelineInformation();
        }
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

      if ((smproperty->GetMTime() > ts) || smproperty->GetInformationOnly())
        {
        // Property was modified between since the PreInitializeProxy() call. We
        // leave it untouched.
        continue;
        }

      vtkPVXMLElement* propHints = smproperty->GetHints();
      if (propHints && propHints->FindNestedElementByName("NoDefault"))
        {
        // Don't reset properties that request overriding of the default mechanism.
        continue;
        }

      observer.Monitor(iter->GetProperty());
      iter->GetProperty()->ResetToDomainDefaults();
      }

    const std::set<vtkSMProperty*> &props =
      observer.GetPropertiesWithModifiedDomains();
    for (std::set<vtkSMProperty*>::const_iterator siter = props.begin(); siter != props.end(); ++siter)
      {
      (*siter)->ResetToDomainDefaults();
      }
    proxy->UpdateVTKObjects();
    }

  // Register proxies created for proxy list domains.
  this->RegisterProxiesForProxyListDomains(proxy);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::FinalizeProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  //---------------------------------------------------------------------------
  // Unregister all helper proxies. This will "unregister" all the helper
  // proxies which ensures that any "dependent" proxies for those helpers
  // also get removed.
  std::string groupname = this->GetHelperProxyGroupName(proxy);

  typedef std::pair<std::string, vtkWeakPointer<vtkSMProxy> > proxymapitemtype;
  typedef std::vector<proxymapitemtype> proxymaptype;
  proxymaptype proxymap;

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
    {
    vtkNew<vtkSMProxyIterator> iter;
    iter->SetSessionProxyManager(pxm);
    iter->SetModeToOneGroup();
    for (iter->Begin(groupname.c_str()); !iter->IsAtEnd(); iter->Next())
      {
      proxymap.push_back(proxymapitemtype(
          iter->GetKey(), iter->GetProxy()));
      }
    }

  for (proxymaptype::iterator iter = proxymap.begin(), end = proxymap.end();
    iter != end; ++iter)
    {
    if (iter->second)
      {
      // remove any dependencies for this proxy.
      this->UnRegisterDependencies(iter->second);
      // remove any helpers for this proxy.
      this->FinalizeProxy(iter->second);
      // unregister the proxy.
      pxm->UnRegisterProxy(
        groupname.c_str(), iter->first.c_str(), iter->second);
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::UnRegisterDependencies(vtkSMProxy* proxy)
{
  assert(proxy != NULL);

  //---------------------------------------------------------------------------
  // Before going any further build a list of all consumer proxies
  // (not pointing to internal/sub-proxies).

  typedef std::vector<vtkWeakPointer<vtkSMProxy> > proxyvectortype;
  proxyvectortype consumers;

  for (unsigned int cc=0, max = proxy->GetNumberOfConsumers(); cc < max; ++cc)
    {
    vtkSMProxy* consumer = proxy->GetConsumerProxy(cc);
    consumer = consumer? consumer->GetTrueParentProxy() : NULL;
    if (consumer)
      {
      consumers.push_back(consumer);
      }
    }

  //---------------------------------------------------------------------------
  for (proxyvectortype::iterator iter = consumers.begin(), max = consumers.end();
    iter != max; ++iter)
    {
    vtkSMProxy* consumer = iter->GetPointer();
    if (!consumer)
      {
      continue;
      }

    if (
      // Remove representations connected to this proxy, if any.
      consumer->IsA("vtkSMRepresentationProxy") ||

      // Remove any animation cues for this proxy
      strcmp(consumer->GetXMLGroup(), "animation") == 0 ||

      false)
      {
      this->UnRegisterProxy(consumer);
      }
    }
  //---------------------------------------------------------------------------

  // TODO: remove any property links/proxy link setup for this proxy.

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::UnRegisterProxy(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  // determine what type of proxy is this, based on that, we can finalize it.
  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  if (pxm->GetProxyName("sources", proxy))
    {
    return this->UnRegisterPipelineProxy(proxy);
    }
  else if (pxm->GetProxyName("representations", proxy) ||
           pxm->GetProxyName("scalar_bars", proxy))
    {
    return this->UnRegisterRepresentationProxy(proxy);
    }
  else if (pxm->GetProxyName("views", proxy))
    {
    return this->UnRegisterViewProxy(proxy);
    }
  else if (pxm->GetProxyName("animation", proxy))
    {
    if (proxy != this->GetAnimationScene(proxy->GetSession()))
      {
      return this->UnRegisterAnimationProxy(proxy);
      }
    }
  else
    {
    const char* known_groups[] = {
      "lookup_tables", "piecewise_functions", "layouts", NULL };
    for (int cc=0; known_groups[cc] != NULL; ++cc)
      {
      if (const char* pname = pxm->GetProxyName(known_groups[cc], proxy))
        {
        // unregister dependencies.
        if (!this->UnRegisterDependencies(proxy))
          {
          return false;
          }

        // this will remove both proxy-list-domain helpers and animation helpers.
        if (!this->FinalizeProxy(proxy))
          {
          return false;
          }
        pxm->UnRegisterProxy(known_groups[cc], pname, proxy);
        return true;
        }
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineController::ResetSession(vtkSMSession* session)
{
  if (!session)
    {
    return false;
    }
  // remove all proxies except this animation scene and time keeper.
  std::set<vtkSMProxy*> to_preserve;
  to_preserve.insert(this->FindTimeKeeper(session));

  typedef std::vector<vtkWeakPointer<vtkSMProxy> > proxyvectortype;
  proxyvectortype proxies;

  vtkNew<vtkSMProxyIterator> iter;
  iter->SetSessionProxyManager(session->GetSessionProxyManager());
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProxy* proxy = iter->GetProxy();
    if (proxy != NULL &&
        to_preserve.find(proxy) == to_preserve.end())
      {
      proxies.push_back(proxy);
      }
    }
  for (proxyvectortype::iterator piter = proxies.begin(), max=proxies.end(); piter != max; ++piter)
    {
    if (piter->GetPointer())
      {
      this->UnRegisterProxy(piter->GetPointer());
      }
    }

  // Now create new time-animation track.
  this->InitializeSession(session);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
