/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeVisManager.h"

#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkKWEvent.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVHorizontalAnimationInterface.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSimpleDisplayProxy.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVComparativeVisManager);
vtkCxxRevisionMacro(vtkPVComparativeVisManager, "1.1.2.1");

vtkCxxSetObjectMacro(
  vtkPVComparativeVisManager, Application, vtkPVApplication);

struct vtkPVComparativeVisManagerInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkPVAnimationCue> > CuesType;
  CuesType Cues;

  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > ProxiesType;
  typedef vtkstd::vector<ProxiesType> ProxiesVectorType;
  ProxiesVectorType ProxiesVector;

  vtkstd::vector<unsigned int> NumberOfParameterValues;
};

//*****************************************************************************
class vtkCVAnimationSceneObserver : public vtkCommand
{
public:
  static vtkCVAnimationSceneObserver* New()
    {
    return new vtkCVAnimationSceneObserver;
    }
  virtual void Execute(vtkObject* obj, unsigned long event, void*)
    {
      this->Manager->ExecuteEvent(obj, event, this->ParameterIndex);
    }

  vtkPVComparativeVisManager* Manager;
  unsigned int ParameterIndex;

protected:
  vtkCVAnimationSceneObserver()
    {
      this->Manager = 0;
      this->ParameterIndex = 0;
    }
};

