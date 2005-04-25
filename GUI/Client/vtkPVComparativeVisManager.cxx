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
#include "vtkInteractorStyleTrackballMultiActor.h"
#include "vtkKWEvent.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVAnimationCueTree.h"
#include "vtkPVAnimationManager.h"
#include "vtkPVApplication.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVHorizontalAnimationInterface.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMSimpleDisplayProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkPVComparativeVisManager);
vtkCxxRevisionMacro(vtkPVComparativeVisManager, "1.6");

vtkCxxSetObjectMacro(
  vtkPVComparativeVisManager, Application, vtkPVApplication);

struct vtkPVComparativeVisManagerInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkPVAnimationCue> > CuesType;
  CuesType Cues;

  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > ProxiesType;
  typedef vtkstd::vector<ProxiesType> ProxiesVectorType;
  ProxiesVectorType Caches;
  ProxiesVectorType Displays;

  typedef vtkstd::vector<double> BoundsType;
  typedef vtkstd::vector<BoundsType> BoundsVectorType;
  BoundsVectorType Bounds;

  vtkstd::vector<unsigned int> NumberOfParameterValues;

  vtkstd::vector<unsigned int> Indices;
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
  this->IStyle = 0;
}

//-----------------------------------------------------------------------------
vtkPVComparativeVisManager::~vtkPVComparativeVisManager()
{
  this->SetApplication(0);
  delete this->Internal;
  if (this->IStyle)
    {
    this->IStyle->Delete();
    }
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

  this->Internal->Caches.clear();
  this->Internal->Displays.clear();
  this->Internal->Bounds.clear();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::TraverseCue(vtkPVAnimationCue* cueTree)
{
  if (!cueTree)
    {
    return;
    }
  vtkPVAnimationCueTree* tree = vtkPVAnimationCueTree::SafeDownCast(cueTree);
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
    if (cueTree->GetNumberOfKeyFrames() > 0)
      {
      this->Internal->Cues.push_back(cueTree);
      }
    }

}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::RemoveAllCues()
{
  this->Internal->Cues.clear();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::Process()
{
  this->TraverseAllCues();

  vtkPVWindow* window = this->Application->GetMainWindow();
  this->IStyle = 
    vtkInteractorStyleTrackballMultiActor::New();
  this->IStyle->SetApplication(this->Application);
  window->ChangeInteractorStyle(2);
  window->GetInteractor()->SetInteractorStyle(this->IStyle);
  window->SetCurrentPVSource(0);
  window->SetPropertiesVisiblity(0);
  window->SetToolbarVisibility("tools", 0);
  window->SetToolbarVisibility("camera", 0);
  window->SetToolbarVisibility("interaction", 0);

  this->PlayOne(0);

  this->ConnectAllGeometry();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::SetNumberOfParameterValues(
  unsigned int idx, unsigned int numValues)
{
  unsigned int prevSize = this->Internal->NumberOfParameterValues.size();
  if (idx >= prevSize)
    {
    this->Internal->NumberOfParameterValues.resize(idx+1);
    for (unsigned int i=prevSize; i<idx; i++)
      {
      this->Internal->NumberOfParameterValues[i] = 1;
      }
    }
  this->Internal->NumberOfParameterValues[idx] = numValues;
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
  int prevStat = aMan->GetCacheGeometry();
  aMan->SetCacheGeometry(0);
  player->Play();
  aMan->SetCacheGeometry(prevStat);

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
          this->StoreGeometry();
          }
        }
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::StoreGeometry()
{
  unsigned int prevSize = this->Internal->Caches.size();
  this->Internal->Caches.resize(prevSize+1);
  this->Internal->Displays.resize(prevSize+1);
  this->Internal->Bounds.resize(prevSize+1);

  this->Internal->Bounds[prevSize].resize(6);
  double* totBounds = 
    &this->Internal->Bounds[prevSize][0];
  int ii;
  for (ii=0; ii<6; ii+=2)
    {
    totBounds[ii] = VTK_DOUBLE_MAX;
    }
  for (ii=1; ii<6; ii+=2)
    {
    totBounds[ii] = VTK_DOUBLE_MIN;
    }
  
  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();
  vtkCollection* displays = ren->GetDisplays();
  vtkCollectionIterator* iter = displays->NewIterator();
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMSimpleDisplayProxy* pDisp = vtkSMSimpleDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp && pDisp->GetVisibilityCM())
      {
      vtkSMProxyManager* proxMan = vtkSMProxyManager::GetProxyManager();
      vtkSMProxy* proxy = 
        proxMan->NewProxy("ComparativeVisHelpers", "GeometryCache");
      vtkSMProxyProperty* prop = 
        vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("AddGeometry"));
      prop->AddProxy(pDisp->GetMapperProxy());
      proxy->UpdateVTKObjects();
      this->Internal->Caches[prevSize].push_back(proxy);
      proxy->Delete();
      
      vtkSMProxyManager* proxM = vtkSMProxyManager::GetProxyManager();
      vtkSMProxy* display = proxM->NewProxy("displays", pDisp->GetXMLName());
      if (display)
        {
        vtkSMProxyProperty* input = 
          vtkSMProxyProperty::SafeDownCast(display->GetProperty("Input"));
        input->AddProxy(proxy);
        display->UpdateVTKObjects();
        display->DeepCopy(pDisp, "vtkSMProxyProperty");
        display->GetProperty("LookupTable")->DeepCopy(
          pDisp->GetProperty("LookupTable"));
        display->UpdateVTKObjects();
        this->Internal->Displays[prevSize].push_back(display);
        display->Delete();
        }
      vtkPVGeometryInformation* geomInfo = pDisp->GetGeometryInformation();
      if (geomInfo)
        {
        double bounds[6];
        geomInfo->GetBounds(bounds);
        this->AddBounds(bounds, totBounds);
        }
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::GetIndex(unsigned int paramIdx, 
                                          unsigned int gidx)
{
  unsigned int numParams = this->Internal->NumberOfParameterValues.size();
  unsigned int nidx;

  unsigned int prevTotal=0;
  for(unsigned int prevIdx=0; prevIdx<paramIdx; prevIdx++)
    {
    unsigned int product=1;
    for (nidx=prevIdx+1; nidx<numParams; nidx++)
      {
      product *= this->Internal->NumberOfParameterValues[nidx];
      }
    prevTotal += product*this->Internal->Indices[prevIdx];
    }
  unsigned int offsetIdx = gidx - prevTotal;

  unsigned int product2=1;
  for (nidx=paramIdx+1; nidx<numParams; nidx++)
    {
    product2 *= this->Internal->NumberOfParameterValues[nidx];
    }
  this->Internal->Indices[paramIdx] = offsetIdx / product2;
  if (paramIdx < numParams-1)
    {
    this->GetIndex(paramIdx+1, gidx);
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::GetIndices(unsigned int gidx)
{
  unsigned int numParams = this->Internal->NumberOfParameterValues.size();
  this->Internal->Indices.resize(numParams);
  this->GetIndex(0, gidx);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::AddBounds(double bounds[6], double totalB[6])
{
  int ii;
  for (ii=0; ii<6; ii+=2)
    {
    if (bounds[ii] < totalB[ii])
      {
      totalB[ii] = bounds[ii];
      }
    }
  for (ii=1; ii<6; ii+=2)
    {
    if (bounds[ii] > totalB[ii])
      {
      totalB[ii] = bounds[ii];
      }
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVisManager::ConnectAllGeometry()
{
  vtkSMProxyManager* proxM = vtkSMProxyManager::GetProxyManager();
  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();

  int winSize[2];
  ren->GetServerRenderWindowSize(winSize);

  double biggestBounds[6] = {VTK_DOUBLE_MAX, VTK_DOUBLE_MIN, 
                             VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                             VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  unsigned int numEntries = this->Internal->Displays.size();
  unsigned int i;
  for(i=0; i<numEntries; i++)
    {
    double* bounds = &this->Internal->Bounds[i][0];
    this->AddBounds(bounds, biggestBounds);
    }
  int nx = this->Internal->NumberOfParameterValues[0];
  int ny = this->Internal->NumberOfParameterValues[1];
  double dataWidth = biggestBounds[1] - biggestBounds[0];
  double dataHeight = biggestBounds[3] - biggestBounds[2];
  double displayWidth = winSize[0] / (double)nx;
  double displayHeight = winSize[1] / (double)ny;
  double aspectRatio = displayHeight / displayWidth;
  double cellSpacing[2];
  double bb[4];
  memcpy(bb, biggestBounds, 4*sizeof(double));
  if ( dataHeight/aspectRatio > dataWidth )
    {
    cellSpacing[0] = dataHeight/aspectRatio;
    cellSpacing[1] = dataHeight;
    bb[1] = biggestBounds[0] +
      (biggestBounds[1]-biggestBounds[0])*cellSpacing[0]/dataWidth;
    }
  else
    {
    cellSpacing[0] = dataWidth;
    cellSpacing[1] = dataWidth*aspectRatio;
    bb[3] = biggestBounds[2] +
      (biggestBounds[3]-biggestBounds[2])*cellSpacing[1]/dataHeight;
    }
  
  vtkSMProxyManager* proxMan = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* multiActorHelper = 
    proxMan->NewProxy("ComparativeVisHelpers", "MultiActorHelper");
  multiActorHelper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  vtkSMProxyProperty* actorsP = vtkSMProxyProperty::SafeDownCast(
    multiActorHelper->GetProperty("Actors"));

  vtkCollection* displays = ren->GetDisplays();
  vtkCollectionIterator* iter = displays->NewIterator();
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMSimpleDisplayProxy* pDisp = vtkSMSimpleDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp && pDisp->GetVisibilityCM())
      {
      pDisp->SetVisibilityCM(0);
      }
    }
  iter->Delete();

  for(i=0; i<numEntries; i++)
    {
    this->GetIndices(i);
    vtkPVComparativeVisManagerInternals::ProxiesType::iterator iter2 = 
      this->Internal->Displays[i].begin();
    double xPos = cellSpacing[0]*this->Internal->Indices[0];
    double yPos = cellSpacing[1]*this->Internal->Indices[1]; 

    for(; iter2 != this->Internal->Displays[i].end(); iter2++)
      {
      vtkSMSimpleDisplayProxy* display = vtkSMSimpleDisplayProxy::SafeDownCast(
        iter2->GetPointer());
      actorsP->AddProxy(display->GetActorProxy());
      vtkSMDoubleVectorProperty* position = 
        vtkSMDoubleVectorProperty::SafeDownCast(
          display->GetProperty("Position"));
      position->SetElements3(xPos, yPos, 0);
      
      vtkSMDoubleVectorProperty* origin = 
        vtkSMDoubleVectorProperty::SafeDownCast(
          display->GetProperty("Origin"));
      origin->SetElements3(xPos + (biggestBounds[0]+biggestBounds[1])/2, 
                           yPos + (biggestBounds[2]+biggestBounds[3])/2, 
                           (biggestBounds[4]+biggestBounds[5])/2);
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        ren->GetProperty("Displays"));
      pp->AddProxy(vtkSMDisplayProxy::SafeDownCast(display));

      vtkSMProxyProperty* clipPlanes = 
        vtkSMProxyProperty::SafeDownCast(display->GetProperty("ClippingPlanes"));

      vtkSMProxy* planeR = 
        proxM->NewProxy("implicit_functions", "Plane");
      planeR->SetServers(
        vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
      clipPlanes->AddProxy(planeR);
      vtkSMDoubleVectorProperty* porigin =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeR->GetProperty("Origin"));
      porigin->SetElements3(xPos+bb[1], 0, 0);
      vtkSMDoubleVectorProperty* pnormal =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeR->GetProperty("Normal"));
      pnormal->SetElements3(-1, 0, 0);
      planeR->UpdateVTKObjects();
      planeR->Delete();

      vtkSMProxy* planeL = 
        proxM->NewProxy("implicit_functions", "Plane");
      planeL->SetServers(
        vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
      clipPlanes->AddProxy(planeL);
      porigin =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeL->GetProperty("Origin"));
      porigin->SetElements3(xPos+bb[0], 0, 0);
      pnormal =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeL->GetProperty("Normal"));
      pnormal->SetElements3(1, 0, 0);
      planeL->UpdateVTKObjects();
      planeL->Delete();
      
      vtkSMProxy* planeD = 
        proxM->NewProxy("implicit_functions", "Plane");
      planeD->SetServers(
        vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
      clipPlanes->AddProxy(planeD);
      porigin =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeD->GetProperty("Origin"));
      porigin->SetElements3(0, yPos+bb[2], 0);
      pnormal =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeD->GetProperty("Normal"));
      pnormal->SetElements3(0, 1, 0);
      planeD->UpdateVTKObjects();
      planeD->Delete();

      vtkSMProxy* planeU = 
        proxM->NewProxy("implicit_functions", "Plane");
      planeU->SetServers(
        vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
      clipPlanes->AddProxy(planeU);
      porigin =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeU->GetProperty("Origin"));
      porigin->SetElements3(0, yPos+bb[3], 0);
      pnormal =
        vtkSMDoubleVectorProperty::SafeDownCast(
          planeU->GetProperty("Normal"));
      pnormal->SetElements3(0, -1, 0);
      planeU->UpdateVTKObjects();
      planeU->Delete();
      }
    }
  
  this->IStyle->SetHelperProxy(multiActorHelper);
  multiActorHelper->Delete();

  vtkSMDoubleVectorProperty* position = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      ren->GetProperty("CameraPosition"));
  position->SetElements3(0, 0, 1);

  vtkSMDoubleVectorProperty* focalP = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      ren->GetProperty("CameraFocalPoint"));
  focalP->SetElements3(0, 0, 0);

  vtkSMDoubleVectorProperty* viewUp = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      ren->GetProperty("CameraViewUp"));
  viewUp->SetElements3(0, 1, 0);

  vtkSMIntVectorProperty* parallelProj =
    vtkSMIntVectorProperty::SafeDownCast(
      ren->GetProperty("CameraParallelProjection"));
  parallelProj->SetElements1(1);

  ren->UpdateVTKObjects();
  ren->ResetCameraClippingRange();

  double totalBounds[6];
  totalBounds[0] = biggestBounds[0];
  totalBounds[1] = totalBounds[0] + cellSpacing[0]*nx;
  totalBounds[2] = biggestBounds[2];
  totalBounds[3] = biggestBounds[2] + cellSpacing[1]*ny;
  totalBounds[4] = biggestBounds[4];
  totalBounds[5] = biggestBounds[5];
  ren->ResetCamera(totalBounds);

  vtkSMDoubleVectorProperty* parallelScale = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      ren->GetProperty("CameraParallelScale"));
  parallelScale->SetElements1((totalBounds[3]-totalBounds[2])/2);
//  parallelScale->SetElements1(1);
  ren->UpdateVTKObjects();

  ren->StillRender();

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
