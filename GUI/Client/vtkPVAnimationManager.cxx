/*=========================================================================

  Program:   ParaView
  Module:    vtkPVAnimationManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationManager.h"
#include "vtkObjectFactory.h"

#include "vtkAnimationScene.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkPVVerticalAnimationInterface.h"
#include "vtkPVHorizontalAnimationInterface.h"
#include "vtkPVAnimationScene.h"
#include "vtkKWFrame.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSmartPointer.h"
#include "vtkString.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMVectorProperty.h"
#include "vtkCommand.h"
#include "vtkKWEvent.h"
#include "vtkPVKeyFrame.h"
#include "vtkPVRampKeyFrame.h"
#include "vtkPVBooleanKeyFrame.h"
#include "vtkPVExponentialKeyFrame.h"
#include "vtkPVSinusoidKeyFrame.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMessageDialog.h"
#include "vtkKWLabeledEntry.h"
#include "vtkPVRenderView.h"
#include "vtkKWEntry.h"
#include "vtkProcessModule.h"
#include "vtkPVReaderModule.h"
#include "vtkCollectionIterator.h"
#include "vtkPVWidget.h"
#include <vtkstd/map>
#include <vtkstd/string>

#define VTK_PV_ANIMATION_GROUP "animateable"

vtkStandardNewMacro(vtkPVAnimationManager);
vtkCxxRevisionMacro(vtkPVAnimationManager, "1.20");
vtkCxxSetObjectMacro(vtkPVAnimationManager, HorizantalParent, vtkKWWidget);
vtkCxxSetObjectMacro(vtkPVAnimationManager, VerticalParent, vtkKWWidget);
//*****************************************************************************
class vtkPVAnimationManagerObserver : public vtkCommand
{
public:
  static vtkPVAnimationManagerObserver* New()
    {
    return new vtkPVAnimationManagerObserver;
    }

  void SetTarget (vtkPVAnimationManager* t)
    {
    this->Target = t;
    }
  virtual void Execute(vtkObject* obj, unsigned long event, void* calldata)
    {
    if(this->Target)
      {
      this->Target->ExecuteEvent(obj, event, calldata);
      }
    
    }
protected:
  vtkPVAnimationManagerObserver()
    {
    this->Target = NULL;
    }
  vtkPVAnimationManager* Target;
};

//*****************************************************************************
class vtkPVAnimationManagerInternals
{
public:
//  typedef vtkstd::map<vtkstd::string, vtkSmartPointer<vtkPVAnimationCue*>>
//  we don't reference count it here as HAnimationInterface will reference count it.
  typedef vtkstd::map<vtkstd::string, vtkPVAnimationCue*>
    StringToPVCueMap;
  StringToPVCueMap PVAnimationCues;
};
//*****************************************************************************
//-----------------------------------------------------------------------------
vtkPVAnimationManager::vtkPVAnimationManager()
{
  this->HorizantalParent = NULL;
  this->VerticalParent = NULL;
  this->VAnimationInterface = vtkPVVerticalAnimationInterface::New();
  this->VAnimationInterface->SetTraceReferenceObject(this);
  this->VAnimationInterface->SetTraceReferenceCommand("GetVAnimationInterface");
  
  this->HAnimationInterface = vtkPVHorizontalAnimationInterface::New();
  this->HAnimationInterface->SetTraceReferenceObject(this);
  this->HAnimationInterface->SetTraceReferenceCommand("GetHAnimationInterface");
  
  
  this->AnimationScene = vtkPVAnimationScene::New();
  this->AnimationScene->SetTraceReferenceObject(this);
  this->AnimationScene->SetTraceReferenceCommand("GetAnimationScene");

  this->ProxyIterator = vtkSMProxyIterator::New();
  this->Internals = new vtkPVAnimationManagerInternals;
  this->Observer = vtkPVAnimationManagerObserver::New();
  this->Observer->SetTarget(this);
  this->RecordAll = 1;
  this->InRecording = 0;
  this->RecordingIncrement = 0.1;

  this->AdvancedView = 0;
  this->OverrideCache = 0;
}

//-----------------------------------------------------------------------------
vtkPVAnimationManager::~vtkPVAnimationManager()
{
  this->SetVerticalParent(0);
  this->SetHorizantalParent(0);
  this->VAnimationInterface->Delete();
  this->HAnimationInterface->Delete();
  this->AnimationScene->Delete();
  this->ProxyIterator->Delete();
  delete this->Internals;
  this->Observer->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::Create(vtkKWApplication* app, const char* )
{

  if (this->IsCreated())
    {
    vtkErrorMacro("Widget already created.");
    return;
    }
  if (!this->VerticalParent || !this->HorizantalParent)
    {
    vtkErrorMacro("VerticalParent and HorizantalParent must be set before calling create.");
    return;
    }
  this->Superclass::Create(app, NULL, NULL);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();

  if (pvApp->HasRegisteryValue(2,"RunTime","AdvancedAnimationView"))
    {
    this->AdvancedView = pvApp->GetIntRegisteryValue(2, "RunTime", "AdvancedAnimationView");
    }

  this->HAnimationInterface->SetParent(this->HorizantalParent);
  this->HAnimationInterface->Create(app, "-relief flat");

  this->VAnimationInterface->SetParent(this->VerticalParent);
  this->VAnimationInterface->SetAnimationManager(this);
  this->VAnimationInterface->Create(app, "-relief flat");

  this->AnimationScene->SetParent(
    this->VAnimationInterface->GetScenePropertiesFrame());
  this->AnimationScene->SetAnimationManager(this);
  this->AnimationScene->SetWindow(pvWin);
  this->AnimationScene->SetRenderView(pvWin->GetMainView());
  this->AnimationScene->Create(app, "-relief flat");

  this->Script("pack %s -anchor n -side top -expand t -fill both",
    this->AnimationScene->GetWidgetName());

}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SetAdvancedView(int advanced)
{
  if (this->AdvancedView == advanced)
    {
    return;
    }
  this->AdvancedView = advanced;
  this->Update();
  this->GetApplication()->SetRegisteryValue(2, "RunTime",
    "AdvancedAnimationView", "%d", advanced);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SetTimeMarker(double time)
{
  this->HAnimationInterface->SetTimeMarker(time);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ShowAnimationInterfaces()
{
  this->Update();
  this->ShowVAnimationInterface();
  this->ShowHAnimationInterface();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ShowVAnimationInterface()
{
  if (!this->VAnimationInterface->IsPacked())
    {
    this->VAnimationInterface->UnpackSiblings();
    this->Script("pack %s -anchor n -side top -expand t -fill both",
      this->VAnimationInterface->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ShowHAnimationInterface()
{
  if (!this->HAnimationInterface->IsPacked())
    {
    this->Script("pack %s -anchor n -side top -expand t -fill both",
      this->HAnimationInterface->GetWidgetName());
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::Update()
{
  if (!this->IsCreated())
    {
    return;
    }
  this->ValidateAndAddSpecialCues();

  //1) validate if any of the old sources have disappeared.
  this->ValidateOldSources();
  //2) add any new sources.
  this->AddNewSources();
  this->HAnimationInterface->GetParentTree()->UpdateCueVisibility(
    this->AdvancedView); 
  this->Script("update");
  this->HAnimationInterface->ResizeCallback();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ValidateOldSources()
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow *pvWindow = (pvApp)? pvApp->GetMainWindow() : NULL;
  vtkSMProxyManager *proxyManager = vtkSMObject::GetProxyManager();
  if (!pvWindow)
    {
    return;
    }
  int verified_until = 0;
  int current_index = 0;
  vtkPVAnimationManagerInternals::StringToPVCueMap::iterator iter;
  for (iter = this->Internals->PVAnimationCues.begin();
    iter != this->Internals->PVAnimationCues.end();current_index++)
    {
    if (current_index < verified_until)
      { 
      iter++; 
      continue;
      }

    int deleted = 0;
    const char* sourcekey = iter->first.c_str();
    char* listname = this->GetSourceListName(sourcekey);
    char* sourcename = this->GetSourceName(sourcekey);
    char* subsourcename = this->GetSubSourceName(sourcekey);

    vtkPVSource* pvSource = pvWindow->GetPVSource(listname, sourcename);
    
    if (strcmp(listname,"_dont_validate_") == 0)
      {
      // provides for special cues that are not PVSources.
      // such as Camera. These are not validate by this generic method.
      // We can validate and add them separately, if needed.
      }
    else if (pvSource == NULL)
      {
      // the source has been deleted.
      if (subsourcename == NULL)
        {
        vtkPVAnimationCueTree* pvCueTree = vtkPVAnimationCueTree::SafeDownCast(
          iter->second);
        this->HAnimationInterface->RemoveAnimationCueTree(pvCueTree);
        //deletes all the subsources as well.
        }
      // if it is a subsource, then it will get deleted when the parent will be deleted.
      this->Internals->PVAnimationCues.erase(iter);
      deleted = 1;
      }
    else if (pvSource && subsourcename && 
      proxyManager->GetProxy(VTK_PV_ANIMATION_GROUP,sourcekey) == NULL)
      {
      // some pvSource (actually some widget in the source) might have
      // unregistered this proxy. So remove it.
      vtkPVAnimationCueTree* pvParent = this->GetAnimationCueTreeForSource(pvSource);
      if (pvParent)
        {
        pvParent->RemoveChild(iter->second);
        this->Internals->PVAnimationCues.erase(iter);
        deleted = 1;
        }
      else
        {
        vtkErrorMacro("Failed to find parent tree.");
        }
      }
    else if (pvSource && subsourcename == NULL)
      {
      // ensure that the label for the source and that of the cue are in sync.
      char *label = pvApp->GetTextRepresentation(pvSource);
      iter->second->SetLabelText(label);
      delete []label;     
      }
    if (deleted)
      {
      current_index = -1;
      iter = this->Internals->PVAnimationCues.begin(); 
      }
    else 
      {
      iter++;
      verified_until++;
      }
    delete [] listname;
    delete [] sourcename;
    delete [] subsourcename;
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::AddNewSources()
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow *pvWindow = (pvApp)? pvApp->GetMainWindow() : NULL;
  if (!pvWindow)
    {
    return;
    }
  
  this->ProxyIterator->Begin(VTK_PV_ANIMATION_GROUP);
  this->ProxyIterator->SetMode(vtkSMProxyIterator::ONE_GROUP);
  for( ;!this->ProxyIterator->IsAtEnd(); this->ProxyIterator->Next() )
    {
    vtkSMProxy* proxy = this->ProxyIterator->GetProxy();
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    const char* proxyname = pxm->GetProxyName(VTK_PV_ANIMATION_GROUP, proxy);
    char* listname = this->GetSourceListName(proxyname);
    char* sourcename = this->GetSourceName(proxyname);
    char* subsourcename = this->GetSubSourceName(proxyname);

    if (strcmp(listname, "GlyphSources") == 0)
      {
      // we don't animate glyph sources.
      delete [] listname;
      delete [] sourcename;
      delete [] subsourcename;
      continue;
      }
    //Check is the sourcename already exists.
    if (this->Internals->PVAnimationCues.find(proxyname) !=
      this->Internals->PVAnimationCues.end())
      {
      //already addded.
      delete [] listname;
      delete [] sourcename;
      delete [] subsourcename;
      continue;
      }

    //new source.
    vtkPVSource* pvSource = pvWindow->GetPVSource(listname, sourcename);
    if (!pvSource)
      {
      vtkDebugMacro("Dangling proxy " << proxyname << ". Source may be yet to be added.");
      delete [] listname;
      delete [] sourcename;
      delete [] subsourcename;
      continue;
      }

    vtkPVAnimationCueTree *pvCue = vtkPVAnimationCueTree::New();
    if (subsourcename)
      {
      pvCue->SetLabelText(subsourcename);
      }
    else
      {
      char *label = pvApp->GetTextRepresentation(pvSource);
      pvCue->SetLabelText(label);
      delete []label;
      }

    // Determine the parent of this tree node.
    // At the same time, determine the unique name for it.
    vtkPVAnimationCueTree* pvParentTree = NULL;
    if (subsourcename!=NULL)
      {
      char* sourcekey = this->GetSourceKey(proxyname);
      pvParentTree = vtkPVAnimationCueTree::SafeDownCast(
        this->Internals->PVAnimationCues[sourcekey]);
      delete [] sourcekey;
      
      if (!pvParentTree)
        {
        vtkErrorMacro("Error while building animatable objects list!" << proxyname);
        delete [] listname;
        delete [] sourcename;
        delete [] subsourcename;
        pvCue->Delete();
        continue;
        }
      else
        {
        // Set the pvSource for the source tree so that 
        // GetAnimationCueTreeForSource can work properly.
        pvCue->SetPVSource(pvSource);
        pvCue->SetName(proxyname);
        pvParentTree->AddChild(pvCue);
        }
      }
    else
      {
      pvCue->SetName(proxyname);
      pvCue->SetPVSource(pvSource);
      this->HAnimationInterface->AddAnimationCueTree(pvCue);
      }

    this->InitializeObservers(pvCue);
    pvCue->Delete();
    /* 
     * TODO: should only add proxies that have children.
     * This gets complicated as the base proxy may not have 
     * any nodes, but the source may have other proxies that have 
     * properties.
     */
    if (this->AddProperties(pvSource, proxy, pvCue) || !pvParentTree)
      {
      // the node is added is it is the node for the source or 
      // it has children.
      //this->Internals->PVAnimationCues[proxyname] = pvCue;
      this->Internals->PVAnimationCues.insert(
        vtkPVAnimationManagerInternals::StringToPVCueMap::value_type(proxyname, pvCue));
      }
    else
      {// Cue has no animatable properties, remove it all together.
      pvParentTree->RemoveChild(pvCue);
      }
    delete [] listname;
    delete [] sourcename;
    delete [] subsourcename;
    }
}

