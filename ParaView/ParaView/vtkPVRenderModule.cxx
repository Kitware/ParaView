/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVRenderModule.h"

#include "vtkCamera.h"
#include "vtkCollectionIterator.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVConfig.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSourceCollection.h"
#include "vtkPVSourceList.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkString.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"

#ifdef _WIN32
#include "vtkWin32OpenGLRenderWindow.h"
#else
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLRenderer.h"
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVRenderModule);
vtkCxxRevisionMacro(vtkPVRenderModule, "1.6");

//int vtkPVRenderModuleCommand(ClientData cd, Tcl_Interp *interp,
//                             int argc, char *argv[]);


//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVRenderModule::vtkPVRenderModule()
{
  this->PVApplication = NULL;
  this->Interactive = 0;
  this->UseReductionFactor = 1;
  
  this->PartDisplays = vtkCollection::New();

  this->Renderer = 0;
  this->RenderWindow = 0;
  this->RendererTclName     = 0;
  this->CompositeTclName    = 0;
  this->RenderWindowTclName = 0;
  this->InteractiveCompositeTime = 0;
  this->InteractiveRenderTime    = 0;
  this->StillRenderTime          = 0;
  this->StillCompositeTime       = 0;
  this->Composite                = 0;

  this->ResetCameraClippingRangeTag = 0;

  this->DisableRenderingFlag = 0;

  this->LODThreshold = 2.0;
  this->LODResolution = 50;
  this->CollectThreshold = 2.0;

  this->RenderInterruptsEnabled = 1;

  this->RenderWindowTclName = NULL;
}

//----------------------------------------------------------------------------
vtkPVRenderModule::~vtkPVRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;

  if (this->Renderer && this->ResetCameraClippingRangeTag > 0)
    {
    this->Renderer->RemoveObserver(this->ResetCameraClippingRangeTag);
    }

  this->PartDisplays->Delete();
  this->PartDisplays = NULL;

  // Tree Composite
  if (this->CompositeTclName && pvApp)
    {
    pvApp->BroadcastScript("%s Delete", this->CompositeTclName);
    this->SetCompositeTclName(NULL);
    this->Composite = NULL;
    }
  else if (this->Composite)
    {
    this->Composite->Delete();
    this->Composite = NULL;
    }
  
  if (this->Renderer)
    {
    if (this->ResetCameraClippingRangeTag > 0)
      {
      this->Renderer->RemoveObserver(this->ResetCameraClippingRangeTag);
      }

    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->RendererTclName);
      }
    else
      {
      this->Renderer->Delete();
      }
    this->SetRendererTclName(NULL);
    this->Renderer = NULL;
    }
  
  if (this->RenderWindow)
    {
    if ( pvApp )
      {
      pvApp->BroadcastScript("%s Delete", this->RenderWindowTclName);
      }
    else
      {
      this->RenderWindow->Delete();
      }
    this->SetRenderWindowTclName(NULL);
    this->RenderWindow = NULL;
    }

  this->SetPVApplication(NULL);
}

//----------------------------------------------------------------------------
// This is a bit of a pain.  I do ResetCameraClippingRange as a call back
// because the PVInteractorStyles call ResetCameraClippingRange 
// directly on the renderer.  Since they are PV styles, I might
// have them call the render module directly like they do for render.
void vtkPVRenderModuleResetCameraClippingRange(
  vtkObject *caller, unsigned long vtkNotUsed(event),void *clientData, void *)
{
  float bds[6];

  vtkPVRenderModule *self = (vtkPVRenderModule *)clientData;
  vtkRenderer *ren = (vtkRenderer*)caller;

  self->ComputeVisiblePropBounds(bds);
  ren->ResetCameraClippingRange(bds);
}

