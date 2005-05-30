/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVis.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVComparativeVis.h"

#include "vtkAnimationScene.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkKWEvent.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationCue.h"
#include "vtkPVApplication.h"
#include "vtkPVGeometryInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVRenderView.h"
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
#include "vtkTimerLog.h"

#include <vtkstd/vector>
#include <vtkstd/list>

vtkStandardNewMacro(vtkPVComparativeVis);
vtkCxxRevisionMacro(vtkPVComparativeVis, "1.1");

vtkCxxSetObjectMacro(
  vtkPVComparativeVis, Application, vtkPVApplication);

struct vtkPVComparativeVisInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > ProxiesType;
  typedef vtkstd::vector<ProxiesType> ProxiesVectorType;
  ProxiesVectorType Caches;
  ProxiesVectorType Displays;

  typedef vtkstd::vector<double> BoundsType;
  typedef vtkstd::vector<BoundsType> BoundsVectorType;
  BoundsVectorType Bounds;

  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > CuesType;
  CuesType Cues;
  vtkstd::vector<unsigned int> NumberOfPropertyValues;

  typedef vtkstd::vector<vtkSmartPointer<vtkPVAnimationCue> > AnimationCuesType;
  AnimationCuesType AnimationCues;

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
      this->Vis->ExecuteEvent(obj, event, this->PropertyIndex);
    }

  vtkPVComparativeVis* Vis;
  unsigned int PropertyIndex;

protected:
  vtkCVAnimationSceneObserver()
    {
      this->Vis = 0;
      this->PropertyIndex = 0;
    }
};

//-----------------------------------------------------------------------------
vtkPVComparativeVis::vtkPVComparativeVis()
{
  this->Application = 0;
  this->Internal = new vtkPVComparativeVisInternals;

  vtkSMProxyManager* proxMan = vtkSMProxy::GetProxyManager();
  this->MultiActorHelper = 
    proxMan->NewProxy("ComparativeVisHelpers", "MultiActorHelper");
  this->Name = 0;

  this->InFirstShow = 1;
  this->IsGenerated = 0;
}

