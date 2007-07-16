/*=========================================================================

  Program:   ParaView
  Module:    vtkSMAnimationSceneProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMAnimationSceneProxy.h"

#include "vtkAnimationScene.h"
#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVCacheSizeInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderViewProxy.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
class vtkSMAnimationSceneProxyInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkSMViewProxy> > 
    VectorOfViews;
  VectorOfViews ViewModules;

  void StillRenderAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      iter->GetPointer()->StillRender();
      }
    }

  void PassCacheTime(double cachetime)
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      (*iter)->SetCacheTime(cachetime);
      }
    }

  void PassUseCache(bool usecache)
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      (*iter)->SetUseCache(usecache);
      }
    }

  void CleanCacheAllViews()
    {
    /* FIXME:UDA
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderViewProxy* rm = vtkSMRenderViewProxy::SafeDownCast(
        iter->GetPointer());
      if (rm)
        {
        rm->InvalidateAllGeometries();
        }
      }
      */
    }

  void DisableInteractionAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderViewProxy* rm = vtkSMRenderViewProxy::SafeDownCast(
        iter->GetPointer());
      if (rm)
        {
        rm->GetInteractor()->Disable();
        }
      }
    }

  void EnableInteractionAllViews()
    {
    VectorOfViews::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderViewProxy* rm = vtkSMRenderViewProxy::SafeDownCast(
        iter->GetPointer());
      if (rm)
        {
        rm->GetInteractor()->Enable();
        }
      }
    }

};