//----------------------------------------------------------------------------
void PVRenderModuleAbortCheck(vtkObject*, unsigned long, void* arg, void*)
{
  vtkPVRenderModule *me = (vtkPVRenderModule*)arg;

  if (me->GetRenderInterruptsEnabled())
    {
    // Just forward the event along.
    me->InvokeEvent(vtkCommand::AbortCheckEvent, NULL);  
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetPVApplication(vtkPVApplication *pvApp)
{
  if (this->PVApplication)
    {
    if (pvApp == NULL)
      {
      this->PVApplication->UnRegister(this);
      this->PVApplication = NULL;
      return;
      }
    vtkErrorMacro("PVApplication already set.");
    return;
    }
  if (pvApp == NULL)
    {
    return;
    }  

  // Maybe I should not reference count this object to avoid
  // a circular reference.
  this->PVApplication = pvApp;
  this->PVApplication->Register(this);

  this->Renderer = (vtkRenderer*)pvApp->MakeTclObject("vtkRenderer", "Ren1");
  this->RendererTclName = NULL;
  this->SetRendererTclName("Ren1");
  
  this->RenderWindow = 
    (vtkRenderWindow*)pvApp->MakeTclObject("vtkRenderWindow", "RenWin1");

  this->SetRenderWindowTclName("RenWin1");
  
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pvApp->GetProcessModule()->GetNumberOfPartitions() > 1))
    {
    pvApp->BroadcastScript("%s SetMultiSamples 0", this->RenderWindowTclName);
    }

  if (pvApp->GetUseTiledDisplay())
    {
    // Thr original tiled display with duplicate polydata.
    //this->Composite = NULL;
    //pvApp->MakeTclObject("vtkTiledDisplayManager", "TDispManager1");
    //int *tileDim = pvApp->GetTileDimensions();
    //pvApp->BroadcastScript("TDispManager1 SetTileDimensions %d %d",
    //                       tileDim[0], tileDim[1]);
    //this->CompositeTclName = NULL;
    //this->SetCompositeTclName("TDispManager1");

    this->Composite = NULL;
    pvApp->MakeTclObject("vtkPVTiledDisplayManager", "TDispManager1");
    int *tileDim = pvApp->GetTileDimensions();
    pvApp->BroadcastScript("TDispManager1 SetTileDimensions %d %d",
                           tileDim[0], tileDim[1]);
    pvApp->BroadcastScript("TDispManager1 InitializeSchedule");

    this->CompositeTclName = NULL;
    this->SetCompositeTclName("TDispManager1");
    }
  else if (pvApp->GetClientMode() || pvApp->GetServerMode())
    {
    this->Composite = NULL;
    pvApp->MakeTclObject("vtkClientCompositeManager", "CCompositeManager1");
    // Clean up this mess !!!!!!!!!!!!!
    // Even a cast to vtkPVClientServerModule would be better than this.
    // How can we syncronize the process modules and render modules?
    pvApp->BroadcastScript("CCompositeManager1 SetClientController [[$Application GetProcessModule] GetSocketController]");
    pvApp->BroadcastScript("CCompositeManager1 SetClientFlag [$Application GetClientMode]");

    this->CompositeTclName = NULL;
    this->SetCompositeTclName("CCompositeManager1");    
    }
  else
    {
    // Create the compositer.
    this->Composite = static_cast<vtkPVTreeComposite*>
      (pvApp->MakeTclObject("vtkPVTreeComposite", "TreeComp1"));

    //this->Composite->RemoveObservers(vtkCommand::AbortCheckEvent);
    vtkCallbackCommand* abc = vtkCallbackCommand::New();
    abc->SetCallback(PVRenderModuleAbortCheck);
    abc->SetClientData(this);
    this->Composite->AddObserver(vtkCommand::AbortCheckEvent, abc);
    abc->Delete();

    // Try using a more efficient compositer (if it exists).
    // This should be a part of a module.
    pvApp->BroadcastScript("if {[catch {vtkCompressCompositer pvTmp}] == 0} "
                           "{TreeComp1 SetCompositer pvTmp; pvTmp Delete}");

    this->CompositeTclName = NULL;
    this->SetCompositeTclName("TreeComp1");

    // If we are using SGI pipes, create a new Controller/Communicator/Group
    // to use for compositing.
    if (pvApp->GetUseRenderingGroup())
      {
      int numPipes = pvApp->GetNumberOfPipes();
      // I would like to create another controller with a subset of world, but...
      // For now, I added it as a hack to the composite manager.
      pvApp->BroadcastScript("%s SetNumberOfProcesses %d",
                             this->CompositeTclName, numPipes);
      }
    }
  pvApp->BroadcastScript("%s AddRenderer %s", this->RenderWindowTclName,
                           this->RendererTclName);
  
  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
                         this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);


  if ( getenv("PV_DISABLE_COMPOSITE_INTERRUPTS") )
    {
    pvApp->BroadcastScript("%s EnableAbortOff", this->CompositeTclName);
    }

  if ( getenv("PV_OFFSCREEN") )
    {
    pvApp->BroadcastScript("%s InitializeOffScreen", this->CompositeTclName);
    }


  // Make sure we have a chance to set the clipping range properly.
  vtkCallbackCommand* cbc;
  cbc = vtkCallbackCommand::New();
  cbc->SetCallback(vtkPVRenderModuleResetCameraClippingRange);
  cbc->SetClientData((void*)this);
  // ren will delete the cbc when the observer is removed.
  this->ResetCameraClippingRangeTag = 
    this->Renderer->AddObserver(vtkCommand::ResetCameraClippingRangeEvent,cbc);
  cbc->Delete();
}


