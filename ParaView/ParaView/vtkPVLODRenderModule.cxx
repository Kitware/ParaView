/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLODRenderModule.cxx
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
#include "vtkPVLODRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVPartDisplay.h"
#include "vtkPVPart.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLODRenderModule);
vtkCxxRevisionMacro(vtkPVLODRenderModule, "1.2");

//int vtkPVLODRenderModuleCommand(ClientData cd, Tcl_Interp *interp,
//                             int argc, char *argv[]);


//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVLODRenderModule::vtkPVLODRenderModule()
{
  this->LODThreshold = 2.0;
  this->LODResolution = 50;
  this->CollectThreshold = 2.0;

  this->Composite                = 0;
  this->CompositeTclName    = 0;
  this->InteractiveCompositeTime = 0;
  this->StillCompositeTime       = 0;

  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODMemorySize = 0;
  this->CollectionDecision = 1;
  this->LODCollectionDecision = 1;

  this->UseReductionFactor = 1;
  this->AbortCheckTag = 0;

}

//----------------------------------------------------------------------------
vtkPVLODRenderModule::~vtkPVLODRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;

  if (this->Composite && this->AbortCheckTag)
    {
    this->Composite->RemoveObserver(this->AbortCheckTag);
    this->AbortCheckTag = 0;
    }
 
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
}

//----------------------------------------------------------------------------
void PVLODRenderModuleAbortCheck(vtkObject*, unsigned long, void* arg, void*)
{
  vtkPVRenderModule *me = (vtkPVRenderModule*)arg;

  if (me->GetRenderInterruptsEnabled())
    {
    // Just forward the event along.
    me->InvokeEvent(vtkCommand::AbortCheckEvent, NULL);  
    }
}



//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetPVApplication(vtkPVApplication *pvApp)
{
  this->Superclass::SetPVApplication(pvApp);
  if (pvApp == NULL)
    {
    return;
    }

  // We had trouble with SGI/aliasing with compositing.
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
    abc->SetCallback(PVLODRenderModuleAbortCheck);
    abc->SetClientData(this);
    this->AbortCheckTag = 
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
}


//----------------------------------------------------------------------------
// This is used before a render to make sure all visible sources
// have been updated.  It returns 1 if all the data has been collected
// and the render should be local. A side action is to set the Global LOD
// Flag.  This is what the argument is used for.  I do not think this is 
// best place to do this ...
int vtkPVLODRenderModule::UpdateAllPVData(int interactive)
{
  vtkPVApplication* pvApp = this->GetPVApplication();

  this->Superclass::UpdateAllPVData(interactive);

  // We need to decide globally whether to use decimated geometry.  
  if (interactive && 
      this->GetTotalVisibleGeometryMemorySize() > 
      this->LODThreshold*1000)
    {
    pvApp->SetGlobalLODFlag(1);
    return this->MakeLODCollectionDecision();
    }
  pvApp->SetGlobalLODFlag(0);
  return this->MakeCollectionDecision();
}



//----------------------------------------------------------------------------
void vtkPVLODRenderModule::StillRender()
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

  this->RenderWindow->SetDesiredUpdateRate(0.002);
  //  this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

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
void vtkPVLODRenderModule::InteractiveRender()
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
void vtkPVLODRenderModule::StartRender()
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
  
  this->Composite->SetReductionFactor((int)newReductionFactor);
}


//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetCollectThreshold(float threshold)
{
  vtkPVApplication *pvApp;

  this->CollectThreshold = threshold;

  // This will cause collection to be re evaluated.
  pvApp = this->GetPVApplication();
  pvApp->SetTotalVisibleMemorySizeValid(0);
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetUseCompositeWithFloat(int val)
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
void vtkPVLODRenderModule::SetUseCompositeWithRGBA(int val)
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
void vtkPVLODRenderModule::SetUseCompositeCompression(int val)
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
void vtkPVLODRenderModule::SetLODThreshold(float threshold)
{
  this->LODThreshold = threshold;
}

//----------------------------------------------------------------------------
void vtkPVLODRenderModule::SetLODResolution(int resolution)
{
  vtkObject* object;
  vtkPVPartDisplay* partDisp;

  this->LODResolution = resolution;

  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    partDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (partDisp)
      {
      partDisp->SetLODResolution(resolution);
      }
    } 
}

