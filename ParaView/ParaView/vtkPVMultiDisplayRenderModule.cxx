/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModule.cxx
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
#include "vtkPVMultiDisplayRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVMultiDisplayPartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayRenderModule);
vtkCxxRevisionMacro(vtkPVMultiDisplayRenderModule, "1.5.2.5");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModule::vtkPVMultiDisplayRenderModule()
{
}

//----------------------------------------------------------------------------
vtkPVMultiDisplayRenderModule::~vtkPVMultiDisplayRenderModule()
{

}


//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::SetPVApplication(vtkPVApplication *pvApp)
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

  this->Composite = NULL;
  pvApp->MakeTclObject("vtkMultiDisplayManager", "TDispManager1");
  int *tileDim = pvApp->GetTileDimensions();
  pvApp->BroadcastScript("TDispManager1 SetTileDimensions %d %d",
                         tileDim[0], tileDim[1]);

  if (pvApp->GetClientMode())
    {
    pvApp->Script("TDispManager1 SetClientFlag 1");
    pvApp->BroadcastScript("TDispManager1 SetSocketController [[$Application GetProcessModule] GetSocketController]");
    pvApp->BroadcastScript("TDispManager1 SetZeroEmpty 0");
    }
  else
    {
    pvApp->BroadcastScript("TDispManager1 SetZeroEmpty 1");
    }
  // Have to initialize after ZeroEmpty, and tile dimensions have been set.
  pvApp->BroadcastScript("TDispManager1 InitializeSchedule");


  this->CompositeTclName = NULL;
  this->SetCompositeTclName("TDispManager1");

  pvApp->BroadcastScript("%s SetRenderWindow %s", this->CompositeTclName,
                         this->RenderWindowTclName);
  pvApp->BroadcastScript("%s InitializeRMIs", this->CompositeTclName);

}

//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVMultiDisplayRenderModule::CreatePartDisplay()
{
  return vtkPVMultiDisplayPartDisplay::New();
}


//----------------------------------------------------------------------------
// This is almost an exact duplicate of the superclass.
// Think about reorganizing methods to reduce code duplication.
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::StillRender()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;
  unsigned long totalMemory = 0;
  int localRender;

  // Find out whether we are going to render localy.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      // This updates if required (collection disabled).
      info = pDisp->GetInformation();
      totalMemory += info->GetGeometryMemorySize();
      }
    }
  localRender = 0;
  if ((float)(totalMemory)/1000.0 < this->GetCompositeThreshold())
    {
    localRender = 1;
    }
  // Change the collection flags and update.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      pDisp->SetCollectionDecision(localRender);
      pDisp->Update();
      }
    }

  if (this->PVApplication && this->CompositeTclName)
    {
    this->PVApplication->Script("%s SetReductionFactor 1",
                                this->CompositeTclName);
    }

  // Switch the compositer to local/composite mode.
  if (this->LocalRender != localRender)
    {
    if (this->CompositeTclName)
      {
      if (localRender)
        {
        this->PVApplication->Script("%s UseCompositingOff", this->CompositeTclName);
        }
      else
        {
        this->PVApplication->Script("%s UseCompositingOn", this->CompositeTclName);
        }
      this->LocalRender = localRender;
      }
    }


  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->RenderWindow->SetDesiredUpdateRate(0.002);
  // this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

  this->GetPVApplication()->SetGlobalLODFlag(0);

  // This is the only thing I believe makes a difference
  // for this sublcass !!!!!!
  if ( ! localRender)
    {
    // A bit of a hack to have client use LOD 
    // which may be different than satellites.
    this->GetPVApplication()->SetGlobalLODFlagInternal(1);
    }

  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");

  // If we do not restore the consistent original value,
  // it will get stuck in high res on satellites.
  if ( ! localRender)
    {
    this->GetPVApplication()->SetGlobalLODFlagInternal(0);
    }
}


//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::InteractiveRender()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;
  unsigned long totalGeoMemory = 0;
  unsigned long totalLODMemory = 0;
  unsigned long tmpMemory;
  int localRender;
  int useLOD;

  // Compute memory totals.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      // This updates if required (collection disabled).
      info = pDisp->GetInformation();
      totalGeoMemory += info->GetGeometryMemorySize();
      totalLODMemory += info->GetLODGeometryMemorySize();
      }
    }

  // Make LOD decision.
  if ((float)(totalGeoMemory)/1000.0 < this->GetLODThreshold())
    {
    useLOD = 0;
    tmpMemory = totalGeoMemory;
    this->GetPVApplication()->SetGlobalLODFlag(0);
    }
  else
    {
    useLOD = 1;
    tmpMemory = totalLODMemory;
    this->GetPVApplication()->SetGlobalLODFlag(1);
    }

  // MakeCollection Decision.
  localRender = 0;
  if ((float)(tmpMemory)/1000.0 < this->GetCompositeThreshold())
    {
    localRender = 1;
    }
  if (useLOD)
    {
    localRender = 1;
    }

  // Change the collection flags and update.
  this->PartDisplays->InitTraversal();
  while ( (object = this->PartDisplays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp->GetVisibility())
      {
      // I had to add this because the default for the
      // Collection filter is not to collect 
      // (in case of large data).  Accept was not performing
      // the collection decision logic so was collecting ...
      pDisp->SetLODCollectionDecision(1);
      // What we are really doing here is a little opaque.
      // First setting the part display's LODCollectionDescision
      // does nothing (the value is ignored).  The LOD
      // always collects.
      // The only special case we have to consider is 
      // rendering full res model but not collecting.
      // In this condition, the client renders the collected LOD,
      // but the satellites render their parition of the full res.
      if ( ! useLOD)
        {
        pDisp->SetCollectionDecision(localRender);
        }
      pDisp->Update();
      }
    }

  // Switch the compositer to local/composite mode.
  if (this->LocalRender != localRender)
    {
    if (this->CompositeTclName)
      {
      if (localRender)
        {
        this->PVApplication->Script("%s UseCompositingOff", this->CompositeTclName);
        }
      else
        {
        this->PVApplication->Script("%s UseCompositingOn", this->CompositeTclName);
        }
      this->LocalRender = localRender;
      }
    }

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  // This might be used for Reduction factor.
  this->RenderWindow->SetDesiredUpdateRate(5.0);
  // this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

  // Compute reduction factor. 
  if (! localRender)
    {
    this->ComputeReductionFactor();
    }

  // This is the only thing I believe makes a difference
  // for this sublcass !!!!!!
  if ( ! localRender)
    {
    // A bit of a hack to have client use LOD 
    // which may be different than satellites.
    this->GetPVApplication()->SetGlobalLODFlagInternal(1);
    }

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");

  // If we do not restore the consistent original value,
  // it will get stuck in high res on satellites.
  if ( ! localRender)
    {
    this->GetPVApplication()->SetGlobalLODFlagInternal(useLOD);
    }


}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::SetUseCompositeCompression(int val)
{
  if (this->CompositeTclName)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    // !!!! Putting this catch here until I make a native TD sublcass. !!!
    pvApp->BroadcastScript("catch {%s SetUseCompositeCompression %d}", 
                            this->CompositeTclName, val);
    }
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