vtkCxxRevisionMacro(vtkSMAnimationSceneProxy, "1.47");
vtkStandardNewMacro(vtkSMAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies = vtkCollection::New();
  this->AnimationCueProxiesIterator = this->AnimationCueProxies->NewIterator();
  this->GeometryCached = 0;
  this->OverrideStillRender = 0;
  this->Internals = new vtkSMAnimationSceneProxyInternals();
  this->PlayMode = SEQUENCE;
  this->CacheLimit = 100*1024; // 100 MBs.
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies->Delete();
  this->AnimationCueProxiesIterator->Delete();
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->AnimationCue = vtkAnimationScene::New();
  this->InitializeObservers(this->AnimationCue);
  this->ObjectsCreated = 1;

  this->Superclass::CreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::InitializeObservers(vtkAnimationCue* cue)
{
  this->Superclass::InitializeObservers(cue);
  if (cue)
    {
    cue->AddObserver(vtkCommand::StartEvent, this->Observer);
    cue->AddObserver(vtkCommand::EndEvent, this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::AddViewModule(vtkSMViewProxy* view)
{
  vtkSMAnimationSceneProxyInternals::VectorOfViews::iterator iter = 
    this->Internals->ViewModules.begin();
  for (; iter != this->Internals->ViewModules.end(); ++iter)
    {
    if (iter->GetPointer() == view)
      {
      // already added.
      return;
      }
    }
  this->Internals->ViewModules.push_back(view);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveViewModule(
  vtkSMViewProxy* view)
{
  vtkSMAnimationSceneProxyInternals::VectorOfViews::iterator iter = 
    this->Internals->ViewModules.begin();
  for (; iter != this->Internals->ViewModules.end(); ++iter)
    {
    if (iter->GetPointer() == view)
      {
      this->Internals->ViewModules.erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveAllViewModules()
{
  this->Internals->ViewModules.clear();
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::Play()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    this->Internals->DisableInteractionAllViews();
    this->Internals->PassUseCache(this->GetCaching()>0);
    scene->Play();
    this->Internals->PassUseCache(false);
    this->Internals->EnableInteractionAllViews();
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::IsInPlay()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->IsInPlay();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::Stop()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->Stop();
    }

}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetLoop(int loop)
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->SetLoop(loop);
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::GetLoop()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->GetLoop();
    }
  vtkErrorMacro("VTK object not created yet");
  return 0;
}
//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetFrameRate(double framerate)
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->SetFrameRate(framerate);
    }
}

//----------------------------------------------------------------------------
double vtkSMAnimationSceneProxy::GetFrameRate()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->GetFrameRate();
    }
  vtkErrorMacro("VTK object not created yet");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::AddCue(vtkSMProxy* proxy)
{
  vtkSMAnimationCueProxy* cue = vtkSMAnimationCueProxy::SafeDownCast(proxy);
  if (!cue)
    {
    return;
    }
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (!scene)
    {
    return;
    }
  if (this->AnimationCueProxies->IsItemPresent(cue))
    {
    vtkErrorMacro("Animation cue already present in the scene");
    return;
    }
  // ensure that Cue objects have been created.
  if (!cue->GetObjectsCreated())
    {
    cue->UpdateVTKObjects();
    }
  scene->AddCue(cue->GetAnimationCue());
  this->AnimationCueProxies->AddItem(cue);

  // since scene is a consumer of the cue, when cue gets modified, the scene's
  // MarkModified() will be called.
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::RemoveCue(vtkSMProxy* proxy)
{
  vtkSMAnimationCueProxy* smCue = vtkSMAnimationCueProxy::SafeDownCast(proxy);
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (!smCue || !scene)
    {
    return;
    }
  if (!this->AnimationCueProxies->IsItemPresent(smCue))
    {
    return;
    }
  scene->RemoveCue(smCue->GetAnimationCue());
  this->AnimationCueProxies->RemoveItem(smCue);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetPlayMode(int mode)
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);

  this->PlayMode = mode;
  if (scene)
    {
    scene->SetPlayMode(mode);
    }
  // Caching is disabled when play mode is real time.
  if (mode == vtkAnimationScene::PLAYMODE_REALTIME && this->Caching)
    {
    // We clean cache and hence forth, we dont call CacheUpdate()
    // this will make sure that cache is not used when playing in 
    // real time.
    this->CleanCache();
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::GetPlayMode()
{
  return this->PlayMode;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::StartCueInternal(void* info)
{
  if (!this->OverrideStillRender)
    {
    this->Internals->StillRenderAllViews();
    }
  this->Superclass::StartCueInternal(info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::TickInternal(void* info)
{
  this->CacheUpdate(info);

  if (!this->OverrideStillRender)
    {
    // Render All Views.
    this->Internals->StillRenderAllViews();
    }

  this->Superclass::TickInternal(info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::EndCueInternal(void* info)
{
  this->CacheUpdate(info);
  if (!this->OverrideStillRender)
    {
    this->Internals->StillRenderAllViews();
    }
  this->Superclass::EndCueInternal(info);

}

//-----------------------------------------------------------------------------
bool vtkSMAnimationSceneProxy::CheckCacheSizeWithinLimit()
{
  vtkSmartPointer<vtkPVCacheSizeInformation> info = 
    vtkSmartPointer<vtkPVCacheSizeInformation>::New();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->GatherInformation(this->ConnectionID,
    vtkProcessModule::RENDER_SERVER, info, pm->GetProcessModuleID());

  vtkSmartPointer<vtkPVCacheSizeInformation> clientinfo = 
    vtkSmartPointer<vtkPVCacheSizeInformation>::New();
  clientinfo->CopyFromObject(pm);
  clientinfo->AddInformation(info);

  return (clientinfo->GetCacheSize() < static_cast<unsigned long>(this->CacheLimit));
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CacheUpdate(void* info)
{
  if (!this->GetCaching() || 
      this->GetPlayMode() == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    return;
    }

  // Check if the cache has overgrown the limit.
  int cachefull = this->CheckCacheSizeWithinLimit()? 0 : 1;

  // Update cache full status on the cache size keeper so that all update
  // suppressors i.e. cache maintainer will be made aware.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream; 
  stream  << vtkClientServerStream::Invoke
          << pm->GetProcessModuleID()
          << "GetCacheSizeKeeper"
          << vtkClientServerStream::End;
  stream  << vtkClientServerStream::Invoke
          << vtkClientServerStream::LastResult
          << "SetCacheFull"
          << cachefull
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
    vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER,
    stream);

  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(info);

  double cachetime = cueInfo->AnimationTime;
  this->Internals->PassCacheTime(cachetime);
  this->GeometryCached = 1;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CleanCache()
{
  if (this->GeometryCached)
    {    
    this->Internals->CleanCacheAllViews();
    this->GeometryCached = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetAnimationTime(double time)
{
  if (this->AnimationCue)
    {
    this->Internals->PassUseCache(this->GetCaching()>0);
    this->AnimationCue->Initialize();
    this->AnimationCue->Tick(time,0);
    this->Internals->PassUseCache(false);
    }
}

//----------------------------------------------------------------------------
unsigned int vtkSMAnimationSceneProxy::GetNumberOfViewModules()
{
  return this->Internals->ViewModules.size();
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMAnimationSceneProxy::GetViewModule(
  unsigned int cc)
{
  if (cc < this->Internals->ViewModules.size())
    {
    return this->Internals->ViewModules[cc];
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::ExecuteEvent(
  vtkObject* wdg, unsigned long event, void* calldata)
{
  if (event == vtkCommand::StartEvent || event == vtkCommand::EndEvent)
    {
    this->InvokeEvent(event);
    return;
    }
  this->Superclass::ExecuteEvent(wdg, event, calldata);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "PlayMode: "<< this->PlayMode << endl;
  os << indent << "OverrideStillRender: " << this->OverrideStillRender << endl;
  os << indent << "CacheLimit: " << this->CacheLimit << endl;
}
