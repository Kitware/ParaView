/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModule.cxx
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
#include "vtkPVCompositeRenderModule.h"
#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkPVTreeComposite.h"
#include "vtkPVApplication.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPVProcessModule.h"
#include "vtkCallbackCommand.h"
#include "vtkPVCompositePartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositeRenderModule);
vtkCxxRevisionMacro(vtkPVCompositeRenderModule, "1.7.2.4");



//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVCompositeRenderModule::vtkPVCompositeRenderModule()
{
  this->LocalRender = 1;
  this->CompositeThreshold = 20.0;

  this->Composite                = 0;
  this->CompositeID.ID    = 0;
  this->InteractiveCompositeTime = 0;
  this->StillCompositeTime       = 0;

  this->CollectionDecision = -1;
  this->LODCollectionDecision = -1;

  this->ReductionFactor = 2;
  this->SquirtLevel = 0;
}

//----------------------------------------------------------------------------
vtkPVCompositeRenderModule::~vtkPVCompositeRenderModule()
{
  vtkPVApplication *pvApp = this->PVApplication;

  if (this->Composite && this->AbortCheckTag)
    {
    this->Composite->RemoveObserver(this->AbortCheckTag);
    this->AbortCheckTag = 0;
    }
 
  // Tree Composite
  if (this->CompositeID.ID && pvApp)
    {
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStreamToClientAndServer();
    this->CompositeID.ID = 0;
    this->Composite = NULL;
    }
  else if (this->Composite)
    {
    this->Composite->Delete();
    this->Composite = NULL;
    }
}


//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVCompositeRenderModule::CreatePartDisplay()
{
  vtkPVLODPartDisplay* pDisp;

  pDisp = vtkPVCompositePartDisplay::New();
  pDisp->SetLODResolution(this->LODResolution);
  return pDisp;
}



//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::StillRender()
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
  // If using a RenderingGroup (i.e. vtkAllToNPolyData), do not do
  // local rendering
  if (!this->PVApplication->GetUseRenderingGroup() &&
      (float)(totalMemory)/1000.0 < this->GetCompositeThreshold())
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

  // No reduction for still render.
  if (this->PVApplication && this->CompositeID.ID)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetReductionFactor" << 1
      << vtkClientServerStream::End;
    pm->SendStreamToClient();
    if (this->PVApplication->GetClientMode())
      {
      // No squirt if disabled, otherwise only lossless or still render.  
      int squirtLevel = 0;
      if (this->SquirtLevel)
        {
        squirtLevel = 1;
        }
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "SetSquirtLevel" << squirtLevel
        << vtkClientServerStream::End;
      pm->SendStreamToClient();
      }
    }

  // Switch the compositer to local/composite mode.
  if (this->CompositeID.ID)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    if (localRender)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOff"
        << vtkClientServerStream::End;
      pm->SendStreamToClient();
      }
    else
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOn"
        << vtkClientServerStream::End;
      pm->SendStreamToClient();
      }
    // Save this so we know where to get the z buffer.
    this->LocalRender = localRender;
   }


  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->RenderWindow->SetDesiredUpdateRate(0.002);
  // this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

  this->GetPVApplication()->SetGlobalLODFlag(0);
  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::InteractiveRender()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  vtkPVLODPartDisplayInformation* info;
  unsigned long totalGeoMemory = 0;
  unsigned long totalLODMemory = 0;
  unsigned long tmpMemory;
  int localRender;
  int useLOD;
  vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();

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
  if (!this->PVApplication->GetUseRenderingGroup() &&
      (float)(tmpMemory)/1000.0 < this->GetCompositeThreshold())
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
      if (useLOD)
        {
        pDisp->SetLODCollectionDecision(localRender);
        }
      else
        {
        pDisp->SetCollectionDecision(localRender);
        }
      pDisp->Update();
      }
    }

  // Switch the compositer to local/composite mode.
  if (this->CompositeID.ID)
    {
    if (localRender)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOff"
        << vtkClientServerStream::End;
      pm->SendStreamToClient();
      }
    else
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOn"
        << vtkClientServerStream::End;
      pm->SendStreamToClient();
      }
    // Save this so we know where to get the z buffer.
    this->LocalRender = localRender;
   }

  // Handle squirt compression.
  if (this->PVApplication->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetSquirtLevel" << this->SquirtLevel
      << vtkClientServerStream::End;
    pm->SendStreamToClient();
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
  if (this->CompositeID.ID && ! localRender)
    {
    this->ComputeReductionFactor();
    }

  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");

  // These times are used to determine reduction factor.
  // Not needed for still rendering !!!
  if (this->Composite)
    {
    this->InteractiveRenderTime = this->Composite->GetMaxRenderTime();
    this->InteractiveCompositeTime = this->Composite->GetCompositeTime()
      + this->Composite->GetGetBuffersTime()
      + this->Composite->GetSetBuffersTime();
    }
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::ComputeReductionFactor()
{
  float renderTime = 1.0 / this->RenderWindow->GetDesiredUpdateRate();
  int *windowSize = this->RenderWindow->GetSize();
  int area, reducedArea, reductionFactor;
  float timePerPixel;
  float getBuffersTime, setBuffersTime, transmitTime;
  float newReductionFactor;
  float maxReductionFactor;
  
  newReductionFactor = 1;
  if (this->ReductionFactor > 1)
    {
    // We have to come up with a more consistent way to compute reduction.
    newReductionFactor = this->ReductionFactor;
    if (this->Composite)
      {
      // Leave halve time for compositing.
      renderTime = renderTime * 0.5;
      // Try to factor in user preference.
      renderTime = renderTime / (float)(this->ReductionFactor);
      // Compute time for each pixel on the last render.
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
  
      // Do not let the width go below 150.
      maxReductionFactor = windowSize[0] / 150.0;
      if (maxReductionFactor > this->ReductionFactor)
        {
        maxReductionFactor = this->ReductionFactor;
        }

      if (newReductionFactor > maxReductionFactor)
        {
        newReductionFactor = maxReductionFactor;
        }
      if (newReductionFactor < 1.0)
        {
        newReductionFactor = 1.0;
        }
      }
    }

  if (this->PVApplication && this->CompositeID.ID)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetReductionFactor"
      << int(newReductionFactor)
      << vtkClientServerStream::End;
    pm->SendStreamToClient();
    }
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::SetCompositeThreshold(float threshold)
{
  this->CompositeThreshold = threshold;

  // This will cause collection to be re evaluated.
  this->SetTotalVisibleMemorySizeValid(0);
}

//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::SetUseCompositeWithFloat(int val)
{
  if (this->Composite)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetUseChar" << (val?0:1)
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
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
void vtkPVCompositeRenderModule::SetUseCompositeWithRGBA(int val)
{
  if (this->Composite)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetUseRGB" << (val?0:1)
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
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
void vtkPVCompositeRenderModule::SetUseCompositeCompression(int)
{
  vtkErrorMacro("SetUseCompositeCompression not "
                "implemented for " << this->GetClassName());
}


//-----------------------------------------------------------------------------
int vtkPVCompositeRenderModule::MakeCollectionDecision()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  int decision = 1;

  // Do I really need to store the TotalVisibleMemorySIze in the application???
  if (this->GetTotalVisibleMemorySizeValid())
    {
    return this->CollectionDecision;
    }

  this->ComputeTotalVisibleMemorySize();
  this->SetTotalVisibleMemorySizeValid(1);

  if (this->TotalVisibleGeometryMemorySize > 
      this->GetCompositeThreshold()*1000)
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
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetCollectionDecision(this->CollectionDecision);
      }
    }

  return this->CollectionDecision;
}


