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
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkKWEntry.h"
#include "vtkKWEntryWithLabel.h"
#include "vtkKWRadioButtonSetWithLabel.h"
#include "vtkKWRadioButtonSet.h"
#include "vtkKWRadioButton.h"
#include "vtkKWEvent.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLoadSaveDialog.h"
#include "vtkKWMessageDialog.h"
#include "vtkProcessModule.h"
#include "vtkPVActiveTrackSelector.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkPVAnimationScene.h"
#include "vtkPVApplication.h"
#include "vtkPVCameraAnimationCue.h"
#include "vtkPVHorizontalAnimationInterface.h"
#include "vtkPVReaderModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVTraceHelper.h"
#include "vtkPVVerticalAnimationInterface.h"
#include "vtkPVWidget.h"
#include "vtkPVWindow.h"
#include "vtkSmartPointer.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMVectorProperty.h"
#include "vtkSMXDMFPropertyDomain.h"
#include "vtkToolkits.h"

#include <vtkstd/map>
#include <vtkstd/string>

#include <vtksys/SystemTools.hxx>

#define VTK_PV_ANIMATION_GROUP "animateable"
#define VTK_PV_CAMERA_PROXYNAME "_dont_validate_.ActiveCamera"

vtkStandardNewMacro(vtkPVAnimationManager);
vtkCxxRevisionMacro(vtkPVAnimationManager, "1.62.2.4");
vtkCxxSetObjectMacro(vtkPVAnimationManager, HorizontalParent, vtkKWWidget);
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

class vtkPVAMSourceDeletedCommand : public vtkCommand
{
public:
  void Execute(vtkObject*, unsigned long, void*)
  {
    if (this->AnimationManager)
      {
      this->AnimationManager->Update();
      }
  }

  vtkPVAMSourceDeletedCommand() : AnimationManager(0) {}

  vtkPVAnimationManager* AnimationManager;
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
  this->HorizontalParent = NULL;
  this->VerticalParent = NULL;
  this->VAnimationInterface = vtkPVVerticalAnimationInterface::New();
  this->VAnimationInterface->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->VAnimationInterface->GetTraceHelper()->SetReferenceCommand(
    "GetVAnimationInterface");
  
  this->HAnimationInterface = vtkPVHorizontalAnimationInterface::New();
  this->HAnimationInterface->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->HAnimationInterface->GetTraceHelper()->SetReferenceCommand(
    "GetHAnimationInterface");
  
  
  this->AnimationScene = vtkPVAnimationScene::New();
  this->AnimationScene->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->AnimationScene->GetTraceHelper()->SetReferenceCommand(
    "GetAnimationScene");
  

  this->ActiveTrackSelector = vtkPVActiveTrackSelector::New();
  this->ActiveTrackSelector->GetTraceHelper()->SetReferenceHelper(
    this->GetTraceHelper());
  this->ActiveTrackSelector->GetTraceHelper()->SetReferenceCommand(
    "GetActiveTrackSelector");

  this->ProxyIterator = vtkSMProxyIterator::New();
  this->Internals = new vtkPVAnimationManagerInternals;
  this->Observer = vtkPVAnimationManagerObserver::New();
  this->Observer->SetTarget(this);
  this->RecordAll = 1;
  this->InRecording = 0;
  this->RecordingIncrement = 0.1;

  this->AdvancedView = 0;

  this->ObserverTag = 0;
}

