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

#include "vtkAnimationCue.h"
#include "vtkAnimationScene.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkErrorCode.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMXMLPVAnimationWriterProxy.h"
#include "vtkToolkits.h"

#include <vtksys/SystemTools.hxx>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
class vtkSMAnimationSceneProxyInternals
{
public:
  typedef vtkstd::vector<vtkSmartPointer<vtkSMAbstractViewModuleProxy> > 
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
    VectorOfViewModules::iterator iter = this->ViewModules.begin();
    for (; iter != this->ViewModules.end(); ++iter)
      {
      vtkSMRenderModuleProxy* ren = vtkSMRenderModuleProxy::SafeDownCast(*iter);
      if (ren)
        {
        ren->CacheUpdate(index, max);
        }
      }
    }
};


vtkCxxRevisionMacro(vtkSMAnimationSceneProxy, "1.32");
vtkStandardNewMacro(vtkSMAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies = vtkCollection::New();
  this->AnimationCueProxiesIterator = this->AnimationCueProxies->NewIterator();
  this->RenderModuleProxy = 0;
  this->GeometryCached = 0;
  
  this->GeometryWriter = 0;

  this->Internals = new vtkSMAnimationSceneProxyInternals();
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies->Delete();
  this->AnimationCueProxiesIterator->Delete();

  if (this->GeometryWriter)
    {
    this->GeometryWriter->Delete();
    this->GeometryWriter = 0;
    }

  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->AnimationCue = vtkAnimationScene::New();
  this->InitializeObservers(this->AnimationCue);
  this->ObjectsCreated = 1;

  this->Superclass::CreateVTKObjects(numObjects);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::AddViewModule(vtkSMAbstractViewModuleProxy* view)
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
  vtkSMAbstractViewModuleProxy* view)
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
void vtkSMAnimationSceneProxy::SaveGeometry(double time)
{
  if (!this->GeometryWriter)
    {
    return;
    }
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GeometryWriter->GetProperty("WriteTime"));
  dvp->SetElement(0, time);
  this->GeometryWriter->UpdateVTKObjects();
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::SaveGeometry(const char* filename)
{
  if (this->GeometryWriter || !this->RenderModuleProxy)
    {
    vtkErrorMacro("Inconsistent state! Cannot SaveGeometry");
    return 1;
    }
  vtkSMXMLPVAnimationWriterProxy* animWriter = 
    vtkSMXMLPVAnimationWriterProxy::SafeDownCast(vtkSMObject::GetProxyManager()
      ->NewProxy("writers","XMLPVAnimationWriter"));
  if (!animWriter)
    {
    vtkErrorMacro("Failed to create XMLPVAnimationWriter proxy.");
    return 1;
    }
  animWriter->SetConnectionID(this->ConnectionID);

  this->SaveFailed = 0;
  this->SetAnimationTime(0);
  this->GeometryWriter = animWriter;

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    animWriter->GetProperty("FileName"));
  svp->SetElement(0,filename);
  animWriter->UpdateVTKObjects();

  vtkSMProxyIterator* proxyIter = vtkSMProxyIterator::New();
  proxyIter->SetMode(vtkSMProxyIterator::ONE_GROUP);
  proxyIter->Begin("displays");
  for (; !proxyIter->IsAtEnd(); proxyIter->Next())
    {
    vtkSMDataObjectDisplayProxy* sDisp = vtkSMDataObjectDisplayProxy::SafeDownCast(
      proxyIter->GetProxy());
    // only the data object displays are saved.
    if (sDisp && sDisp->GetVisibilityCM())
      {
      sDisp->SetInputAsGeometryFilter(animWriter);
      }
    }
  proxyIter->Delete();

  vtkSMProperty* p = animWriter->GetProperty("Start");
  p->Modified();
  animWriter->UpdateVTKObjects();

  // Play the animation.
  int oldMode = this->GetPlayMode();
  int old_loop = this->GetLoop();
  this->SetLoop(0);
  this->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
  this->Play();
  this->SetPlayMode(oldMode);
  this->SetLoop(old_loop);
 
  p = animWriter->GetProperty("Finish");
  p->Modified();
  animWriter->UpdateVTKObjects();

  
  if (animWriter->GetErrorCode() == vtkErrorCode::OutOfDiskSpaceError)
    {
    this->SaveFailed = vtkErrorCode::OutOfDiskSpaceError;
    }
  animWriter->Delete();
  this->GeometryWriter = NULL;
  return this->SaveFailed;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SaveInBatchScript(ofstream* file)
{
  this->Superclass::SaveInBatchScript(file);

  ostrstream batchNameStr;
  batchNameStr << "pvTemp" << this->GetSelfIDAsString()
               << ends;
  char* batchName = batchNameStr.str();;

  *file << "  [$" << batchName << " GetProperty Loop]"
    << " SetElements1 " << this->GetLoop() << endl;
  *file << "  [$" << batchName << " GetProperty FrameRate]"
    << " SetElements1 " << this->GetFrameRate() << endl;
  *file << "  [$" << batchName << " GetProperty PlayMode]"
    << " SetElements1 " << this->GetPlayMode() << endl;
  
  *file << "  $" << batchName << " SetRenderModuleProxy $pvTemp" 
        << this->RenderModuleProxy->GetSelfIDAsString()
        << endl;
  *file << "  $" << batchName << " UpdateVTKObjects" << endl;
  *file << endl;
  vtkCollectionIterator* iter = this->AnimationCueProxiesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkSMAnimationCueProxy* cue = 
      vtkSMAnimationCueProxy::SafeDownCast(iter->GetCurrentObject());
    if (cue)
      {
      cue->SaveInBatchScript(file);
      *file << "  [$" << batchName << " GetProperty Cues]"
        " AddProxy $pvTemp" << cue->GetSelfIDAsString() << endl;
      *file << "  $" << batchName << " UpdateVTKObjects" << endl;
      *file << endl;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::Play()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    int old_enable = 0;
    vtkRenderWindowInteractor *iren=0;
    if (this->RenderModuleProxy)
      {
      iren = this->RenderModuleProxy->GetInteractor();
      old_enable = iren?iren->GetEnabled():0;
      if (old_enable)
        {
        iren->Disable();
        }
      }
    scene->Play();
    if (old_enable && iren)
      {
      iren->Enable();
      }
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::IsInPlay()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    scene->IsInPlay();
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
  if (scene)
    {
    scene->SetPlayMode(mode);
    }
  // Caching is disabled when play mode is real time.
  if (mode == vtkAnimationScene::PLAYMODE_REALTIME && this->Caching)
    {
    vtkWarningMacro("Disabling caching. "
      "Caching not available in Real Time mode.");
    this->SetCaching(0);
    }
}

//----------------------------------------------------------------------------
int vtkSMAnimationSceneProxy::GetPlayMode()
{
  vtkAnimationScene* scene = vtkAnimationScene::SafeDownCast(
    this->AnimationCue);
  if (scene)
    {
    return scene->GetPlayMode();
    }
  vtkErrorMacro("VTK object was not created");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::StartCueInternal(void* info)
{
  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->StillRender();
    }
  this->Internals->StillRenderAllViews();
  this->Superclass::StartCueInternal(info);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::TickInternal(void* info)
{
  this->CacheUpdate(info);

  // Render All Views.
  this->Internals->StillRenderAllViews();

  this->Superclass::TickInternal(info);
  vtkAnimationCue::AnimationCueInfo *cueInfo = reinterpret_cast<
    vtkAnimationCue::AnimationCueInfo*>(info);
  this->SaveGeometry(cueInfo->AnimationTime);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::EndCueInternal(void* info)
{
  this->CacheUpdate(info);
  this->Internals->StillRenderAllViews();
  if (this->RenderModuleProxy)
    {
    this->RenderModuleProxy->StillRender();
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
  if (this->GeometryCached && this->RenderModuleProxy)
    {    
    this->RenderModuleProxy->InvalidateAllGeometries();
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
vtkSMAbstractViewModuleProxy* vtkSMAnimationSceneProxy::GetViewModule(
  unsigned int cc)
{
  if (cc < this->Internals->ViewModules.size())
    {
    return this->Internals->ViewModules[cc];
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
