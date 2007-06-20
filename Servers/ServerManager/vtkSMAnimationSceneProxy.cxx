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
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
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
    VectorOfViewModules;
  VectorOfViewModules ViewModules;

  void StillRenderAllViews()
    {
    VectorOfViewModules::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      iter->GetPointer()->StillRender();
      }
    }

  void CacheUpdateAllViews(int index, int max)
    {
    (void) index;
    (void) max;
    /* FIXME:UDA
    VectorOfViewModules::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderViewProxy* ren = vtkSMRenderViewProxy::SafeDownCast(*iter);
      if (ren)
        {
        ren->CacheUpdate(index, max);
        }
      }
      */
    }

  void CleanCacheAllViews()
    {
    /* FIXME:UDA
    VectorOfViewModules::iterator iter = this->ViewModules.begin();
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
    VectorOfViewModules::iterator iter = this->ViewModules.begin();
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
    VectorOfViewModules::iterator iter = this->ViewModules.begin();
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


vtkCxxRevisionMacro(vtkSMAnimationSceneProxy, "1.43");
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
  vtkSMAnimationSceneProxyInternals::VectorOfViewModules::iterator iter = 
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
  vtkSMAnimationSceneProxyInternals::VectorOfViewModules::iterator iter = 
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
void vtkSMAnimationSceneProxy::SetCaching(int enable)
{
  this->Superclass::SetCaching(enable);
  vtkCollectionIterator* iter = this->AnimationCueProxies->NewIterator();
  
  for (iter->InitTraversal();
    !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMAnimationCueProxy* cue = 
      vtkSMAnimationCueProxy::SafeDownCast(iter->GetCurrentObject());
    cue->SetCaching(enable);
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::Play()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    this->Internals->DisableInteractionAllViews();
    scene->Play();
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
  cue->SetCaching(this->GetCaching());
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
    /*
    vtkWarningMacro("Disabling caching. "
      "Caching not available in Real Time mode.");
    this->SetCaching(0);
    */
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

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CacheUpdate(void* info)
{
  if (!this->GetCaching() || 
      this->GetPlayMode() == vtkAnimationScene::PLAYMODE_REALTIME)
    {
    return;
    }
  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(info);

  double etime = this->GetEndTime();
  double stime = this->GetStartTime();

  int index = 
    static_cast<int>((cueInfo->AnimationTime - stime) * this->GetFrameRate());

  int maxindex = 
    static_cast<int>((etime - stime) * this->GetFrameRate()) + 1; 

  this->Internals->CacheUpdateAllViews(index, maxindex);
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
    this->AnimationCue->Initialize();
    this->AnimationCue->Tick(time,0);
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
}