//-----------------------------------------------------------------------------
int vtkPVAnimationManager::AddProperties(vtkPVSource* pvSource, vtkSMProxy* proxy, 
  vtkPVAnimationCueTree* pvCueTree)
{
  int property_added = 0;
  vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    vtkSMProperty* property = iter->GetProperty();
    if (property->GetInformationOnly() || vtkSMProxyProperty::SafeDownCast(property) ||
      property->GetNumberOfDomains() == 0)
      {
      continue;
      }
    
    if (vtkSMStringVectorProperty::SafeDownCast(property))
      {
      property_added += this->AddStringVectorProperty(pvSource, proxy, pvCueTree, 
        vtkSMStringVectorProperty::SafeDownCast(property));
      continue;
      }
    vtkSMVectorProperty* vproperty = vtkSMVectorProperty::SafeDownCast(property);
    if (!vproperty)
      {
      continue;
      }
    int numOfElements = vproperty->GetNumberOfElements();
    if (numOfElements > 1)
      {
      vtkPVAnimationCueTree* cueTree = vtkPVAnimationCueTree::New();
      cueTree->SetLabelText(property->GetXMLName());
      //create tree name.
      ostrstream cueTreeName;
      cueTreeName << pvCueTree->GetName() << "." << property->GetXMLName() << ends;
      cueTree->SetName(cueTreeName.str());
      cueTree->SetPVSource(pvSource);
      cueTreeName.rdbuf()->freeze(0);
      pvCueTree->AddChild(cueTree);
      this->InitializeObservers(cueTree);
      cueTree->Delete();
       
      for (int i=0; i < numOfElements; i++)
        {
        ostrstream str;
        str << i << ends;
        this->SetupCue(pvSource, cueTree, proxy, property->GetXMLName(),
          NULL, i, str.str());
        str.rdbuf()->freeze(0);
        property_added++;
        }
      }
    else
      {
      this->SetupCue(pvSource, pvCueTree, proxy, property->GetXMLName(),NULL,0,
        property->GetXMLName());
      property_added++;
      }
    }
  iter->Delete();
  return (property_added > 0)? 1 : 0;
}

