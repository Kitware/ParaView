/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVAnimationScene.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVAnimationScene.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkPVAnimationScene);

//----------------------------------------------------------------------------
vtkPVAnimationScene::vtkPVAnimationScene()
{
  this->AnimationCues = vtkCollection::New();
  this->AnimationCuesIterator = this->AnimationCues->NewIterator();
  this->SceneTime = 0;
  this->InTick = false;
}

//----------------------------------------------------------------------------
vtkPVAnimationScene::~vtkPVAnimationScene()
{
  this->AnimationCuesIterator->Delete();
  this->AnimationCues->Delete();
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::AddCue(vtkAnimationCue* cue)
{
  if (this->AnimationCues->IsItemPresent(cue))
    {
    vtkErrorMacro("Animation cue already present in the scene");
    return;
    }

  this->AnimationCues->AddItem(cue);
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveCue(vtkAnimationCue* cue)
{
  this->AnimationCues->RemoveItem(cue);
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::RemoveAllCues()
{  
  this->AnimationCues->RemoveAllItems();
}
//----------------------------------------------------------------------------
int vtkPVAnimationScene::GetNumberOfCues()
{
  return this->AnimationCues->GetNumberOfItems();
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::InitializeChildren()
{
  // run thr all the cues and init them.
  vtkCollectionIterator *it = this->AnimationCuesIterator;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkAnimationCue* cue = 
      vtkAnimationCue::SafeDownCast(it->GetCurrentObject());
    if (cue)
      {
      cue->Initialize();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::FinalizeChildren()
{
  vtkCollectionIterator *it = this->AnimationCuesIterator;
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkAnimationCue* cue = 
      vtkAnimationCue::SafeDownCast(it->GetCurrentObject());
    if (cue)
      {
      cue->Finalize();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::StartCueInternal()
{
  this->Superclass::StartCueInternal();
  this->InitializeChildren();
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::TickInternal(
  double currenttime, double deltatime, double clocktime)
{
  bool previous = this->InTick;
  this->InTick = true;
  this->SceneTime = currenttime;

  vtkCollectionIterator* iter = this->AnimationCuesIterator;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkAnimationCue* cue = vtkAnimationCue::SafeDownCast(
      iter->GetCurrentObject());
    if (cue)
      {
      switch(cue->GetTimeMode())
        {
      case vtkAnimationCue::TIMEMODE_RELATIVE:
        cue->Tick(currenttime - this->StartTime, deltatime, clocktime);
        break;

      case vtkAnimationCue::TIMEMODE_NORMALIZED:
        cue->Tick( (currenttime - this->StartTime) / (this->EndTime - this->StartTime),
          deltatime / (this->EndTime - this->StartTime), clocktime);
        break;

      default:
        vtkErrorMacro("Invalid cue time mode");
        }
      }
    }

  this->Superclass::TickInternal(currenttime, deltatime, clocktime);
  this->InTick = previous;
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::EndCueInternal()
{
  this->FinalizeChildren();
  this->Superclass::EndCueInternal();
}

//----------------------------------------------------------------------------
void vtkPVAnimationScene::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