//-----------------------------------------------------------------------------
int vtkPVCompositeRenderModule::MakeLODCollectionDecision()
{
  vtkObject* object;
  vtkPVCompositePartDisplay* pDisp;
  int decision = 1;

  if (this->GetTotalVisibleMemorySizeValid())
    {
    return this->LODCollectionDecision;
    }

  this->ComputeTotalVisibleMemorySize();
  this->SetTotalVisibleMemorySizeValid(1);
  if (this->TotalVisibleLODMemorySize > 
      this->GetCompositeThreshold()*1000)
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
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp)
      {
      pDisp->SetLODCollectionDecision(this->LODCollectionDecision);
      }
    }

  return this->LODCollectionDecision;
}


//----------------------------------------------------------------------------
float vtkPVCompositeRenderModule::GetZBufferValue(int x, int y)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->LocalRender)
    {
    return this->Superclass::GetZBufferValue(x, y);
    }

  // Only MPI has a pointer to a composite.
  if (this->Composite)
    {
    return this->Composite->GetZ(x, y);
    }

  // If client-server...
  if (pvApp->GetClientMode())
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "GetZBufferValue" << x << y
      << vtkClientServerStream::End;
    pm->SendStreamToClient();
    float z = 0;
    if(pm->GetLastClientResult().GetArgument(0, 0, &z))
      {
      return z;
      }
    else
      {
      vtkErrorMacro("Error getting float value from GetZBufferValue result.");
      }
    }

  vtkErrorMacro("Unknown RenderModule mode.");
  return 0;
}



//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "CompositeThreshold: " << this->CompositeThreshold << endl;

  if (this->CompositeID.ID)
    {
    os << indent << "CompositeID: " << this->CompositeID.ID << endl;
    }

  os << indent << "InteractiveCompositeTime: " 
     << this->GetInteractiveCompositeTime() << endl;
  os << indent << "StillCompositeTime: " 
     << this->GetStillCompositeTime() << endl;

  os << indent << "ReductionFactor: " << this->ReductionFactor << endl;
  os << indent << "SquirtLevel: " << this->SquirtLevel << endl;
}