//-----------------------------------------------------------------------------
int vtkPVAnimationManager::AddStringVectorProperty(vtkPVSource* pvSource,
  vtkSMProxy* proxy, vtkPVAnimationCueTree* pvCueTree, 
  vtkSMStringVectorProperty* svp)
{
  vtkSMDomainIterator* diter = svp->NewDomainIterator();
  diter->Begin();

  vtkSMDomain* domain = diter->GetDomain();
  diter->Delete();
  vtkSMArrayListDomain* ald = vtkSMArrayListDomain::SafeDownCast(domain);
  vtkSMStringListDomain* sld = vtkSMStringListDomain::SafeDownCast(domain);
//TODO:  vtkSMArraySelectionDomain* asd = vtkSMArraySelectionDomain::SafeDownCast(domain);

  if (ald) //must check for ald before sld.
    {
    if (svp->GetNumberOfElements() > 1)
      {
      vtkErrorMacro("Case not handled.");
      return 0;
      }
    this->SetupCue(pvSource, pvCueTree, proxy, svp->GetXMLName(),NULL, 
      0, svp->GetXMLName());
    return 1;
    }
  else if (sld)
    {
    if (svp->GetNumberOfElements() > 1)
      {
      vtkErrorMacro("Case not handled.");
      return 0;
      }
    this->SetupCue(pvSource, pvCueTree, proxy, svp->GetXMLName(),NULL, 
      0, svp->GetXMLName());
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SetupCue(vtkPVSource* pvSource, vtkPVAnimationCueTree* parent,
  vtkSMProxy* proxy, const char* propertyname, const char* domainname,
  int element, const char* label)
{
  vtkPVAnimationCue* cue = vtkPVAnimationCue::New();
  
  if (parent->GetName() == NULL)
    {
    vtkErrorMacro("Parent cue has not name. Trace will not work properly");
    }
  else
    {
    ostrstream str;
    str << parent->GetName() << "." << propertyname << "." << element << ends;
    cue->SetName(str.str());
    str.rdbuf()->freeze(0);
    }
    
  cue->SetAnimationScene(this->AnimationScene);
  cue->SetLabelText(label);
  cue->SetPVSource(pvSource);
  parent->AddChild(cue); //class create internally.
  cue->Delete();
  cue->SetAnimatedProxy(proxy);
  cue->SetAnimatedPropertyName(propertyname);
  cue->SetAnimatedElement(element);
  cue->SetAnimatedDomainName(domainname); //how to determine the domian.
  this->InitializeObservers(cue);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ValidateAndAddSpecialCues()
{
  /*
  // this is the place to add camera cue.
  ostrstream str;
  str << "_dont_validate_;" << "camera" << ends;

  ostrstream name;
  name << "_Scene_" << "." << str << ends;
  
  if (this->Internals->PVAnimationCues.find(str.str()) ==
    this->Internals->PVAnimationCues.end())
    {
    // Add the cue.
    // Or, we create a subclass CueTree to get a special CameraCueTree which
    // manages all the manipulation.
    vtkPVAnimationCueTree* pvCueTree = vtkPVAnimationCueTree::New();
    pvCueTree->SetLabelText("Active Camera");
    pvCueTree->SetPVSource(NULL);
    pvCueTree->SetName(name.str());
    this->HAnimationInterface->AddAnimationCueTree(pvCueTree);
    this->Internals->PVAnimationCues[str.str()] = pvCueTree;
    pvCueTree->Delete();
    }
  
  name.rdbuf()->freeze(0);
  str.rdbuf()->freeze(0); 
  */
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::InitializeObservers(vtkPVAnimationCue* cue)
{
  //init observers.
  cue->AddObserver(vtkKWEvent::FocusInEvent, this->Observer);
  cue->AddObserver(vtkKWEvent::FocusOutEvent, this->Observer);
  // TODO: HACK: must fix this.
  this->Script("bind %s <<ResizeEvent>> {%s ResizeCallback}",
    cue->GetWidgetName(), this->HAnimationInterface->GetTclName());
}

//-----------------------------------------------------------------------------
char* vtkPVAnimationManager::GetSourceListName(const char* proxyname)
{
  if (proxyname==NULL || vtkString::Length(proxyname) == 0)
    {
    vtkErrorMacro("Invalid proxy name");
    return NULL;
    }
  char* listname = new char[vtkString::Length(proxyname)+1];
  listname[0] = 0;
  sscanf(proxyname,"%[^.].",listname);
  return listname;
}

//-----------------------------------------------------------------------------
char* vtkPVAnimationManager::GetSourceName(const char* proxyname)
{
  if (proxyname==NULL || vtkString::Length(proxyname) == 0)
    {
    vtkErrorMacro("Invalid proxy name");
    return NULL;
    }
  char* listname = new char[vtkString::Length(proxyname)+1];
  char* sourcename =  new char[vtkString::Length(proxyname)+1];
  listname[0] = 0;
  sourcename[0] = 0;
  sscanf(proxyname,"%[^.].%[^.]",listname,sourcename);
  delete [] listname;
  return sourcename;
}

//-----------------------------------------------------------------------------
char* vtkPVAnimationManager::GetSourceKey(const char* proxyname)
{
  char* listname = this->GetSourceListName(proxyname);
  char* sourcename  = this->GetSourceName(proxyname);
  char* key = vtkString::Append(listname,".",sourcename);
  delete [] listname;
  delete [] sourcename;
  return key;
}

//-----------------------------------------------------------------------------
char* vtkPVAnimationManager::GetSubSourceName(const char* proxyname)
{
  if (proxyname==NULL || vtkString::Length(proxyname) == 0)
    {
    vtkErrorMacro("Invalid proxy name");
    return NULL;
    }
  char* listname = new char[vtkString::Length(proxyname)+1];
  char* sourcename =  new char[vtkString::Length(proxyname)+1];
  char* subsourcename =  new char[vtkString::Length(proxyname)+1];
  listname[0] = 0;
  sourcename[0] = 0;
  subsourcename[0] = 0;
  sscanf(proxyname,"%[^.].%[^.].%s",listname,sourcename,subsourcename);
  delete [] listname;
  delete [] sourcename;
  if (vtkString::Length(subsourcename) > 0)
    {
    return subsourcename;
    }
  delete [] subsourcename;
  return NULL;
}
//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ExecuteEvent(vtkObject* obj, unsigned long event,
  void* )
{
  vtkPVAnimationCue* cue = vtkPVAnimationCue::SafeDownCast(obj);
  if (cue)
    {
    switch (event)
      {
    case vtkKWEvent::FocusOutEvent:
      if (cue == this->VAnimationInterface->GetAnimationCue())
        {
        this->VAnimationInterface->SetAnimationCue(NULL);
        }
      break;
    case vtkKWEvent::FocusInEvent:
      this->VAnimationInterface->SetAnimationCue(cue);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVAnimationManager::NewKeyFrame(int type)
{
  vtkPVKeyFrame* keyframe = NULL;
  switch(type)
    {
  case vtkPVAnimationManager::RAMP:
    keyframe = vtkPVRampKeyFrame::New();
    break;
  case vtkPVAnimationManager::STEP:
    keyframe = vtkPVBooleanKeyFrame::New();
    break;
  case vtkPVAnimationManager::EXPONENTIAL:
    keyframe = vtkPVExponentialKeyFrame::New();
    break;
  case vtkPVAnimationManager::SINUSOID:
    keyframe = vtkPVSinusoidKeyFrame::New();
    break;
  default:
    vtkErrorMacro("Unknown type of keyframe requested: " << type);
    return NULL;
    }
  keyframe->SetParent(this->VAnimationInterface->GetPropertiesFrame());
  return keyframe;
}

//-----------------------------------------------------------------------------
int vtkPVAnimationManager::GetKeyFrameType(vtkPVKeyFrame* kf)
{
  if (vtkPVRampKeyFrame::SafeDownCast(kf))
    {
    return vtkPVAnimationManager::RAMP;
    }
  else if (vtkPVBooleanKeyFrame::SafeDownCast(kf))
    {
    return vtkPVAnimationManager::STEP;
    }
  else if (vtkPVExponentialKeyFrame::SafeDownCast(kf))
    {
    return vtkPVAnimationManager::EXPONENTIAL;
    }
  else if (vtkPVSinusoidKeyFrame::SafeDownCast(kf))
    {
    return vtkPVAnimationManager::SINUSOID;
    }
  return vtkPVAnimationManager::LAST_NOT_USED;
}

//-----------------------------------------------------------------------------
vtkPVKeyFrame* vtkPVAnimationManager::ReplaceKeyFrame(vtkPVAnimationCue* pvCue, 
  int type, vtkPVKeyFrame* replaceFrame)
{
  if (this->GetKeyFrameType(replaceFrame) == type)
    {
    // no replace necessary.
    return replaceFrame;
    }
  vtkPVKeyFrame* keyFrame = this->NewKeyFrame(type);
  if (!keyFrame)
    {
    return NULL;
    }
  keyFrame->SetAnimationCue(pvCue);
  keyFrame->Create(this->GetApplication(), 0);
  pvCue->ReplaceKeyFrame(replaceFrame, keyFrame);
  keyFrame->Delete();
  return keyFrame;
}


//-----------------------------------------------------------------------------
void vtkPVAnimationManager::StartRecording()
{
  if (this->InRecording)
    {
    return;
    }
  this->InRecording = 1;
  this->RecordingIncrement = 0.1;
  this->HAnimationInterface->StartRecording();

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow *pvWindow = (pvApp)? pvApp->GetMainWindow() : NULL;
  if (pvWindow)
    {
    pvWindow->UpdateEnableState();
    }

}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::StopRecording()
{
  if (!this->InRecording)
    {
    return;
    }
  this->InRecording = 0;
  this->HAnimationInterface->StopRecording();

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow *pvWindow = (pvApp)? pvApp->GetMainWindow() : NULL;
  if (pvWindow)
    {
    pvWindow->UpdateEnableState();
    }
}


//-----------------------------------------------------------------------------
void vtkPVAnimationManager::RecordState()
{
  if (!this->InRecording)
    {
    vtkErrorMacro("Not in recording.");
    return;
    }
  // determine the paramter to insert the key frame.
  double curbounds[2] = {0, 0};
  this->HAnimationInterface->GetTimeBounds(curbounds);
  double parameter = curbounds[1];
  if (parameter + this->RecordingIncrement > 1)
    {
    curbounds[1] -= this->RecordingIncrement;
    this->HAnimationInterface->SetTimeBounds(curbounds,1);
    parameter -= this->RecordingIncrement;
    this->RecordingIncrement *= ( 1.0 - this->RecordingIncrement); 
    }
  this->Script("update");
  this->HAnimationInterface->RecordState(parameter, this->RecordingIncrement,
    !this->RecordAll);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveState(ofstream* file)
{
  // Save the Manager state.
  *file << "set kw(" << this->AnimationScene->GetTclName() << ") [$kw(" 
    << this->GetTclName() << ") GetAnimationScene]" << endl;
  *file << "set kw(" << this->VAnimationInterface->GetTclName() << ") [$kw("
    << this->GetTclName() << ") GetVAnimationInterface]" << endl;
  *file << "set kw(" << this->HAnimationInterface->GetTclName() << ") [$kw("
    << this->GetTclName() << ") GetHAnimationInterface]" << endl;
  
  // Save the VAnimationInterface * HAnimationInterface State
  this->HAnimationInterface->SaveState(file);
  this->VAnimationInterface->SaveState(file);
  // Save the Animation Scene State.
  this->AnimationScene->SaveState(file);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveInBatchScript(ofstream* file)
{
  if (this->AnimationScene)
    {
    this->AnimationScene->SaveInBatchScript(file);
    }
}

//-----------------------------------------------------------------------------
int vtkPVAnimationManager::GetInPlay()
{
  if (this->AnimationScene && this->AnimationScene->IsCreated())
    {
    return this->AnimationScene->IsInPlay();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SetCurrentTime(double ntime)
{
  if (this->AnimationScene)
    {
    this->AnimationScene->SetNormalizedCurrentTime(ntime);
    }
  this->AddTraceEntry("$kw(%s) SetCurrentTime %f",
    this->GetTclName(), ntime);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  int inPlay = 0;
  if (this->AnimationScene && this->AnimationScene->IsInPlay())
    {
    inPlay = 1;
    }
  this->PropagateEnableState(this->VAnimationInterface);
  this->PropagateEnableState(this->AnimationScene);
  
  int enabled = this->Enabled;
  if (inPlay)
    {
    this->Enabled = 0;
    }
  this->PropagateEnableState(this->HAnimationInterface);
  this->Enabled = enabled;
}

//-----------------------------------------------------------------------------
int vtkPVAnimationManager::GetUseGeometryCache()
{
  if (this->OverrideCache)
    {
    return 0;
    }
  return (this->VAnimationInterface->GetCacheGeometry() && 
    this->AnimationScene->GetPlayMode() == VTK_ANIMATION_SCENE_PLAYMODE_SEQUENCE);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveAnimation()
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVRenderView* view = pvWin->GetMainView();

  vtkKWLoadSaveDialog* saveDialog = vtkKWLoadSaveDialog::New();
  this->GetWindow()->RetrieveLastPath(saveDialog, "SaveAnimationFile2");
  saveDialog->SetParent(this);
  saveDialog->SaveDialogOn();
  saveDialog->Create(this->GetApplication(), 0);
  saveDialog->SetTitle("Save Animation Images");
  ostrstream ostr;
  ostr << "{{JPEG Images} {.jpg}} {{TIFF Images} {.tif}} {{PNG Images} {.png}}";
  ostr << " {{MPEG2 movie file} {.mp2}}";

#ifdef _WIN32
  ostr << " {{AVI movie file} {.avi}}";
#endif

  ostr << ends;

  saveDialog->SetFileTypes(ostr.str());
  ostr.rdbuf()->freeze(0);

  if ( saveDialog->Invoke() &&
    strlen(saveDialog->GetFileName())>0 )
    {
    this->GetWindow()->SaveLastPath(saveDialog, "SaveAnimationFile2");
    const char* filename = saveDialog->GetFileName();  

    // Split into root and extension.
    char* fileRoot;
    char* ptr;
    char* ext = NULL;
    fileRoot = new char[strlen(filename)+1];
    strcpy(fileRoot, filename);
    // Find extension (last .)
    ptr = fileRoot;
    while (*ptr != '\0')
      {
      if (*ptr == '.')
        {
        ext = ptr;
        }
      ++ptr;
      }
    if (ext == NULL)
      {
      vtkErrorMacro(<< "Could not find extension in " << filename);
      delete [] fileRoot;
      fileRoot = NULL;
      saveDialog->Delete();
      saveDialog = NULL;
      return;
      }
    // Separate the root from the extension.
    *ext = '\0';
    ++ext;

    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
    dlg->SetMasterWindow(pvWin);
    dlg->Create(this->GetApplication(), "");
    // is this a video format
    int isMPEG = (!strcmp(ext, "mpg") || !strcmp(ext, "mpeg") ||
      !strcmp(ext, "MPG") || !strcmp(ext, "MPEG") ||
      !strcmp(ext, "AVI") || !strcmp(ext, "avi") ||
      !strcmp(ext, "MP2") || !strcmp(ext, "mp2"));
    if (isMPEG)
      {
      dlg->SetText(
        "Specify the width and height of the mpeg to be saved from this "
        "animation. The width must be a multiple of 32 and the height a "
        "multiple of 8. Each will be resized to the next smallest multiple "
        "if it does not meet this criterion. The maximum size allowed is "
        "1920 by 1080");
      }
    else
      { 
      dlg->SetText(
        "Specify the width and height of the images to be saved from this "
        "animation. Each dimension must be a multiple of 4. Each will be "
        "resized to the next smallest multiple of 4 if it does not meet this "
        "criterion.");
      }
    vtkKWWidget *frame = vtkKWWidget::New();
    frame->SetParent(dlg->GetTopFrame());
    frame->Create(this->GetApplication(), "frame", "");

    int origWidth = view->GetRenderWindowSize()[0];
    int origHeight = view->GetRenderWindowSize()[1];

    vtkKWLabeledEntry *widthEntry = vtkKWLabeledEntry::New();
    widthEntry->SetLabel("Width:");
    widthEntry->SetParent(frame);
    widthEntry->Create(this->GetApplication(), "");
    widthEntry->GetEntry()->SetValue(origWidth);

    vtkKWLabeledEntry *heightEntry = vtkKWLabeledEntry::New();
    heightEntry->SetLabel("Height:");
    heightEntry->SetParent(frame);
    heightEntry->Create(this->GetApplication(), "");
    heightEntry->GetEntry()->SetValue(origHeight);

    this->Script("pack %s %s -side left -fill both -expand t",
      widthEntry->GetWidgetName(), heightEntry->GetWidgetName());
    this->Script("pack %s -side top -pady 5", frame->GetWidgetName());

    dlg->Invoke();

    int width = widthEntry->GetEntry()->GetValueAsInt();
    int height = origHeight;
    height = heightEntry->GetEntry()->GetValueAsInt();

    // For now, the image size for the animations cannot be larger than
    // the size of the render window. The problem is that tiling doesn't
    // work with multiple processes.
    if (0 && (width > origWidth || height > origHeight))
      {
      int diffX = width - origWidth;
      int diffY = height - origHeight;
      double aspect = width / (double)height;
      if (diffX > diffY)
        {
        width = origWidth;
        height = (int)(width / aspect);
        }
      else
        {
        height = origHeight;
        width = (int)(height * aspect);
        }
      }

    if (isMPEG)
      {
      if ((width % 32) > 0)
        {
        width -= width % 32;
        }
      if ((height % 8) > 0)
        {
        height -= height % 8;
        }
      if (width > 1920)
        {
        width = 1920;
        }
      if (height > 1080)
        {
        height = 1080;
        }      
      }
    else
      {
      if ((width % 4) > 0)
        {
        width -= width % 4;
        }
      if ((height % 4) > 0)
        {
        height -= height % 4;
        }
      }

    widthEntry->Delete();
    heightEntry->Delete();
    frame->Delete();
    dlg->Delete();

    this->AnimationScene->SaveImages(fileRoot, ext, width, height, 0);
    view->SetRenderWindowSize(origWidth, origHeight);

    delete [] fileRoot;
    fileRoot = NULL;
    ext = NULL;
    }

  saveDialog->Delete();
  saveDialog = NULL;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveGeometry()
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  
  vtkKWLoadSaveDialog* saveDialog = pvApp->NewLoadSaveDialog();
  pvWin->RetrieveLastPath(saveDialog, "SaveGeometryFile");
  saveDialog->SetParent(this);
  saveDialog->SaveDialogOn();
  saveDialog->Create(this->GetApplication(), 0);
  saveDialog->SetTitle("Save Animation Geometry");
  saveDialog->SetFileTypes("{{ParaView Data Files} {.pvd}}");
  if(saveDialog->Invoke() && (strlen(saveDialog->GetFileName()) > 0))
    {
    pvWin->SaveLastPath(saveDialog, "SaveGeometryFile");
//    this->SaveGeometry(saveDialog->GetFileName(), numPartitions);
    this->AnimationScene->SaveGeometry(saveDialog->GetFileName());
    }

  saveDialog->Delete();
  saveDialog = NULL;

}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::AddDefaultAnimation(vtkPVSource* pvSource)
{
  vtkPVReaderModule* clone = vtkPVReaderModule::SafeDownCast(pvSource);
  if (clone)
    {
    int numOfTimeSteps = clone->GetNumberOfTimeSteps();
    if (numOfTimeSteps <= 1)
      {
      return;
      }
    vtkPVWidget* pvTimeStepWidget = clone->GetTimeStepWidget();
    const char* animatedPropertyName = pvTimeStepWidget->GetSMProperty()->GetXMLName();
    if (!animatedPropertyName || !animatedPropertyName[0])
      {
      return;
      }

    // add keyframes to the timestep cue.
    // First get the CueTree for this source.
    vtkPVAnimationCueTree* cueTree = this->GetAnimationCueTreeForSource(pvSource);
    if (!cueTree)
      {
      vtkErrorMacro("Animation Cue for could not be found for the source.");
      return;
      }
    // Second, get the FileName cue.
    vtkCollectionIterator* iter = cueTree->NewChildrenIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkPVAnimationCue* child = vtkPVAnimationCue::SafeDownCast(
        iter->GetCurrentObject());
      const char* propertyname = child->GetAnimatedPropertyName();
      if (propertyname && strcmp(propertyname,animatedPropertyName)==0)
        {
        child->AddNewKeyFrame(0.0);
        child->AddNewKeyFrame(1.0);
        this->AnimationScene->ShowAnimationToolbar();
        break;
        }
      }
    iter->Delete();
    }
}

//-----------------------------------------------------------------------------
vtkPVAnimationCueTree* vtkPVAnimationManager::GetAnimationCueTreeForSource(
  vtkPVSource* pvSource)
{
  vtkPVAnimationManagerInternals::StringToPVCueMap::iterator iter;
  for ( iter = this->Internals->PVAnimationCues.begin();
    iter != this->Internals->PVAnimationCues.end(); iter++)
    {
    if (iter->second->GetPVSource() == pvSource)
      {
      return vtkPVAnimationCueTree::SafeDownCast(iter->second);
      }
    }
  return NULL;
}

//-----------------------------------------------------------------------------
vtkPVAnimationCueTree* vtkPVAnimationManager::GetAnimationCueTreeForProxy(
  const char* proxyname)
{
  char* sourcekey = this->GetSourceKey(proxyname);
  if (!sourcekey)
    {
    vtkErrorMacro("Cannot find source for proxy " << proxyname);
    return NULL;
    }
  vtkPVAnimationManagerInternals::StringToPVCueMap::iterator iter;
  iter = this->Internals->PVAnimationCues.find(sourcekey);
  if (iter == this->Internals->PVAnimationCues.end())
    {
    vtkErrorMacro("Cannot find source for proxy " << proxyname);
    return NULL;
    }
  vtkPVAnimationCueTree* tree =  vtkPVAnimationCueTree::SafeDownCast(iter->second);
  return vtkPVAnimationCueTree::SafeDownCast(tree->GetChild(proxyname));
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::RemoveAllKeyFrames()
{
  if (this->IsCreated())
    {
    this->HAnimationInterface->RemoveAllKeyFrames();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveWindowGeometry()
{
  if (this->IsCreated())
    {
    this->HAnimationInterface->SaveWindowGeometry();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::RestoreWindowGeometry()
{
  this->HAnimationInterface->RestoreWindowGeometry();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::InvalidateAllGeometries()
{
  this->AnimationScene->InvalidateAllGeometries();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void vtkPVAnimationManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RecordAll: " << this->RecordAll << endl;
  os << indent << "VAnimationInterface: " << this->VAnimationInterface << endl;
  os << indent << "HAnimationInterface: " << this->HAnimationInterface << endl;
  os << indent << "AnimationScene: " << this->AnimationScene << endl;
  os << indent << "ProxyIterator: " << this->ProxyIterator << endl;
  os << indent << "AdvancedView: " << this->AdvancedView << endl;
  os << indent << "OverrideCache: " << this->OverrideCache << endl;
  os << indent << "InRecording: " << this->InRecording << endl;
}