//----------------------------------------------------------------------------
vtkRenderer *vtkPVRenderModule::GetRenderer()
{
  return this->Renderer;
}

//----------------------------------------------------------------------------
vtkRenderWindow *vtkPVRenderModule::GetRenderWindow()
{
  return this->RenderWindow;
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetBackgroundColor(float r, float g, float b)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  pvApp->BroadcastScript("%s SetBackground %f %f %f",
                         this->RendererTclName, r, g, b);
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::ComputeVisiblePropBounds(float bds[6])
{
  double* tmp;
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVPart* part;

  // Compute the bounds for our sources.
  bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
  bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    part = pDisp->GetPart();
    if (part && pDisp->GetVisibility())
      {
      tmp = part->GetDataInformation()->GetBounds();
      if (tmp[0] < bds[0]) { bds[0] = tmp[0]; }  
      if (tmp[1] > bds[1]) { bds[1] = tmp[1]; }  
      if (tmp[2] < bds[2]) { bds[2] = tmp[2]; }  
      if (tmp[3] > bds[3]) { bds[3] = tmp[3]; }  
      if (tmp[4] < bds[4]) { bds[4] = tmp[4]; }  
      if (tmp[5] > bds[5]) { bds[5] = tmp[5]; }  
      }
    }

  if ( bds[0] > bds[1])
    {
    bds[0] = bds[2] = bds[4] = -1.0;
    bds[1] = bds[3] = bds[5] = 1.0;
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::AddPVSource(vtkPVSource *pvs)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  vtkPVPartDisplay *pDisp;
  int num, idx;
  
  if (pvs == NULL)
    {
    return;
    }  
  
  // I would like to move the addition of the prop into vtkPVPart sometime.
  num = pvs->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = pvs->GetPart(idx);
    // Create a part display for each part.
    pDisp = vtkPVPartDisplay::New();
    this->PartDisplays->AddItem(pDisp);
    pDisp->SetPVApplication(pvApp);
    part->SetPartDisplay(pDisp);
    pDisp->SetPart(part);

    if (part && pDisp->GetPropTclName() != NULL)
      {
      pvApp->BroadcastScript("%s AddProp %s", this->RendererTclName,
                             pDisp->GetPropTclName());
      }
    pDisp->Delete();
    }

  // I would like to find a new place for this initialization.
  // The data is not up to date anyway.  Maybe in Update ... !!!!
  // I would like to gather information for part displays separately.
  for (idx = 0; idx < num; ++idx)
    {
    part = pvs->GetPart(idx);
    // Create a part display for each part.
    pDisp = part->GetPartDisplay();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::RemovePVSource(vtkPVSource *pvs)
{
  int idx, num;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVPart *part;
  vtkPVPartDisplay *pDisp;

  if (pvs == NULL)
    {
    return;
    }

  num = pvs->GetNumberOfParts();
  for (idx = 0; idx < num; ++idx)
    {
    part = pvs->GetPart(idx);
    pDisp = part->GetPartDisplay();
    if (pDisp)
      {
      this->PartDisplays->RemoveItem(pDisp);
      if (pDisp->GetPropTclName() != NULL)
        {
        pvApp->BroadcastScript("%s RemoveProp %s", this->RendererTclName,
                               pDisp->GetPropTclName());
        }
      part->SetPartDisplay(NULL);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::StartRender()
{
  float renderTime = 1.0 / this->RenderWindow->GetDesiredUpdateRate();
  int *windowSize = this->RenderWindow->GetSize();
  int area, reducedArea, reductionFactor;
  float timePerPixel;
  float getBuffersTime, setBuffersTime, transmitTime;
  float newReductionFactor;
  float maxReductionFactor;
  
  // Tiled displays do not use pixel reduction LOD.
  // This is not necessary because to caller already checks,
  // but it clarifies the situation.
  if (this->Composite == NULL)
    {
    return;
    }

  if (!this->UseReductionFactor)
    {
    this->Composite->SetReductionFactor(1);
    return;
    }
  
  // Do not let the width go below 150.
  maxReductionFactor = windowSize[0] / 150.0;

  renderTime *= 0.5;
  area = windowSize[0] * windowSize[1];
  reductionFactor = this->Composite->GetReductionFactor();
  reducedArea = area / (reductionFactor * reductionFactor);
  getBuffersTime = this->Composite->GetGetBuffersTime();
  setBuffersTime = this->Composite->GetSetBuffersTime();
  transmitTime = this->Composite->GetCompositeTime();

  // Do not consider SetBufferTime because 
  //it is not dependent on reduction factor.,
  timePerPixel = (getBuffersTime + transmitTime) / reducedArea;
  newReductionFactor = sqrt(area * timePerPixel / renderTime);
  
  if (newReductionFactor > maxReductionFactor)
    {
    newReductionFactor = maxReductionFactor;
    }
  if (newReductionFactor < 1.0)
    {
    newReductionFactor = 1.0;
    }

  //cerr << "---------------------------------------------------------\n";
  //cerr << "New ReductionFactor: " << newReductionFactor << ", oldFact: " 
  //     << reductionFactor << endl;
  //cerr << "Alloc.Comp.Time: " << renderTime << ", area: " << area 
  //     << ", pixelTime: " << timePerPixel << endl;
  //cerr << "GetBufTime: " << getBuffersTime << ", SetBufTime: " << setBuffersTime
  //     << ", transTime: " << transmitTime << endl;
  
  this->Composite->SetReductionFactor((int)newReductionFactor);
}

int vtkPVRenderModule::UpdateAllPVData(int interactive)
{
  vtkPVWindow* pvwindow = this->GetPVWindow();
  vtkPVApplication *pvApp = this->GetPVApplication();
  if ( !pvwindow || !pvApp )
    {
    return 0;
    }
  vtkPVSourceCollection* col = 0;
  //cout << "Update all PVData" << endl;
  col = pvwindow->GetSourceList("Sources");
  if ( col )
    {
    vtkCollectionIterator *it = col->NewIterator();
    it->InitTraversal();
    vtkPVSource* source = 0;
    while ( !it->IsDoneWithTraversal() )
      {
      source = static_cast<vtkPVSource*>(it->GetObject());
      if ( source->GetInitialized() && source->GetVisibility() )
        {
        source->ForceUpdate(pvApp);
        }
      it->GoToNextItem();
      }
    it->Delete();
    }


  // We need to decide globally whether to use decimated geometry.  
  if (interactive && 
        this->GetPVWindow()->GetTotalVisibleGeometryMemorySize() > 
        this->LODThreshold*1000)
    {
    pvApp->SetGlobalLODFlag(1);
    return this->GetPVWindow()->MakeLODCollectionDecision();
    }
  pvApp->SetGlobalLODFlag(0);
  return this->GetPVWindow()->MakeCollectionDecision();
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::InteractiveRender()
{
  int renderLocally = this->UpdateAllPVData(1);

  //this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
  this->RenderWindow->SetDesiredUpdateRate(5.0);

  // Used to have abort check here. (temporary comment in case soemthing goes wrong.)

  // The composite is not set for powerwall or client server.
  // Only the CompositeTclName is set.  I should change this
  // or eliminate the Composite pointer.
  if (this->CompositeTclName)
    {
    if (renderLocally)
      {
      this->PVApplication->Script("%s UseCompositingOff", this->CompositeTclName);
      }
    else
      {
      this->PVApplication->Script("%s UseCompositingOn", this->CompositeTclName);
      }
    }

  if (this->Composite && ! renderLocally)
    { // just set up the reduction factor
    this->StartRender();
    }

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");

  if (this->Composite)
    {
    this->InteractiveRenderTime = this->Composite->GetMaxRenderTime();
    this->InteractiveCompositeTime = this->Composite->GetCompositeTime()
      + this->Composite->GetGetBuffersTime()
      + this->Composite->GetSetBuffersTime();
    }
}




//----------------------------------------------------------------------------
void vtkPVRenderModule::StillRender()
{
  int renderLocally;

  vtkPVApplication *pvApp = this->GetPVApplication();
  renderLocally = this->UpdateAllPVData(0);

  // Tell composite whether to render locally  or composite.
  // The composite is not set for powerwall or client server.
  // Only the CompositeTclName is set.  I should change this
  // or eliminate the Composite pointer.
  if (this->CompositeTclName)
    {
    if (renderLocally)
      {
      this->PVApplication->Script("%s UseCompositingOff", this->CompositeTclName);
      }
    else
      {
      this->PVApplication->Script("%s UseCompositingOn", this->CompositeTclName);
      }
    }

  if (this->GetPVWindow()->GetInteractor())
    {
    this->RenderWindow->SetDesiredUpdateRate(
      this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());
    }

  // Used to have abort check here. (temporary comment in case soemthing goes wrong.)
  if (this->Composite)
    {
    this->StartRender();
    }

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  pvApp->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");

  if (this->Composite)
    {
    this->StillRenderTime = this->Composite->GetMaxRenderTime();
    this->StillCompositeTime = this->Composite->GetCompositeTime()
      + this->Composite->GetGetBuffersTime()
      + this->Composite->GetSetBuffersTime();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseTriangleStrips(int val)
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp;
  int numParts, partIdx;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    numParts = pvs->GetNumberOfParts();
    for (partIdx = 0; partIdx < numParts; ++partIdx)
      {
      pvApp->BroadcastScript("%s SetUseStrips %d",
                             pvs->GetPart(partIdx)->GetGeometryTclName(),
                             val);
      }
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable triangle strips.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable triangle strips.");
    }

}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseParallelProjection(int val)
{
  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOn();
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable parallel projection.");
    this->Renderer->GetActiveCamera()->ParallelProjectionOff();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseImmediateMode(int val)
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp;
  int partIdx, numParts;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    numParts = pvs->GetNumberOfParts();
    for (partIdx = 0; partIdx < numParts; ++partIdx)
      {
      pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                             pvs->GetPart(partIdx)->GetPartDisplay()->GetMapperTclName(),
                             val);
      pvApp->BroadcastScript("%s SetImmediateModeRendering %d",
                             pvs->GetPart(partIdx)->GetPartDisplay()->GetLODMapperTclName(),
                             val);
      }
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Disable display lists.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Enable display lists.");
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetLODThreshold(float threshold)
{
  this->LODThreshold = threshold;
}




//----------------------------------------------------------------------------
void vtkPVRenderModule::SetLODResolution(int resolution)
{
  vtkPVWindow *pvWin;
  vtkPVSourceCollection *sources;
  vtkPVSource *pvs;
  vtkPVApplication *pvApp;
  int idx, num;
  vtkPVPart *part;

  this->LODResolution = resolution;

  pvApp = this->GetPVApplication();
  pvWin = this->GetPVWindow();
  if (pvWin == NULL)
    {
    vtkErrorMacro("Missing window.");
    return;
    }
  sources = pvWin->GetSourceList("Sources");
  sources->InitTraversal();
  while ( (pvs = sources->GetNextPVSource()) )
    {
    num = pvs->GetNumberOfParts();
    for (idx = 0; idx < num; ++idx)
      {
      part = pvs->GetPart(idx);
      part->GetPartDisplay()->SetLODResolution(resolution);
      }
    } 
}





//----------------------------------------------------------------------------
void vtkPVRenderModule::SetCollectThreshold(float threshold)
{
  vtkPVApplication *pvApp;

  this->CollectThreshold = threshold;

  // This will cause collection to be re evaluated.
  pvApp = this->GetPVApplication();
  pvApp->SetTotalVisibleMemorySizeValid(0);
}




//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseCompositeWithFloat(int val)
{
  if (this->Composite)
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseChar %d",
                                              this->CompositeTclName,
                                              !val);
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as floats.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Get color buffers as unsigned char.");
    }

}

//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseCompositeWithRGBA(int val)
{
  if (this->Composite)
    {
    this->GetPVApplication()->BroadcastScript("%s SetUseRGB %d",
                                              this->CompositeTclName,
                                              !val);
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Use RGBA pixels to get color buffers.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Use RGB pixels to get color buffers.");
    }
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::SetUseCompositeCompression(int val)
{
  if (this->Composite)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    if (val)
      {
      pvApp->BroadcastScript("vtkCompressCompositer pvTemp");
      }
    else
      {
      pvApp->BroadcastScript("vtkTreeCompositer pvTemp");
      }
    pvApp->BroadcastScript("%s SetCompositer pvTemp", this->CompositeTclName);
    pvApp->BroadcastScript("pvTemp Delete");
    }

  if (val)
    {
    vtkTimerLog::MarkEvent("--- Enable compression when compositing.");
    }
  else
    {
    vtkTimerLog::MarkEvent("--- Disable compression when compositing.");
    }
}


//----------------------------------------------------------------------------
vtkPVWindow *vtkPVRenderModule::GetPVWindow()
{
  return this->GetPVApplication()->GetMainWindow();
}


//----------------------------------------------------------------------------
int* vtkPVRenderModule::GetRenderWindowSize()
{
  if ( this->GetRenderWindow() )
    {
    return this->GetRenderWindow()->GetSize();
    }
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InteractiveCompositeTime: " 
     << this->GetInteractiveCompositeTime() << endl;
  os << indent << "InteractiveRenderTime: " 
     << this->GetInteractiveRenderTime() << endl;
  os << indent << "RendererTclName: " 
     << (this->GetRendererTclName()?this->GetRendererTclName():"<none>") << endl;
  os << indent << "StillCompositeTime: " 
     << this->GetStillCompositeTime() << endl;
  os << indent << "StillRenderTime: " << this->GetStillRenderTime() << endl;
  os << indent << "DisableRenderingFlag: " 
     << (this->DisableRenderingFlag ? "on" : "off") << endl;
  os << indent << "RenderInterruptsEnabled: " 
     << (this->RenderInterruptsEnabled ? "on" : "off") << endl;

  os << indent << "UseReductionFactor: " << this->UseReductionFactor << endl;
  if (this->CompositeTclName)
    {
    os << indent << "CompositeTclName: " << this->CompositeTclName << endl;
    }
  if (this->PVApplication)
    {
    os << indent << "PVApplication: " << this->PVApplication << endl;
    }
  else
    {
    os << indent << "PVApplication: NULL" << endl;
    }
  //os << indent << "LODThreshold: " << this->LODThreshold << endl;
  //os << indent << "LODResolution: " << this->LODResolution << endl;
  //os << indent << "CollectThreshold: " << this->CollectThreshold << endl;
}

