/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCompositeRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCompositeRenderModule.h"

#include "vtkObjectFactory.h"
#include "vtkTimerLog.h"
#include "vtkPVTreeComposite.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkPVProcessModule.h"
#include "vtkCallbackCommand.h"
#include "vtkPVCompositePartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkClientServerStream.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCompositeRenderModule);
vtkCxxRevisionMacro(vtkPVCompositeRenderModule, "1.4");


//----------------------------------------------------------------------------
vtkPVCompositeRenderModule::vtkPVCompositeRenderModule()
{
  this->LocalRender = 1;
  this->CompositeThreshold = 20.0;

  this->Composite                = 0;
  this->CompositeID.ID           = 0;
  this->InteractiveCompositeTime = 0;
  this->StillCompositeTime       = 0;

  this->CollectionDecision      = -1;
  this->LODCollectionDecision   = -1;

  this->ReductionFactor = 2;
  this->SquirtLevel = 0;
}

//----------------------------------------------------------------------------
vtkPVCompositeRenderModule::~vtkPVCompositeRenderModule()
{
  vtkPVProcessModule *pm = this->ProcessModule;

  if (this->Composite && this->AbortCheckTag)
    {
    this->Composite->RemoveObserver(this->AbortCheckTag);
    this->AbortCheckTag = 0;
    }
 
  // Tree Composite
  if (this->CompositeID.ID && pm)
    {
    pm->DeleteStreamObject(this->CompositeID);
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
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
  vtkPVLODPartDisplay* pDisp = vtkPVCompositePartDisplay::New();
  pDisp->SetProcessModule(this->ProcessModule);
  pDisp->SetLODResolution(this->LODResolution); //this line differ from vtkPVMultiDisplayRenderModule
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

  vtkPVProcessModule* pm = this->ProcessModule;

  pm->SendPrepareProgress();

  // Find out whether we are going to render localy.
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      // This updates if required (collection disabled).
      info = pDisp->GetInformation();
      totalMemory += info->GetGeometryMemorySize();
      }
    }
  localRender = 0;
  // If using a RenderingGroup (i.e. vtkAllToNPolyData), do not do
  // local rendering
  //if (!this->PVApplication->GetUseRenderingGroup() &&
  if (((float)(totalMemory)/1000.0 < this->GetCompositeThreshold() ||
       (!this->ProcessModule->GetClientMode() && pm->GetNumberOfPartitions()<2)))
    {
    localRender = 1;
    }
  // Change the collection flags and update.
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
      {
      pDisp->SetCollectionDecision(localRender);
      pDisp->Update();
      }
    }

  // No reduction for still render.
  if (pm && this->CompositeID.ID)
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetImageReductionFactor" << 1
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    if (pm->GetClientMode())
      {
      // No squirt if disabled, otherwise only lossless for still render.  
      int squirtLevel = 0;
      if (this->SquirtLevel)
        {
        squirtLevel = 1;
        }
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "SetSquirtLevel" << squirtLevel
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT);
      }
    }

  // Switch the compositer to local/composite mode.
  // This is cheap since we only change the client.
  if (this->CompositeID.ID)
    {
    if (localRender)
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOff"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT);
      }
    else
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOn"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT);
      }
    // Save this so we know where to get the z buffer (for picking?).
    this->LocalRender = localRender;
   }

  // This was to fix a clipping range bug. Still Render can get called some 
  // funky ways.  Some do not reset the clipping range.
  // Interactive renders get called through the PVInteractorStyles
  // which call ResetCameraClippingRange on the Renderer.
  // We could convert still renders to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  // This used to be the way LODs (geometry and pixel redection) we choosen.
  // Geometry LODs are now choosen globally to simplify the process.
  // The algorithm in the render window was too complex and often
  // made bad choices.  Most composite ImageReduction values
  // are constant (for interactive render)and just directly choosen by the user.
  // We can make make the choice more sophisticated in the future.
  this->RenderWindow->SetDesiredUpdateRate(0.002);

  this->ProcessModule->SetGlobalLODFlag(0);

  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");

  pm->SendCleanupPendingProgress();
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
  vtkPVProcessModule* pm = this->ProcessModule;
  pm->SendPrepareProgress();

  // Compute memory totals.
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
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
    this->ProcessModule->SetGlobalLODFlag(0);
    }
  else
    {
    useLOD = 1;
    tmpMemory = totalLODMemory;
    this->ProcessModule->SetGlobalLODFlag(1);
    }

  // MakeCollection Decision.
  localRender = 0;
  //if (!this->PVApplication->GetUseRenderingGroup() &&
  if ( ((float)(tmpMemory)/1000.0 < this->GetCompositeThreshold() ||
       ( ! pm->GetClientMode() && pm->GetNumberOfPartitions()<2)))
      
    {
    localRender = 1;
    }
  // Change the collection flags and update.
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
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
      pm->SendStream(vtkProcessModule::CLIENT);
      }
    else
      {
      pm->GetStream()
        << vtkClientServerStream::Invoke
        << this->CompositeID << "UseCompositingOn"
        << vtkClientServerStream::End;
      pm->SendStream(vtkProcessModule::CLIENT);
      }
    // Save this so we know where to get the z buffer.
    this->LocalRender = localRender;
   }

  // Handle squirt compression.
  if (pm->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetSquirtLevel" << this->SquirtLevel
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  // This might be used for Reduction factor.
  // See comment in still render.
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
  pm->SendCleanupPendingProgress();
}


//----------------------------------------------------------------------------
void vtkPVCompositeRenderModule::ComputeReductionFactor()
{
  float renderTime = 1.0 / this->RenderWindow->GetDesiredUpdateRate();
  int *windowSize = this->RenderWindow->GetSize();
  int area, reducedArea;
  float reductionFactor;
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
      reductionFactor = (float)this->Composite->GetImageReductionFactor();
      reducedArea = (int)(area / (reductionFactor * reductionFactor));
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

  if (this->ProcessModule && this->CompositeID.ID)
    {
    vtkPVProcessModule* pm = this->ProcessModule;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetImageReductionFactor"
      << int(newReductionFactor)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
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
    vtkPVProcessModule* pm = this->ProcessModule;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetUseChar" << (val?0:1)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
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
    vtkPVProcessModule* pm = this->ProcessModule;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetUseRGB" << (val?0:1)
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
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
    
  this->Displays->InitTraversal();
  while ( (object=this->Displays->GetNextItemAsObject()) )
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
    
  this->Displays->InitTraversal();
  while ( (object=this->Displays->GetNextItemAsObject()) )
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
  vtkPVProcessModule* pm = this->ProcessModule;
  if (pm->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "GetZBufferValue" << x << y
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    float z = 0;
    if(pm->GetLastResult(vtkProcessModule::CLIENT).GetArgument(0, 0, &z))
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

