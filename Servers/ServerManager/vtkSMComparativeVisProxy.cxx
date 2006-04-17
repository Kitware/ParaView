/*=========================================================================

  Program:   ParaView
  Module:    vtkSMComparativeVisProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMComparativeVisProxy.h"

#include "vtkAnimationScene.h"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVGeometryInformation.h"
#include "vtkProcessModule.h"
#include "vtkSMAnimationCueProxy.h"
#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyAdaptor.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMDataObjectDisplayProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkTimerLog.h"

#include <vtkstd/list>
#include <vtkstd/string>
#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMComparativeVisProxy);
vtkCxxRevisionMacro(vtkSMComparativeVisProxy, "1.17");

vtkCxxSetObjectMacro(vtkSMComparativeVisProxy, RenderModule, vtkSMRenderModuleProxy);

//PIMPL (private implementation)
struct vtkSMComparativeVisProxyInternals
{
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > ProxiesType;
  typedef vtkstd::vector<ProxiesType> ProxiesVectorType;
  // Geometry cache. These server side objects (sources) that store
  // the generated geometry.
  ProxiesVectorType Caches;
  // Display objects: geometry filters, mappers, actors etc.
  ProxiesVectorType Displays;
  // Labels
  ProxiesType Labels;

  typedef vtkstd::vector<double> BoundsType;
  typedef vtkstd::vector<BoundsType> BoundsVectorType;
  // The bounds of each frame
  BoundsVectorType Bounds;

  // These define the comparative visualization.
  typedef vtkstd::vector<vtkSmartPointer<vtkSMProxy> > CuesType;
  CuesType Cues;
  // The number of properties (parameters). Should be >= 1 for Generate().
  // Currently only 2 can be shown
  vtkstd::vector<unsigned int> NumberOfFramesInCue;

  // All the proxy vectors are 1D. ComputeIndices() convert an index
  // to these vectors to an nD array (where n is the number of parameters).
  // The result of ComputeIndices() is stored in this vector. 
  vtkstd::vector<unsigned int> Indices;

  // The names of the sources are used only by the gui.
  vtkstd::vector<vtkstd::string> SourceNames;

  // The tcl names of the sources are used only for saving scripts.
  vtkstd::vector<vtkstd::string> SourceTclNames;
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
      this->Vis->PropertyIndex = this->PropertyIndex;
    }

  vtkSMComparativeVisProxy* Vis;
  unsigned int PropertyIndex;

protected:
  vtkCVAnimationSceneObserver()
    {
      this->Vis = 0;
      this->PropertyIndex = 0;
    }
};

const double vtkSMComparativeVisProxy::BorderWidth = 0.02;

//-----------------------------------------------------------------------------
vtkSMComparativeVisProxy::vtkSMComparativeVisProxy()
{
  this->Internal = new vtkSMComparativeVisProxyInternals;

  vtkSMProxyManager* proxMan = vtkSMProxy::GetProxyManager();
  this->MultiActorHelper = 
    proxMan->NewProxy("ComparativeVisHelpers", "MultiActorHelper");
  this->MultiActorHelper->SetConnectionID(this->ConnectionID);
  this->VisName = 0;

  this->InFirstShow = 1;
  this->IsGenerated = 0;

  this->NumberOfFrames = 1;
  this->CurrentFrame = 0;

  this->ShouldAbort = 0;

  this->RenderModule = 0;

  this->PropertyIndex = 0;

  this->Adaptor = vtkSMPropertyAdaptor::New();

  this->NumberOfXFrames = 1;
  this->NumberOfYFrames = 1;
}

//-----------------------------------------------------------------------------
vtkSMComparativeVisProxy::~vtkSMComparativeVisProxy()
{
  this->SetRenderModule(0);
  delete this->Internal;

  this->MultiActorHelper->Delete();
  this->MultiActorHelper = 0;

  this->SetVisName(0);

  this->Adaptor->Delete();
  this->Adaptor = 0;
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::SetConnectionID(vtkIdType id)
{
  this->Superclass::SetConnectionID(id);
  if (this->MultiActorHelper)
    {
    this->MultiActorHelper->SetConnectionID(id);
    }
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::ComputeNumberOfFrames()
{
  this->NumberOfFrames = 1;
  unsigned int numProps = this->Internal->NumberOfFramesInCue.size();
  for (unsigned int i=0; i<numProps; i++)
    {
    this->NumberOfFrames *= this->Internal->NumberOfFramesInCue[i];
    }
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::Generate()
{
  this->RemoveAllCache();

  vtkTimerLog::MarkStartEvent("CV: Play One (all)");
  this->CurrentFrame = 0;
  this->ComputeNumberOfFrames();
  this->PlayOne(this->Internal->Cues.size() - 1);
  vtkTimerLog::MarkEndEvent("CV: Play One (all)");
  this->InFirstShow = 1;
  if (!this->ShouldAbort)
    {
    this->IsGenerated = 1;
    }
  this->ShouldAbort = 0;
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::RemoveAllCache()
{
  this->Internal->Caches.clear();
  this->Internal->Displays.clear();
  this->Internal->Bounds.clear();
  this->IsGenerated = 0;

  this->NumberOfFrames = 0;
  this->CurrentFrame = 0;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::Initialize()
{
  this->Hide();

  this->RemoveAllCues();

  this->RemoveAllCache();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::RemoveAllCues()
{
  this->Internal->Cues.clear();
  this->Internal->NumberOfFramesInCue.clear(); 
  this->Internal->SourceNames.clear();
  this->Internal->SourceTclNames.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::SetNumberOfFramesInCue(
  unsigned int idx, unsigned int numValues)
{
  unsigned int prevSize = this->Internal->NumberOfFramesInCue.size();
  if (idx >= prevSize)
    {
    this->Internal->NumberOfFramesInCue.resize(idx+1);
    for (unsigned int i=prevSize; i<idx; i++)
      {
      this->Internal->NumberOfFramesInCue[i] = 1;
      }
    }
  this->Internal->NumberOfFramesInCue[idx] = numValues;
  this->Modified();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMComparativeVisProxy::GetNumberOfCues()
{
  return this->Internal->Cues.size();
}

//-----------------------------------------------------------------------------
const char* vtkSMComparativeVisProxy::GetSourceName(unsigned int idx)
{
  if (idx >= this->Internal->SourceNames.size())
    {
    return 0;
    }
  return this->Internal->SourceNames[idx].c_str();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::SetSourceName(
  unsigned int idx, const char* name)
{
  if (idx >= this->Internal->SourceNames.size())
    {
    this->Internal->SourceNames.resize(idx+1);
    }

  this->Internal->SourceNames[idx] = name;
  this->Modified();
}

//-----------------------------------------------------------------------------
const char* vtkSMComparativeVisProxy::GetSourceTclName(unsigned int idx)
{
  if (idx >= this->Internal->SourceTclNames.size())
    {
    return 0;
    }
  return this->Internal->SourceTclNames[idx].c_str();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::SetSourceTclName(
  unsigned int idx, const char* name)
{
  if (idx >= this->Internal->SourceTclNames.size())
    {
    this->Internal->SourceTclNames.resize(idx+1);
    }

  this->Internal->SourceTclNames[idx] = name;
  this->Modified();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMComparativeVisProxy::GetNumberOfFramesInCue(
  unsigned int idx)
{
  if (idx >= this->Internal->NumberOfFramesInCue.size())
    {
    return 0;
    }
  return this->Internal->NumberOfFramesInCue[idx];
}

//-----------------------------------------------------------------------------
vtkSMAnimationCueProxy* vtkSMComparativeVisProxy::GetCue(unsigned int idx)
{
  if (idx >= this->GetNumberOfCues())
    {
    return 0;
    }
  return vtkSMAnimationCueProxy::SafeDownCast(this->Internal->Cues[idx]);
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::AddCue(vtkSMProxy* cueProxy)
{
  this->SetCue(this->GetNumberOfCues(), cueProxy);
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::SetCue(unsigned int idx, vtkSMProxy* cueProxy)
{
  if (!cueProxy)
    {
    return;
    }

  if (idx >= this->GetNumberOfCues())
    {
    this->SetNumberOfCues(idx+1);
    }

  this->Internal->Cues[idx] = cueProxy;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::SetNumberOfCues(unsigned int num)
{
  this->Internal->Cues.resize(num);
  this->Modified();
}

//-----------------------------------------------------------------------------
// This function is the main workhorse. It generates a "row" of the comparative
// visualization. A "row" is a collection of frames in which only one parameter
// is varied (played) over it's range while others are kept constant
void vtkSMComparativeVisProxy::PlayOne(unsigned int idx)
{
  vtkTimerLog::MarkStartEvent("CV: Play One");
  if (!this->RenderModule)
    {
    vtkErrorMacro("No RenderModule has been assigned. Cannot generate.");
    return;
    }

  if (idx >= this->Internal->Cues.size())
    {
    return;
    }

  // Define the animation (row).
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
  player->SetEndTime(this->Internal->NumberOfFramesInCue[idx]-1);
  player->SetPlayMode(vtkAnimationScene::PLAYMODE_SEQUENCE);
  player->UpdateVTKObjects();

  this->RenderModule->InvalidateAllGeometries();

  // Run. Note that PlayOne() is recursive and this may lead to other
  // calls of PlayOne() with a different property index.
  player->Play();

  observer->Delete();
  player->Delete();
  vtkTimerLog::MarkEndEvent("CV: Play One");
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::ExecuteEvent(
  vtkObject* , unsigned long event, unsigned int paramIndex)
{
  switch(event)
    {
    case vtkCommand::AnimationCueTickEvent:
      if (this->RenderModule && !this->ShouldAbort)
        {
        // If not the last property, call the next one. For example,
        // if there are two parameters and each have 2 values, the
        // following cases will be generated (in order):
        // 0 0
        // 0 1
        // 1 0
        // 1 1
        if (paramIndex > 0)
          {
          this->PlayOne(paramIndex-1);
          }
//         if (paramIndex < this->Internal->Cues.size() - 1)
//           {
//           this->PlayOne(paramIndex+1);
//           }
        // If last property, render the frame and store the geometry
        else
          {
          vtkTimerLog::MarkStartEvent("CV: Update Displays");
          this->RenderModule->UpdateAllDisplays();
          vtkTimerLog::MarkEndEvent("CV: Update Displays");
          this->StoreGeometry();
          this->UpdateProgress(static_cast<double>(this->CurrentFrame)/
                               this->NumberOfFrames);
          this->CurrentFrame++;
          }
        }
      break;
    }
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::UpdateProgress(double progress)
{
  this->InvokeEvent(vtkCommand::ProgressEvent,(void *)&progress);
}

//-----------------------------------------------------------------------------
// Cache the geometry on the server
void vtkSMComparativeVisProxy::StoreGeometry()
{
  vtkTimerLog::MarkStartEvent("CV: Store Geometry");

  unsigned int prevSize = this->Internal->Caches.size();
  this->Internal->Caches.resize(prevSize+1);
  this->Internal->Displays.resize(prevSize+1);
  this->Internal->Labels.resize(prevSize+1);
  this->Internal->Bounds.resize(prevSize+1);

  vtkSMProxyManager* proxM = vtkSMProxy::GetProxyManager();

  // Initialize bounds for this case. These bounds are
  // later used in Show()
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
  
  // Create the label
  vtkSMProxy* label = proxM->NewProxy("displays", "LabelDisplay");
  label->SetConnectionID(this->ConnectionID);
  label->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->Internal->Labels[prevSize] = label;
  vtkSMStringVectorProperty* text = 
    vtkSMStringVectorProperty::SafeDownCast(
      label->GetProperty("Text"));
  if (text)
    {
    unsigned int numCues = this->GetNumberOfCues();
    ostrstream text_s;
    for (unsigned int i=0; i<numCues; i++)
      {
      vtkSMAnimationCueProxy* cue = this->GetCue(i);
      if (cue && cue->GetAnimatedProperty())
        {
        this->Adaptor->SetProperty(cue->GetAnimatedProperty());
        text_s << cue->GetAnimatedPropertyName() 
               << " = ";
        int animEl = cue->GetAnimatedElement();
        if (animEl < 0)
          {
          unsigned int numEls = this->Adaptor->GetNumberOfRangeElements();
          for (unsigned int j=0; j<numEls; j++)
            {
            const char* value = this->Adaptor->GetRangeValue(j);
            size_t len = strlen(value);
            if (len > 18)
              {
              value = value + len - 18;
              }
            text_s << value;
            if (j < numEls - 1)
              {
              text_s << ",";
              }
            }
          }
        else
          {
          const char* value = 
            this->Adaptor->GetRangeValue(cue->GetAnimatedElement());
          size_t len = strlen(value);
          if (len > 18)
            {
            value = value + len - 18;
            }
          text_s << value;
          }
        if (i != numCues - 1)
          {
          text_s << " , ";
          }
        }
      }
    text_s << ends;
    text->SetElement(0, text_s.str());
    delete[] text_s.str();
    }
  label->UpdateVTKObjects();
  label->Delete();

  vtkCollection* displays = this->RenderModule->GetDisplays();
  vtkCollectionIterator* iter = displays->NewIterator();
  for(iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMDataObjectDisplayProxy* pDisp = 
      vtkSMDataObjectDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp && pDisp->GetVisibilityCM())
      {
      // Geometry cache will make copies (shallow) of all geometry objects
      // and cache them.
      vtkSMProxy* proxy = 
        proxM->NewProxy("ComparativeVisHelpers", "GeometryCache");
      proxy->SetConnectionID(this->ConnectionID);
      vtkSMProxyProperty* prop = 
        vtkSMProxyProperty::SafeDownCast(proxy->GetProperty("AddGeometry"));
      prop->AddProxy(pDisp->GetGeometryFilterProxy());

      proxy->UpdateVTKObjects();
      this->Internal->Caches[prevSize].push_back(proxy);
      proxy->Delete();

      // Create the display and copy setting from original.
      vtkSMProxy* display = proxM->NewProxy("displays", pDisp->GetXMLName());
      display->SetConnectionID(this->ConnectionID);
      if (display)
        {
        vtkSMProxyProperty* input = 
          vtkSMProxyProperty::SafeDownCast(display->GetProperty("Input"));
        input->AddProxy(proxy);
        display->UpdateVTKObjects();
        display->Copy(pDisp, "vtkSMProxyProperty");
        display->GetProperty("LookupTable")->Copy(
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

  vtkTimerLog::MarkEndEvent("CV: Store Geometry");
}

//-----------------------------------------------------------------------------
// Given a 1D index, compute one entry in the nD index vector. For example,
// for 2 parameters with 2 values each:
// 1D idx   param1 idx   param2 idx
// ------  -----------   ----------
//    0          0            0
//    1          0            1
//    2          1            0
//    3          1            1
void vtkSMComparativeVisProxy::ComputeIndex(
  unsigned int paramIdx, unsigned int gidx)
{
  unsigned int numParams = this->Internal->NumberOfFramesInCue.size();
  unsigned int nidx;

  unsigned int prevTotal=0;
  for(unsigned int prevIdx=0; prevIdx<paramIdx; prevIdx++)
    {
    unsigned int product=1;
    for (nidx=prevIdx+1; nidx<numParams; nidx++)
      {
      product *= this->Internal->NumberOfFramesInCue[nidx];
      }
    prevTotal += product*this->Internal->Indices[prevIdx];
    }
  unsigned int offsetIdx = gidx - prevTotal;

  unsigned int product2=1;
  for (nidx=paramIdx+1; nidx<numParams; nidx++)
    {
    product2 *= this->Internal->NumberOfFramesInCue[nidx];
    }
  this->Internal->Indices[paramIdx] = offsetIdx / product2;
  if (paramIdx < numParams-1)
    {
    this->ComputeIndex(paramIdx+1, gidx);
    }
}

//-----------------------------------------------------------------------------
// Compute all nD indices
void vtkSMComparativeVisProxy::ComputeIndices(unsigned int gidx)
{
  unsigned int numParams = this->Internal->NumberOfFramesInCue.size();
  this->Internal->Indices.resize(numParams);
  this->ComputeIndex(0, gidx);
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::AddBounds(double bounds[6], double totalB[6])
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
int vtkSMComparativeVisProxy::Show()
{
  if (!this->RenderModule)
    {
    vtkErrorMacro("RenderModule is not set. Cannot show.");
    return 0;
    }

  //unsigned int numProps = this->Internal->NumberOfFramesInCue.size();
  if (this->Internal->NumberOfFramesInCue.size() < 1 ||
      this->Internal->NumberOfFramesInCue.size() > 2)
    {
    vtkErrorMacro("This visualization only works with <= 2 properties. "
                  "Cannot display");
    return 0;
    }

  // We need to get the size of the render server render window since
  // that is the main display. The aspect ratio on the client might
  // be different and the visualization might not look right there but
  // there isn't much we can do about that.
  vtkSMProxyManager* proxM = vtkSMProxy::GetProxyManager();
  int winSize[2];
  this->RenderModule->GetServerRenderWindowSize(winSize);

  // Compute the collective bounds (of all frames)
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
  int nx = this->NumberOfXFrames;
  int ny = this->NumberOfYFrames;
//   int nx = this->Internal->NumberOfFramesInCue[0];
//   int ny = 1;
//   if (numProps > 1)
//     {
//     ny = this->Internal->NumberOfFramesInCue[1];
//     }
  // Compute a proper cell (frame) spacing based on the data
  // bounds aspect ratio and window aspect ratio.
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
  
  // Use by the interactor
  this->MultiActorHelper->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  vtkSMProxyProperty* actorsP = vtkSMProxyProperty::SafeDownCast(
    this->MultiActorHelper->GetProperty("Actors"));

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->RenderModule->GetProperty("Displays"));

  unsigned int ix=0, iy=0;
  for(i=0; i<numEntries && ix<this->NumberOfXFrames; i++)
    {
    this->ComputeIndices(i);
    vtkSMComparativeVisProxyInternals::ProxiesType::iterator iter2 = 
      this->Internal->Displays[i].begin();
//     double xPos = cellSpacing[0]*this->Internal->Indices[0];
//     double yPos = 0;
//     if (numProps > 1)
//       {
//       yPos = cellSpacing[1]*this->Internal->Indices[1];
//       }
    double xPos = cellSpacing[0]*ix;
    double yPos = cellSpacing[1]*iy;
    vtkSMDisplayProxy* label = vtkSMDisplayProxy::SafeDownCast(
      this->Internal->Labels[i]);
    pp->AddProxy(label);

    for(; iter2 != this->Internal->Displays[i].end(); iter2++)
      {
      vtkSMDataObjectDisplayProxy* display = 
        vtkSMDataObjectDisplayProxy::SafeDownCast(
          iter2->GetPointer());
      pp->AddProxy(display);
      
      // If showing the first time, set the position as well as the
      // clipping planes of actors. Clipping planes simulate multiple
      // renderer.
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
        planeR->SetConnectionID(this->ConnectionID);
        planeR->SetServers(
          vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
        clipPlanes->AddProxy(planeR);
        vtkSMDoubleVectorProperty* porigin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeR->GetProperty("Origin"));
        porigin->SetElements3(
          xPos+bb[1]-cellSpacing[0]*this->BorderWidth, 0, 0);
        vtkSMDoubleVectorProperty* pnormal =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeR->GetProperty("Normal"));
        pnormal->SetElements3(-1, 0, 0);
        planeR->UpdateVTKObjects();
        planeR->Delete();
        
        vtkSMProxy* planeL = 
          proxM->NewProxy("implicit_functions", "Plane");
        planeL->SetConnectionID(this->ConnectionID);
        planeL->SetServers(
          vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
        clipPlanes->AddProxy(planeL);
        porigin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeL->GetProperty("Origin"));
        porigin->SetElements3(
          xPos+bb[0]+cellSpacing[0]*this->BorderWidth, 0, 0);
        pnormal =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeL->GetProperty("Normal"));
        pnormal->SetElements3(1, 0, 0);
        planeL->UpdateVTKObjects();
        planeL->Delete();
        
        vtkSMProxy* planeD = 
          proxM->NewProxy("implicit_functions", "Plane");
        planeD->SetConnectionID(this->ConnectionID);
        planeD->SetServers(
          vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
        clipPlanes->AddProxy(planeD);
        porigin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeD->GetProperty("Origin"));
        porigin->SetElements3(
          0, yPos+bb[2]+cellSpacing[1]*this->BorderWidth, 0);
        pnormal =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeD->GetProperty("Normal"));
        pnormal->SetElements3(0, 1, 0);
        planeD->UpdateVTKObjects();
        planeD->Delete();
        
        vtkSMProxy* planeU = 
          proxM->NewProxy("implicit_functions", "Plane");
        planeU->SetConnectionID(this->ConnectionID);
        planeU->SetServers(
          vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
        clipPlanes->AddProxy(planeU);
        porigin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeU->GetProperty("Origin"));
        porigin->SetElements3(
          0, yPos+bb[3]-cellSpacing[1]*this->BorderWidth, 0);
        pnormal =
          vtkSMDoubleVectorProperty::SafeDownCast(
            planeU->GetProperty("Normal"));
        pnormal->SetElements3(0, -1, 0);
        planeU->UpdateVTKObjects();
        planeU->Delete();

        // Update the label
        vtkSMDoubleVectorProperty* pos = 
          vtkSMDoubleVectorProperty::SafeDownCast(
            label->GetProperty("Position"));
        int delx = winSize[0] / nx;
        int dely = winSize[1] / ny;
        //pos->SetElement(0, this->Internal->Indices[0]*delx);
        pos->SetElement(0, ix*delx);
//         if (numProps > 1)
//           {
//           pos->SetElement(1, this->Internal->Indices[1]*dely);
//           }
//         else
//           {
//           pos->SetElement(1, 0);
//           }
        pos->SetElement(1, iy*dely);
        label->UpdateVTKObjects();
        }
      else
        {
        // It is not enough to reset the positions. All
        // transforms have to be reset
        //vtkSMDoubleVectorProperty* position = 
        //vtkSMDoubleVectorProperty::SafeDownCast(
        //display->GetProperty("Position"));
        //position->SetElements3(xPos, yPos, 0);
        }
      }
    ix++;
    if (ix == this->NumberOfXFrames)
      {
      ix = 0;
      iy++;
      }
    }
  
  this->MultiActorHelper->UpdateVTKObjects();


  // Place and focus the camera
  vtkSMDoubleVectorProperty* position = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->RenderModule->GetProperty("CameraPosition"));
  position->SetElements3(0, 0, 1);

  vtkSMDoubleVectorProperty* focalP = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->RenderModule->GetProperty("CameraFocalPoint"));
  focalP->SetElements3(0, 0, 0);

  vtkSMDoubleVectorProperty* viewUp = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->RenderModule->GetProperty("CameraViewUp"));
  viewUp->SetElements3(0, 1, 0);

  this->RenderModule->UpdateVTKObjects();
  this->RenderModule->ResetCameraClippingRange();

  double totalBounds[6];
  totalBounds[0] = biggestBounds[0];
  totalBounds[1] = totalBounds[0] + cellSpacing[0]*nx;
  totalBounds[2] = biggestBounds[2];
  totalBounds[3] = biggestBounds[2] + cellSpacing[1]*ny;
  totalBounds[4] = biggestBounds[4];
  totalBounds[5] = biggestBounds[5];
  this->RenderModule->ResetCamera(totalBounds);

  // To avoid warping and clipping plane issues, use parallel
  // projection. This is not an optimal solution and should
  // be fixed in long run.
  vtkSMDoubleVectorProperty* parallelScale = 
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->RenderModule->GetProperty("CameraParallelScale"));
  parallelScale->SetElements1((totalBounds[3]-totalBounds[2])/2);
//  parallelScale->SetElements1(1);
  this->RenderModule->UpdateVTKObjects();

  if (this->InFirstShow)
    {
    vtkSMDoubleVectorProperty::SafeDownCast(
      this->MultiActorHelper->GetProperty("UniformScale"))->SetElements1(0.8);
    this->MultiActorHelper->UpdateVTKObjects();
    this->InFirstShow = 0;
    }

  this->RenderModule->StillRender();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::Hide()
{
  if (!this->RenderModule)
    {
    return;
    }

  unsigned int i;

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->RenderModule->GetProperty("Displays"));

  unsigned int numEntries = this->Internal->Displays.size();
  for(i=0; i<numEntries; i++)
    {
    vtkSMComparativeVisProxyInternals::ProxiesType::iterator iter2 = 
      this->Internal->Displays[i].begin();
    for(; iter2 != this->Internal->Displays[i].end(); iter2++)
      {
      vtkSMDataObjectDisplayProxy* display = 
        vtkSMDataObjectDisplayProxy::SafeDownCast(iter2->GetPointer());
      pp->RemoveProxy(vtkSMDisplayProxy::SafeDownCast(display));
      }
    }

  numEntries = this->Internal->Labels.size();
  for(i=0; i<numEntries; i++)
    {
    vtkSMDisplayProxy* label = 
      vtkSMDisplayProxy::SafeDownCast(this->Internal->Labels[i]);
    pp->RemoveProxy(vtkSMDisplayProxy::SafeDownCast(label));
    }
  this->RenderModule->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMComparativeVisProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "IsGenerated: " << this->IsGenerated << endl;
  os << indent << "Name: " << (this->VisName?this->VisName:"(null)") << endl;
  os << indent << "MultiActorHelper: " << this->MultiActorHelper << endl;
  os << indent << "ShouldAbort: " << this->ShouldAbort << endl;
  os << indent << "RenderModule: " << this->RenderModule << endl;
  os << indent << "NumberOfXFrames: " << this->NumberOfXFrames << endl;
  os << indent << "NumberOfYFrames: " << this->NumberOfYFrames << endl;
}