//-----------------------------------------------------------------------------
vtkPVAnimationManager::~vtkPVAnimationManager()
{
  this->SetVerticalParent(0);
  this->SetHorizontalParent(0);
  this->AnimationScene->Delete();
  this->ActiveTrackSelector->Delete();
  this->VAnimationInterface->Delete();
  this->HAnimationInterface->Delete();
  this->ProxyIterator->Delete();
  delete this->Internals;
  this->Observer->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::Create(vtkKWApplication* app)
{
  if (!this->VerticalParent || !this->HorizontalParent)
    {
    vtkErrorMacro("VerticalParent and HorizontalParent must be set before calling create.");
    return;
    }

  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::Create(app);

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();

  vtkPVAMSourceDeletedCommand* comm = new vtkPVAMSourceDeletedCommand;
  comm->AnimationManager = this;
  this->ObserverTag = pvWin->AddObserver(vtkKWEvent::SourceDeletedEvent, comm);
  comm->Delete();

  if (pvApp->HasRegistryValue(2,"RunTime","AdvancedAnimationView"))
    {
    this->AdvancedView = pvApp->GetIntRegistryValue(2, "RunTime", "AdvancedAnimationView");
    }

  this->HAnimationInterface->SetParent(this->HorizontalParent);
  this->HAnimationInterface->Create(app);
  this->HAnimationInterface->SetReliefToFlat();

  this->VAnimationInterface->SetParent(this->VerticalParent);
  this->VAnimationInterface->SetAnimationManager(this);
  this->VAnimationInterface->Create(app);
  this->VAnimationInterface->SetReliefToFlat();

  this->AnimationScene->SetParent(
    this->VAnimationInterface->GetScenePropertiesFrame());
  this->AnimationScene->SetAnimationManager(this);
  this->AnimationScene->SetWindow(pvWin);
  this->AnimationScene->SetRenderView(pvWin->GetMainView());
  this->AnimationScene->Create(app);
  this->AnimationScene->SetReliefToFlat();
  // This is set so that when the duration (or anything else)
  // from the scene changes, the VAnimation interface is updated
  // and so is the keyframe it is showing, if any. Thus, the keyframe
  // will show the correct time when the duration is changed.
  this->AnimationScene->SetPropertiesChangedCallback(
    this->VAnimationInterface, "Update");

  this->Script("pack %s -anchor n -side top -expand t -fill both",
    this->AnimationScene->GetWidgetName());

  this->ActiveTrackSelector->SetParent(
    this->VAnimationInterface->GetSelectorFrame());
  this->ActiveTrackSelector->Create(app);
  this->ActiveTrackSelector->SetReliefToFlat();
  this->Script("pack %s -anchor n -side top -expand t -fill both",
    this->ActiveTrackSelector->GetWidgetName());
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
  this->GetApplication()->SetRegistryValue(2, "RunTime",
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
//  this->ShowHAnimationInterface();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ShowVAnimationInterface()
{
  if (!this->VAnimationInterface->IsPacked())
    {
    this->VAnimationInterface->UnpackSiblings();
    this->Script("pack %s -anchor n -side top -expand t -fill both",
      this->VAnimationInterface->GetWidgetName());
    this->VAnimationInterface->Update();
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
        this->ActiveTrackSelector->RemoveSource(pvCueTree);
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
        pvParent->RemoveChildCue(iter->second);
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
        pvParentTree->AddChildCue(pvCue);
        }
      }
    else
      {
      pvCue->SetName(proxyname);
      pvCue->SetPVSource(pvSource);
      this->HAnimationInterface->AddAnimationCueTree(pvCue);
      this->ActiveTrackSelector->AddSource(pvCue);
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
      pvParentTree->RemoveChildCue(pvCue);
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
      property->GetNumberOfDomains() == 0 || !property->GetAnimateable())
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
    int has_repeat_command = vproperty->GetRepeatCommand();
    // Generally, for every element in the propety, we create a separate track,
    // except for those properties that have the repeat command set. (We can later
    // change this to be a special attribute, but for now, repeat commmand suffices).
    // For those that have repeat command set, a single track is created, which 
    // manages the property (AnimatedElement for that cue is set to -1).
    if (numOfElements == 1 || has_repeat_command)
      {
      int element_index = (has_repeat_command)? -1: 0;
      this->SetupCue(pvSource, pvCueTree, proxy, property->GetXMLName(),NULL,
        element_index,  property->GetXMLName());
      property_added++;
      }
    else
      {
      vtkPVAnimationCueTree* cueTree = vtkPVAnimationCueTree::New();
      cueTree->SetLabelText(property->GetXMLName());
      //create tree name.
      ostrstream cueTreeName;
      cueTreeName /*<< pvCueTree->GetName() << "."*/ << property->GetXMLName() << ends;
      cueTree->SetName(cueTreeName.str());
      cueTree->SetPVSource(pvSource);
      cueTreeName.rdbuf()->freeze(0);
      pvCueTree->AddChildCue(cueTree);
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
  vtkSMXDMFPropertyDomain* xdmf = vtkSMXDMFPropertyDomain::SafeDownCast(domain);

//TODO:  vtkSMArraySelectionDomain* asd = vtkSMArraySelectionDomain::SafeDownCast(domain);

  if (xdmf)
    {
    int nos = svp->GetNumberOfElements();
    int nos_per_command = svp->GetNumberOfElementsPerCommand();
    if (nos_per_command != 2)
      {
      vtkErrorMacro("Can only handle specific XDMF case");
      return 0;
      }
    int no_tracks = nos / 2;
    for (int i=0; i < no_tracks; i++)
      {
      const char* name = svp->GetElement(2 * i);
      this->SetupCue(pvSource, pvCueTree, proxy, svp->GetXMLName(),
        0, i, name);
      }
    return (no_tracks > 0)? 1 : 0;
    }
  else if (ald) //must check for ald before sld.
    {
    if (svp->GetNumberOfElements() > 1)
      {
      vtkDebugMacro("Case not handled.");
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
      vtkDebugMacro("Case not handled.");
      return 0;
      }
    this->SetupCue(pvSource, pvCueTree, proxy, svp->GetXMLName(),NULL, 
      0, svp->GetXMLName());
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkPVAnimationCue* vtkPVAnimationManager::SetupCue(vtkPVSource* pvSource, 
  vtkPVAnimationCueTree* parent,
  vtkSMProxy* proxy, const char* propertyname, const char* domainname,
  int element, const char* label, vtkPVAnimationCue* cueToSetup /*= NULL*/)
{
  vtkPVAnimationCue* cue = (cueToSetup)? cueToSetup : vtkPVAnimationCue::New();
  
  if (parent->GetName() == NULL)
    {
    vtkErrorMacro("Parent cue has not name. Trace will not work properly");
    }
  else
    {
    ostrstream str;
    str /*<< parent->GetName() << "." */<< propertyname << "." << element << ends;
    cue->SetName(str.str());
    str.rdbuf()->freeze(0);
    }
  cue->SetKeyFrameParent(this->VAnimationInterface->GetPropertiesFrame());
  cue->SetAnimationScene(this->AnimationScene);
  cue->SetLabelText(label);
  cue->SetPVSource(pvSource);
  parent->AddChildCue(cue); //class create internally.
  cue->SetAnimatedProxy(proxy);
  cue->SetAnimatedPropertyName(propertyname);
  cue->SetAnimatedElement(element);
  cue->SetAnimatedDomainName(domainname); //how to determine the domian.
  this->InitializeObservers(cue);
  if (!cueToSetup)
    {
    cue->Delete();  
    }
  return cue;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::ValidateAndAddSpecialCues()
{
  // this is the place to add camera cue.
  const char* camera_cuetree_name =  VTK_PV_CAMERA_PROXYNAME;

  if (this->Internals->PVAnimationCues.find(camera_cuetree_name) ==
    this->Internals->PVAnimationCues.end())
    {
    // Add the tree for the Camera.
    vtkPVAnimationCueTree* pvCueTree = vtkPVAnimationCueTree::New();
    pvCueTree->SetLabelText("Active Camera");
    pvCueTree->SetPVSource(NULL);
    pvCueTree->SetName(camera_cuetree_name);
    pvCueTree->SetSourceTreeName(camera_cuetree_name);
    this->HAnimationInterface->AddAnimationCueTree(pvCueTree);
    char* key = this->GetSourceKey(camera_cuetree_name);
    this->Internals->PVAnimationCues[key] = pvCueTree;
    delete[] key;
    pvCueTree->Delete();
    this->InitializeObservers(pvCueTree);
    this->ActiveTrackSelector->AddSource(pvCueTree);

    // Now create the Cue that maintains camera keyframes.
    vtkSMProxy* cameraProxy = vtkPVApplication::SafeDownCast(
      this->GetApplication())->GetRenderModuleProxy();
    vtkPVAnimationCue* cue = vtkPVCameraAnimationCue::New();
    this->SetupCue( NULL, pvCueTree, cameraProxy, "Camera", 0, -1, 
      "Camera", cue);
    cue->Delete();
    cue->SetDefaultKeyFrameType(vtkPVSimpleAnimationCue::CAMERA);
    cue->SetSourceTreeName(pvCueTree->GetName());
    }
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
  if (proxyname==NULL || strlen(proxyname) == 0)
    {
    vtkErrorMacro("Invalid proxy name");
    return NULL;
    }
  char* listname = new char[strlen(proxyname)+1];
  listname[0] = 0;
  sscanf(proxyname,"%[^.].",listname);
  return listname;
}

//-----------------------------------------------------------------------------
char* vtkPVAnimationManager::GetSourceName(const char* proxyname)
{
  if (proxyname==NULL || strlen(proxyname) == 0)
    {
    vtkErrorMacro("Invalid proxy name");
    return NULL;
    }
  char* listname = new char[strlen(proxyname)+1];
  char* sourcename =  new char[strlen(proxyname)+1];
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
  char* key = vtksys::SystemTools::AppendStrings(listname, ".", sourcename);
  delete [] listname;
  delete [] sourcename;
  return key;
}

//-----------------------------------------------------------------------------
char* vtkPVAnimationManager::GetSubSourceName(const char* proxyname)
{
  if (proxyname==NULL || strlen(proxyname) == 0)
    {
    vtkErrorMacro("Invalid proxy name");
    return NULL;
    }
  char* listname = new char[strlen(proxyname)+1];
  char* sourcename =  new char[strlen(proxyname)+1];
  char* subsourcename =  new char[strlen(proxyname)+1];
  listname[0] = 0;
  sourcename[0] = 0;
  subsourcename[0] = 0;
  sscanf(proxyname,"%[^.].%[^.].%s",listname,sourcename,subsourcename);
  delete [] listname;
  delete [] sourcename;
  if (strlen(subsourcename) > 0)
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
//      if (cue == this->VAnimationInterface->GetAnimationCue())
        {
        this->VAnimationInterface->SetAnimationCue(NULL);
        this->ActiveTrackSelector->SelectCue(NULL);
        }
      break;
    case vtkKWEvent::FocusInEvent:
      this->VAnimationInterface->SetAnimationCue(cue);
      this->ActiveTrackSelector->SelectCue(cue);
      break;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::StartRecording()
{
  if (this->InRecording)
    {
    return;
    }
  this->InRecording = 1;
//  this->RecordingIncrement = 0.1;
  this->RecordingIncrement = 1.0;
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
    double scale_factor = 1.0 / (parameter + this->RecordingIncrement);
    curbounds[1] *= scale_factor;
    curbounds[0] *= scale_factor;
    parameter = curbounds[1];
    this->HAnimationInterface->SetTimeBounds(curbounds, 1);
    this->RecordingIncrement *= scale_factor;
    if (this->RecordingIncrement == 0)
      {
      vtkErrorMacro("Recording error!");
      return;
      }
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
void vtkPVAnimationManager::SetAnimationTime(double ntime)
{
  if (this->AnimationScene)
    {
    this->AnimationScene->SetNormalizedAnimationTime(ntime);
    }
  this->GetTraceHelper()->AddEntry("$kw(%s) SetAnimationTime %f",
    this->GetTclName(), ntime);
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->VAnimationInterface);
  this->PropagateEnableState(this->AnimationScene);
  
  int inPlay = 0;
  if (this->AnimationScene && this->AnimationScene->IsInPlay())
    {
    inPlay = 1;
    }

  if (this->HAnimationInterface)
    {
    this->HAnimationInterface->SetEnabled(inPlay ? 0 : this->GetEnabled());
    }

  if (this->ActiveTrackSelector)
    {
    this->ActiveTrackSelector->SetEnabled(inPlay? 0 : this->GetEnabled());
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::PrepareForDelete()
{
  this->AnimationScene->PrepareForDelete();

  if (this->ObserverTag > 0)
    {
    vtkPVApplication* pvApp = 
      vtkPVApplication::SafeDownCast(this->GetApplication());
    vtkPVWindow* pvWin = pvApp->GetMainWindow();
    pvWin->RemoveObserver(this->ObserverTag);
    this->ObserverTag = 0;
    }

  // We need to delete the cue for the camera here since it keeps a
  // reference to the vtkSMRenderModuleProxy which must be deleted before
  // the GUI is destroyed.
  char* sourcekey = this->GetSourceKey(VTK_PV_CAMERA_PROXYNAME);
  vtkPVAnimationManagerInternals::StringToPVCueMap::iterator iter = 
    this->Internals->PVAnimationCues.find(sourcekey);
  if (iter != this->Internals->PVAnimationCues.end())
    {
    vtkPVAnimationCueTree* cameraTree = vtkPVAnimationCueTree::SafeDownCast(
      iter->second);
    this->HAnimationInterface->RemoveAnimationCueTree(cameraTree);
    this->ActiveTrackSelector->RemoveSource(cameraTree);
    this->Internals->PVAnimationCues.erase(this->Internals->PVAnimationCues.find(sourcekey));
    }
  delete[] sourcekey;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SetCacheGeometry(int cache)
{
  this->AnimationScene->SetCaching(cache);
}

//-----------------------------------------------------------------------------
int vtkPVAnimationManager::GetCacheGeometry()
{
  return this->AnimationScene->GetCaching();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::EnableCacheCheck()
{
  this->VAnimationInterface->EnableCacheCheck();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::DisableCacheCheck()
{
  this->VAnimationInterface->DisableCacheCheck();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveAnimation()
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkPVWindow* pvWin = pvApp->GetMainWindow();
  vtkPVRenderView* view = pvWin->GetMainView();

  vtkKWLoadSaveDialog* saveDialog = vtkKWLoadSaveDialog::New();
  this->GetApplication()->RetrieveDialogLastPathRegistryValue(saveDialog, "SaveAnimationFile2");
  saveDialog->SetParent(this);
  saveDialog->SaveDialogOn();
  saveDialog->Create(this->GetApplication());
  saveDialog->SetTitle("Save Animation Images");
  ostrstream ostr;
  ostr << "{{JPEG Images} {.jpg}} {{TIFF Images} {.tif}} {{PNG Images} {.png}}";
  ostr << " {{MPEG2 movie file} {.mpg}}";

#ifdef _WIN32
  ostr << " {{AVI movie file} {.avi}}";
#else
#ifdef VTK_USE_FFMPEG_ENCODER
  ostr << " {{AVI movie file} {.avi}}";
#endif
#endif

  ostr << ends;

  saveDialog->SetFileTypes(ostr.str());
  ostr.rdbuf()->freeze(0);

  if ( saveDialog->Invoke() &&
    strlen(saveDialog->GetFileName())>0 )
    {
    this->GetApplication()->SaveDialogLastPathRegistryValue(saveDialog, "SaveAnimationFile2");
    const char* filename = saveDialog->GetFileName();  
    vtksys_stl::string filename_stl = filename;
    vtksys_stl::string ext_stl = vtksys::SystemTools::GetFilenameLastExtension(filename);
    vtksys_stl::string::size_type dot_pos = filename_stl.rfind(".");
    vtksys_stl::string fileRoot;
    if (dot_pos != vtksys_stl::string::npos)
      {
      fileRoot = filename_stl.substr(0, dot_pos);
      }
    else
      {
      fileRoot = filename_stl;
      }
    if (ext_stl.size() >= 1)
      {
      ext_stl.erase(0,1); // Get rid of the "." GetFilenameExtension returns.
      }
    const char* ext = ext_stl.c_str();

    vtkKWMessageDialog *dlg = vtkKWMessageDialog::New();
    dlg->SetTitle("Image Size");
    dlg->SetMasterWindow(pvWin);
    dlg->Create(this->GetApplication());
    // is this a video format
    int isMPEG = 
      (!strcmp(ext, "mpg") || !strcmp(ext, "mpeg") ||
       !strcmp(ext, "MPG") || !strcmp(ext, "MPEG") ||
       !strcmp(ext, "MP2") || !strcmp(ext, "mp2"));
    if (isMPEG)
      {
      dlg->SetText(
        "Specify the width, height and frame rate of the mpeg to be saved from this "
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
    vtkKWFrame *frame = vtkKWFrame::New();
    frame->SetParent(dlg->GetTopFrame());
    frame->Create(this->GetApplication());

    int origWidth = view->GetRenderWindowSize()[0];
    int origHeight = view->GetRenderWindowSize()[1];

    vtkKWEntryWithLabel *widthEntry = vtkKWEntryWithLabel::New();
    widthEntry->GetLabel()->SetText("Width:");
    widthEntry->SetParent(frame);
    widthEntry->Create(this->GetApplication());
    widthEntry->GetWidget()->SetValueAsInt(origWidth);

    vtkKWEntryWithLabel *heightEntry = vtkKWEntryWithLabel::New();
    heightEntry->GetLabel()->SetText("Height:");
    heightEntry->SetParent(frame);
    heightEntry->Create(this->GetApplication());
    heightEntry->GetWidget()->SetValueAsInt(origHeight);

    vtkKWEntryWithLabel *framerateEntry = vtkKWEntryWithLabel::New();
    framerateEntry->GetLabel()->SetText("Frame Rate:");
    framerateEntry->SetParent(frame);
    framerateEntry->Create(this->GetApplication());
    framerateEntry->GetWidget()->SetValueAsInt(1);

    int isAVI = (!strcmp(ext, "AVI") || !strcmp(ext, "avi"));

    vtkKWRadioButtonSetWithLabel *qualityEntryLabel = vtkKWRadioButtonSetWithLabel::New();
    qualityEntryLabel->SetParent(frame);
    qualityEntryLabel->SetLabelText("Compressed Quality:");
    qualityEntryLabel->Create(this->GetApplication());
    vtkKWRadioButtonSet *qualityEntry = qualityEntryLabel->GetWidget();
    vtkKWRadioButton *lowButton = qualityEntry->AddWidget(0);
    lowButton->SetText("Low");
    lowButton->SetSelectedState(0);
    vtkKWRadioButton *mediumButton = qualityEntry->AddWidget(1);
    mediumButton->SetText("Medium");
    mediumButton->SetSelectedState(0);
    vtkKWRadioButton *highButton = qualityEntry->AddWidget(2);
    highButton->SetText("High");
    highButton->SetSelectedState(1);

    this->Script("pack %s %s -side left -fill both -expand t",
      widthEntry->GetWidgetName(), heightEntry->GetWidgetName());
    if (isMPEG)
      {
      this->Script("pack %s -side left -fill both -expand t",
        framerateEntry->GetWidgetName());
      }
    if (isAVI)
      {
      this->Script("pack %s -side left -fill both -expand t",
        qualityEntryLabel->GetWidgetName());
      }
    this->Script("pack %s -side top -pady 5", frame->GetWidgetName());

    dlg->Invoke();

    int quality = 2;
    if (lowButton->GetSelectedState())
      {
      quality = 0;
      }
    if (mediumButton->GetSelectedState())
      {
      quality = 1;
      }

    int width = widthEntry->GetWidget()->GetValueAsInt();
    int height = origHeight;
    height = heightEntry->GetWidget()->GetValueAsInt();
    double framerate = (isMPEG)?
      framerateEntry->GetWidget()->GetValueAsDouble() : 1.0;
    if (!framerate) 
      {
      framerate = 1.0;
      }
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
    framerateEntry->Delete();
    qualityEntryLabel->Delete();
    frame->Delete();
    dlg->Delete();

    this->AnimationScene->SaveImages(fileRoot.c_str(), ext, width, height, framerate, quality);
    view->SetRenderWindowSize(origWidth, origHeight);
    }

  saveDialog->Delete();
  saveDialog = NULL;
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::SaveGeometry()
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(this->GetApplication());
  vtkKWLoadSaveDialog* saveDialog = pvApp->NewLoadSaveDialog();
  this->GetApplication()->RetrieveDialogLastPathRegistryValue(saveDialog, "SaveGeometryFile");
  saveDialog->SetParent(this);
  saveDialog->SaveDialogOn();
  saveDialog->Create(this->GetApplication());
  saveDialog->SetTitle("Save Animation Geometry");
  saveDialog->SetFileTypes("{{ParaView Data Files} {.pvd}}");
  if(saveDialog->Invoke() && (strlen(saveDialog->GetFileName()) > 0))
    {
    this->GetApplication()->SaveDialogLastPathRegistryValue(saveDialog, "SaveGeometryFile");
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
        this->AnimationScene->SetDuration(numOfTimeSteps);
        this->AnimationScene->SetPlayModeToSequence();
        this->AnimationScene->SetFrameRate(1);
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
  //TODO: verify this method...seems obsolete and incorrect!
  char* sourcekey = this->GetSourceKey(proxyname);
  if (!sourcekey)
    {
    vtkErrorMacro("Cannot find source for proxy " << proxyname);
    return NULL;
    }
  vtkPVAnimationManagerInternals::StringToPVCueMap::iterator iter;
  iter = this->Internals->PVAnimationCues.find(sourcekey);
  delete[] sourcekey;

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
void vtkPVAnimationManager::SaveWindowGeometryToRegistry()
{
  if (this->IsCreated())
    {
    this->HAnimationInterface->SaveWindowGeometryToRegistry();
    }
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::RestoreWindowGeometryFromRegistry()
{
  this->HAnimationInterface->RestoreWindowGeometryFromRegistry();
}

//-----------------------------------------------------------------------------
void vtkPVAnimationManager::InvalidateAllGeometries()
{
  this->AnimationScene->InvalidateAllGeometries();
}

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
  os << indent << "InRecording: " << this->InRecording << endl;

  os << indent << "ActiveTrackSelector: ";
  if (this->ActiveTrackSelector)
    {
    this->ActiveTrackSelector->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }
}

//-----------------------------------------------------------------------------
#ifndef VTK_LEGACY_REMOVE
void vtkPVAnimationManager::SetCurrentTime(double ntime)
{
  vtkGenericWarningMacro("vtkPVAnimationManager::SetCurrentTime was deprecated for ParaView 2.4 and will be removed in a future version.  Use vtkPVAnimationManager::SetAnimationTime instead.");
  this->SetAnimationTime(ntime);
}
#endif