//-----------------------------------------------------------------------------
int vtkPVLODRenderModule::MakeCollectionDecision()
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVApplication* pvApp = this->GetPVApplication();
  int decision = 1;

  // Do I really need to store the TotalVisibleMemorySIze in the application???
  if (pvApp->GetTotalVisibleMemorySizeValid())
    {
    return this->CollectionDecision;
    }

  this->ComputeTotalVisibleMemorySize();
  pvApp->SetTotalVisibleMemorySizeValid(1);

  if (this->TotalVisibleGeometryMemorySize > 
      this->GetCollectThreshold()*1000)
    {
    decision = 0;
    }

  if (decision == this->CollectionDecision)
    {
    return decision;
    }
  this->CollectionDecision = decision;
    
  this->PartDisplays->InitTraversal();
  while ( (object=this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetCollectionDecision(this->CollectionDecision);
      }
    }

  return this->CollectionDecision;
}


//-----------------------------------------------------------------------------
int vtkPVLODRenderModule::MakeLODCollectionDecision()
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVApplication* pvApp = this->GetPVApplication();
  int decision = 1;

  if (pvApp->GetTotalVisibleMemorySizeValid())
    {
    return this->LODCollectionDecision;
    }

  this->ComputeTotalVisibleMemorySize();
  pvApp->SetTotalVisibleMemorySizeValid(1);
  if (this->TotalVisibleLODMemorySize > 
      this->GetCollectThreshold()*1000)
    {
    decision = 0;
    }

  if (decision == this->LODCollectionDecision)
    {
    return decision;
    }
  this->LODCollectionDecision = decision;
    
  this->PartDisplays->InitTraversal();
  while ( (object=this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetLODCollectionDecision(this->LODCollectionDecision);
      }
    }

  return this->LODCollectionDecision;
}



//-----------------------------------------------------------------------------
void vtkPVLODRenderModule::ComputeTotalVisibleMemorySize()
{
  vtkObject* object;
  vtkPVPartDisplay* pDisp;
  vtkPVPart* part;
  vtkPVDataInformation* pvdi;

  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODMemorySize = 0;
  this->PartDisplays->InitTraversal();
  while ( (object=this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVPartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility() && pDisp->GetPart())
      {
      part = pDisp->GetPart();
      pvdi = part->GetDataInformation();
      this->TotalVisibleGeometryMemorySize += pvdi->GetGeometryMemorySize();
      this->TotalVisibleLODMemorySize += pvdi->GetLODMemorySize();
      }
    }

}


//-----------------------------------------------------------------------------
unsigned long vtkPVLODRenderModule::GetTotalVisibleGeometryMemorySize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp->GetTotalVisibleMemorySizeValid())
    {
    return this->TotalVisibleGeometryMemorySize;
    }

  this->ComputeTotalVisibleMemorySize();
  return this->TotalVisibleGeometryMemorySize;
}




//----------------------------------------------------------------------------
void vtkPVLODRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LODThreshold: " << this->LODThreshold << endl;
  os << indent << "LODResolution: " << this->LODResolution << endl;
  os << indent << "CollectThreshold: " << this->CollectThreshold << endl;

  os << indent << "UseReductionFactor: " << this->UseReductionFactor << endl;
  if (this->CompositeTclName)
    {
    os << indent << "CompositeTclName: " << this->CompositeTclName << endl;
    }

  os << indent << "InteractiveCompositeTime: " 
     << this->GetInteractiveCompositeTime() << endl;
  os << indent << "StillCompositeTime: " 
     << this->GetStillCompositeTime() << endl;

}

