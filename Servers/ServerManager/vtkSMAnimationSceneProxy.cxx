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
#include "vtkObjectFactory.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkSMDisplayWindowProxy.h"
vtkCxxRevisionMacro(vtkSMAnimationSceneProxy, "1.1");
vtkStandardNewMacro(vtkSMAnimationSceneProxy);
vtkCxxSetObjectMacro(vtkSMAnimationSceneProxy, DisplayWindowProxy, vtkSMDisplayWindowProxy);

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies = vtkCollection::New();
  this->AnimationCueProxiesIterator = this->AnimationCueProxies->NewIterator();
  this->DisplayWindowProxy = 0;
  
}

//----------------------------------------------------------------------------
vtkSMAnimationSceneProxy::~vtkSMAnimationSceneProxy()
{
  this->AnimationCueProxies->Delete();
  this->AnimationCueProxiesIterator->Delete();
  this->SetDisplayWindowProxy(0);
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
void vtkSMAnimationSceneProxy::SaveInBatchScript(ofstream* file)
{
  this->Superclass::SaveInBatchScript(file);
  vtkClientServerID id = this->SelfID;

  *file << "  [$pvTemp" << id << " GetProperty Loop]"
    << " SetElements1 " << this->GetLoop() << endl;
  *file << "  [$pvTemp" << id << " GetProperty FrameRate]"
    << " SetElements1 " << this->GetFrameRate() << endl;
  *file << "  [$pvTemp" << id << " GetProperty PlayMode]"
    << " SetElements1 " << this->GetPlayMode() << endl;
//TODO: How to set this?
  *file << "  $pvTemp" << id << " SetDisplayWindowProxy $Ren1" << endl;
  *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
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
      *file << "  [$pvTemp" << id << " GetProperty Cue]"
        " RemoveAllProxies" << endl;
      *file << "  [$pvTemp" << id << " GetProperty Cue]"
        " AddProxy $pvTemp" << cue->GetID() << endl;
      *file << "  $pvTemp" << id << " UpdateVTKObjects" << endl;
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
    scene->Play();
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
  this->Superclass::StartCueInternal(info);
  if (this->DisplayWindowProxy)
    {
    //this->DisplayWindowProxy->UpdateVTKObjects();
    this->DisplayWindowProxy->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::TickInternal(void* info)
{
  this->Superclass::TickInternal(info);
  if (this->DisplayWindowProxy)
    {
    //this->DisplayWindowProxy->UpdateVTKObjects();
    this->DisplayWindowProxy->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::EndCueInternal(void* info)
{
  this->Superclass::EndCueInternal(info);
  if (this->DisplayWindowProxy)
    {
    //this->DisplayWindowProxy->UpdateVTKObjects();
    this->DisplayWindowProxy->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::SetCurrentTime(double time)
{
  this->AnimationCue->Initialize();
  this->AnimationCue->Tick(time,0);
}

//----------------------------------------------------------------------------
void vtkSMAnimationSceneProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