//-----------------------------------------------------------------------------
vtkPVComparativeVis::~vtkPVComparativeVis()
{
  this->SetApplication(0);
  delete this->Internal;
  this->MultiActorHelper->Delete();
  this->SetName(0);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::Generate()
{
  this->Initialize();

  vtkTimerLog::MarkStartEvent("Play One (all)");
  this->PlayOne(0);
  vtkTimerLog::MarkEndEvent("Play One (all)");
  this->InFirstShow = 1;
  this->IsGenerated = 1;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::Initialize()
{
  this->Internal->Caches.clear();
  this->Internal->Displays.clear();
  this->Internal->Bounds.clear();
  this->IsGenerated = 0;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::RemoveAllProperties()
{
  this->Internal->Cues.clear();
  this->Internal->AnimationCues.clear();
  this->Internal->NumberOfPropertyValues.clear();

  this->Initialize();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::SetNumberOfPropertyValues(
  unsigned int idx, unsigned int numValues)
{
  unsigned int prevSize = this->Internal->NumberOfPropertyValues.size();
  if (idx >= prevSize)
    {
    this->Internal->NumberOfPropertyValues.resize(idx+1);
    for (unsigned int i=prevSize; i<idx; i++)
      {
      this->Internal->NumberOfPropertyValues[i] = 1;
      }
    }
  this->Internal->NumberOfPropertyValues[idx] = numValues;
}

//-----------------------------------------------------------------------------
unsigned int vtkPVComparativeVis::GetNumberOfProperties()
{
  return this->Internal->Cues.size();
}

//-----------------------------------------------------------------------------
vtkPVAnimationCue* vtkPVComparativeVis::GetAnimationCue(unsigned int idx)
{
  if (idx >= this->Internal->AnimationCues.size())
    {
    return 0;
    }
  return this->Internal->AnimationCues[idx];
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::AddProperty(
  vtkPVAnimationCue* acue, vtkSMProxy* cue, int numValues)
{
  if (!cue)
    {
    return;
    }
  this->Internal->Cues.push_back(cue);
  this->Internal->AnimationCues.push_back(acue);

  unsigned int numProperties = this->Internal->Cues.size();
  this->Internal->NumberOfPropertyValues.resize(numProperties);
  this->Internal->NumberOfPropertyValues[numProperties-1] = numValues;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::PlayOne(unsigned int idx)
{
  vtkTimerLog::MarkStartEvent("Play One");
  if (!this->Application)
    {
    return;
    }

  if (idx >= this->Internal->Cues.size())
    {
    return;
    }

  vtkCVAnimationSceneObserver* observer = vtkCVAnimationSceneObserver::New();
  observer->Vis = this;
  observer->PropertyIndex = idx;

  vtkSMAnimationSceneProxy* player = vtkSMAnimationSceneProxy::New();
  player->UpdateVTKObjects();
  player->AddCue(this->Internal->Cues[idx]);
  player->UpdateVTKObjects();
  player->AddObserver(vtkCommand::AnimationCueTickEvent, observer);

  player->SetFrameRate(1);
  player->SetStartTime(0);
  player->SetEndTime(this->Internal->NumberOfPropertyValues[idx]-1);
  player->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
  player->UpdateVTKObjects();

  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();
  ren->InvalidateAllGeometries();

  player->Play();

  observer->Delete();
  player->Delete();
  vtkTimerLog::MarkEndEvent("Play One");
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::ExecuteEvent(
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
          vtkTimerLog::MarkStartEvent("Force Render");
          this->Application->GetMainView()->ForceRender();
          vtkTimerLog::MarkEndEvent("Force Render");
          this->StoreGeometry();
          }
        }
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::StoreGeometry()
{
  cout << "IN STORE GEOMETRY" << endl;
  vtkTimerLog::MarkStartEvent("Store Geometry");
  unsigned int prevSize = this->Internal->Caches.size();
  this->Internal->Caches.resize(prevSize+1);
  this->Internal->Displays.resize(prevSize+1);
  this->Internal->Bounds.resize(prevSize+1);

  // Initialize bounds for this case
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
      cout << "Storing" << endl;
      // Geometry cache will make copies (shallow) of all geometry objects
      // and cache them.
      vtkSMProxyManager* proxM = vtkSMProxy::GetProxyManager();
      vtkSMProxy* proxy = 
        proxM->NewProxy("ComparativeVisHelpers", "GeometryCache");
      vtkSMProxyProperty* prop = 
        vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("AddGeometry"));
      prop->AddProxy(pDisp->GetMapperProxy());
      proxy->UpdateVTKObjects();
      this->Internal->Caches[prevSize].push_back(proxy);
      proxy->Delete();
      
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
      // Collect bounds of all geometry
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
  vtkTimerLog::MarkEndEvent("Store Geometry");
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::ComputeIndex(unsigned int paramIdx, unsigned int gidx)
{
  unsigned int numParams = this->Internal->NumberOfPropertyValues.size();
  unsigned int nidx;

  unsigned int prevTotal=0;
  for(unsigned int prevIdx=0; prevIdx<paramIdx; prevIdx++)
    {
    unsigned int product=1;
    for (nidx=prevIdx+1; nidx<numParams; nidx++)
      {
      product *= this->Internal->NumberOfPropertyValues[nidx];
      }
    prevTotal += product*this->Internal->Indices[prevIdx];
    }
  unsigned int offsetIdx = gidx - prevTotal;

  unsigned int product2=1;
  for (nidx=paramIdx+1; nidx<numParams; nidx++)
    {
    product2 *= this->Internal->NumberOfPropertyValues[nidx];
    }
  this->Internal->Indices[paramIdx] = offsetIdx / product2;
  if (paramIdx < numParams-1)
    {
    this->ComputeIndex(paramIdx+1, gidx);
    }
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::ComputeIndices(unsigned int gidx)
{
  unsigned int numParams = this->Internal->NumberOfPropertyValues.size();
  this->Internal->Indices.resize(numParams);
  this->ComputeIndex(0, gidx);
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::AddBounds(double bounds[6], double totalB[6])
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
int vtkPVComparativeVis::Show()
{
  if (!this->Application)
    {
    vtkErrorMacro("Application is not set. Cannot show.");
    return 0;
    }

  if (this->Internal->NumberOfPropertyValues.size() < 1 ||
      this->Internal->NumberOfPropertyValues.size() > 2)
    {
    vtkErrorMacro("This visualization only works with two properties. "
                  "Cannot display");
    return 0;
    }

  vtkSMProxyManager* proxM = vtkSMProxy::GetProxyManager();
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
  int nx = this->Internal->NumberOfPropertyValues[0];
  int ny = this->Internal->NumberOfPropertyValues[1];
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
  
  this->MultiActorHelper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  vtkSMProxyProperty* actorsP = vtkSMProxyProperty::SafeDownCast(
    this->MultiActorHelper->GetProperty("Actors"));

  for(i=0; i<numEntries; i++)
    {
    this->ComputeIndices(i);
    vtkPVComparativeVisInternals::ProxiesType::iterator iter2 = 
      this->Internal->Displays[i].begin();
    double xPos = cellSpacing[0]*this->Internal->Indices[0];
    double yPos = cellSpacing[1]*this->Internal->Indices[1]; 

    for(; iter2 != this->Internal->Displays[i].end(); iter2++)
      {
      vtkSMSimpleDisplayProxy* display = vtkSMSimpleDisplayProxy::SafeDownCast(
        iter2->GetPointer());

      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        ren->GetProperty("Displays"));
      pp->AddProxy(vtkSMDisplayProxy::SafeDownCast(display));

      if (this->InFirstShow)
        {
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
        
        vtkSMProxyProperty* clipPlanes = 
          vtkSMProxyProperty::SafeDownCast(
            display->GetProperty("ClippingPlanes"));
        
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
    }
  
  if (this->InFirstShow)
    {
    this->InFirstShow = 0;
    }

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

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::Hide()
{
  vtkSMRenderModuleProxy* ren =
    this->Application->GetRenderModuleProxy();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    ren->GetProperty("Displays"));

  unsigned int numEntries = this->Internal->Displays.size();
  for(unsigned int i=0; i<numEntries; i++)
    {
    vtkPVComparativeVisInternals::ProxiesType::iterator iter2 = 
      this->Internal->Displays[i].begin();
    for(; iter2 != this->Internal->Displays[i].end(); iter2++)
      {
      vtkSMSimpleDisplayProxy* display = vtkSMSimpleDisplayProxy::SafeDownCast(
        iter2->GetPointer());
      pp->RemoveProxy(vtkSMDisplayProxy::SafeDownCast(display));
      }
    }
  ren->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkPVComparativeVis::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkPVComparativeVisInternals::CuesType::iterator iter = 
    this->Internal->Cues.begin();
  os << indent << "Cues: " << endl;
  for(; iter != this->Internal->Cues.end(); iter++)
    {
    iter->GetPointer()->PrintSelf(os, indent.GetNextIndent());
    }
}