//-----------------------------------------------------------------------------
vtkPVComparativeVisManager::vtkPVComparativeVisManager()
{
  this->Application = 0;
  this->Internal = new vtkPVComparativeVisManagerInternals;
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisManager::~vtkPVComparativeVisManager()
{
  this->SetApplication(0);
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::TraverseAllCues()
{
  if (!this->Application)
    {
    return;
    }

  vtkPVAnimationManager* aMan = 
    this->Application->GetMainWindow()->GetAnimationManager();
  vtkPVHorizontalAnimationInterface* interface =
    aMan->GetHAnimationInterface();

  if (!interface)
    {
    return;
    }

  this->TraverseCue(interface->GetParentTree());
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::TraverseCue(vtkPVAnimationCue* cue)
{
  if (!cue)
    {
    return;
    }
  vtkPVAnimationCueTree* tree = vtkPVAnimationCueTree::SafeDownCast(cue);
  if (tree)
    {
    vtkCollectionIterator* iter = tree->NewChildrenIterator();
    iter->InitTraversal();
    while(!iter->IsDoneWithTraversal())
      {
      vtkPVAnimationCue* cue = vtkPVAnimationCue::SafeDownCast(
        iter->GetCurrentObject());
      this->TraverseCue(cue);
      iter->GoToNextItem();
      }
    iter->Delete();
    }
  else
    {
    if (cue->GetNumberOfKeyFrames() > 0)
      {
      this->Internal->Cues.push_back(cue);
      }
    }

  unsigned int numCues = this->Internal->Cues.size();
  this->Internal->NumberOfParameterValues.resize(numCues);
  unsigned int numParams=1;
  unsigned int i;
  int num=5;
  for(i=0; i<numCues; i++)
    {
    this->Internal->NumberOfParameterValues[i] = num;
    numParams *= num;
    num--;
    if (num == 0)
      {
      break;
      }
    }

  this->Internal->ProxiesVector.resize(numParams);
  for(i=0; i<numParams; i++)
    {
    this->Internal->ProxiesVector.clear();
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::RemoveAllCues()
{
  this->Internal->Cues.clear();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::PlayAll()
{
  this->PlayOne(0);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::PlayOne(unsigned int idx)
{
  if (!this->Application)
    {
    return;
    }

  if (idx >= this->Internal->Cues.size())
    {
    return;
    }
  vtkCVAnimationSceneObserver* observer = vtkCVAnimationSceneObserver::New();
  observer->Manager = this;
  observer->ParameterIndex = idx;

  vtkSMAnimationSceneProxy* player = vtkSMAnimationSceneProxy::New();
  player->UpdateVTKObjects();
  player->AddCue(this->Internal->Cues[idx]->GetCueProxy());
  player->UpdateVTKObjects();
  player->AddObserver(vtkCommand::AnimationCueTickEvent, observer);

  player->SetFrameRate(1);
  player->SetStartTime(0);
  player->SetEndTime(this->Internal->NumberOfParameterValues[idx]-1);
  player->SetPlayMode(0);
  player->UpdateVTKObjects();

  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();
  ren->InvalidateAllGeometries();

  vtkPVAnimationManager* aMan = 
    this->Application->GetMainWindow()->GetAnimationManager();
  int prevStat = aMan->GetOverrideCache();
  aMan->SetOverrideCache(1);
  player->Play();
  aMan->SetOverrideCache(prevStat);

  observer->Delete();
  player->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::ExecuteEvent(
  vtkObject* , unsigned long event, unsigned int paramIndex)
{
  switch(event)
    {
    case vtkCommand::AnimationCueTickEvent:
      if (this->Application)
        {
        if (paramIndex < this->Internal->Cues.size() - 1)
          {
          this->PlayOne(paramIndex+1);
          }
        else
          {
          this->Application->GetMainView()->ForceRender();
          this->SaveGeometry();
          }
        }
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::SaveAllGeometry()
{
  vtkSMProxyManager* proxM = vtkSMProxyManager::GetProxyManager();
  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();

  vtkPVComparativeVisManagerInternals::ProxiesVectorType::iterator iter1 = 
    this->Internal->ProxiesVector.begin();
  for(; iter1 != this->Internal->ProxiesVector.end(); iter1++)
    {
    vtkPVComparativeVisManagerInternals::ProxiesType::iterator iter2 = 
      iter1->begin();
    for(; iter2 != iter1->end(); iter2++)
      {
      vtkSMProxy* proxy = iter2->GetPointer();
      vtkSMProxy* display = proxM->NewProxy("displays", "CompositeDisplay");
      if (display)
        {
        vtkSMProxyProperty* input = 
          vtkSMProxyProperty::SafeDownCast(display->GetProperty("Input"));
        input->AddProxy(proxy);
        vtkSMIntVectorProperty::SafeDownCast(
          display->GetProperty("Representation"))->SetElements1(2);
        display->UpdateVTKObjects();
        ren->AddDisplay(vtkSMDisplayProxy::SafeDownCast(display));
        display->Delete();
        }
      }
    }
  ren->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::SaveGeometry()
{
  unsigned int prevSize = this->Internal->ProxiesVector.size();
  this->Internal->ProxiesVector.resize(prevSize+1);
  cout << "--- Saving geometry: " << prevSize << endl;
   vtkSMRenderModuleProxy* ren =
     this->Application->GetRenderModuleProxy();
   vtkCollection* displays = ren->GetDisplays();
   vtkCollectionIterator* iter = displays->NewIterator();
   for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
     {
     vtkSMSimpleDisplayProxy* pDisp = vtkSMSimpleDisplayProxy::SafeDownCast(
       iter->GetCurrentObject());
     if (pDisp && pDisp->cmGetVisibility())
       {
       vtkSMProxyManager* proxMan = vtkSMProxyManager::GetProxyManager();
       vtkSMProxy* proxy = 
         proxMan->NewProxy("ComparativeVisHelpers", "GeometryCache");
       vtkSMProxyProperty* prop = 
         vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("AddGeometry"));
       prop->AddProxy(pDisp->GetMapperProxy());
       proxy->UpdateVTKObjects();
       this->Internal->ProxiesVector[prevSize].push_back(proxy);
       proxy->Delete();
       }
     }
   iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkPVComparativeVisManagerInternals::CuesType::iterator iter = 
    this->Internal->Cues.begin();
  os << indent << "Cues: " << endl;
  for(; iter != this->Internal->Cues.end(); iter++)
    {
    iter->GetPointer()->PrintSelf(os, indent.GetNextIndent());
    }
}
