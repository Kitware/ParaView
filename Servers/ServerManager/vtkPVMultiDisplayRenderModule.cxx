/*=========================================================================

  Program:   ParaView
  Module:    vtkPVMultiDisplayRenderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVMultiDisplayRenderModule.h"

#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkPVProcessModule.h"
#include "vtkPVMultiDisplayPartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkClientServerStream.h"
#include "vtkPVServerInformation.h"


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayRenderModule);
vtkCxxRevisionMacro(vtkPVMultiDisplayRenderModule, "1.2");



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
void vtkPVMultiDisplayRenderModule::SetProcessModule(vtkPVProcessModule *pm)
{
  this->Superclass::SetProcessModule(pm);
  if (pm == NULL)
    {
    return;
    }

  // We had trouble with SGI/aliasing with compositing.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pm->GetNumberOfPartitions() > 1))
    {
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->RenderWindowID 
                    << "SetMultiSamples" << 0 
                    << vtkClientServerStream::End;
    }

  this->Composite = NULL;
  this->CompositeID = pm->NewStreamObject("vtkMultiDisplayManager");
  int *tileDim = pm->GetServerInformation()->GetTileDimensions();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CompositeID << "SetTileDimensions"
    << tileDim[0] << tileDim[1]
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);

  if (pm->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetClientFlag" << 1
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);

    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pm->GetProcessModuleID() << "GetRenderServerSocketController"
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetSocketController"
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetZeroEmpty" << 0
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
  else
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetZeroEmpty" << 1
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }

  // Have to initialize after ZeroEmpty, and tile dimensions have been set.
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CompositeID << "InitializeSchedule"
    << vtkClientServerStream::End;
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CompositeID << "InitializeRMIs"
    << vtkClientServerStream::End;
  pm->GetStream() 
    << vtkClientServerStream::Invoke
    <<  this->CompositeID 
    << "SetRenderWindow"
    << this->RenderWindowID
    << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
}

//----------------------------------------------------------------------------
vtkPVPartDisplay* vtkPVMultiDisplayRenderModule::CreatePartDisplay()
{
  vtkPVPartDisplay* pDisp = vtkPVMultiDisplayPartDisplay::New();
  pDisp->SetProcessModule(this->GetProcessModule());
  return pDisp;
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
  vtkPVProcessModule* pm = this->GetProcessModule();

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
  if ((float)(totalMemory)/1000.0 < this->GetCompositeThreshold())
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

  if (this->ProcessModule && this->CompositeID.ID != 0)
    {
    pm->GetStream() 
      << vtkClientServerStream::Invoke 
      << this->CompositeID
      << "SetImageReductionFactor" << 1 
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT);
    }

  // Switch the compositer to local/composite mode.
  // This is cheap since we only change the client.
  this->LocalRender = localRender;
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
    }  

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  this->Renderer->ResetCameraClippingRange();

  this->RenderWindow->SetDesiredUpdateRate(0.002);
  // this->GetPVWindow()->GetInteractor()->GetStillUpdateRate());

  this->GetProcessModule()->SetGlobalLODFlag(0);

  // This is the only thing I believe makes a difference
  // for this sublcass !!!!!!
  if ( ! localRender)
    {
    // A bit of a hack to have client use LOD 
    // which may be different than satellites.
    this->GetProcessModule()->SetGlobalLODFlagInternal(1);
    }

  vtkTimerLog::MarkStartEvent("Still Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Still Render");

  // If we do not restore the consistent original value,
  // it will get stuck in high res on satellites.
  if ( ! localRender)
    {
    this->GetProcessModule()->SetGlobalLODFlagInternal(0);
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
  vtkPVProcessModule* pm = this->GetProcessModule();

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
    this->GetProcessModule()->SetGlobalLODFlag(0);
    }
  else
    {
    useLOD = 1;
    tmpMemory = totalLODMemory;
    this->GetProcessModule()->SetGlobalLODFlag(1);
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
  this->Displays->InitTraversal();
  while ( (object = this->Displays->GetNextItemAsObject()) )
    {
    pDisp = vtkPVCompositePartDisplay::SafeDownCast(object);
    if (pDisp && pDisp->GetVisibility())
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
      this->LocalRender = localRender;
      }
    }

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
    this->GetProcessModule()->SetGlobalLODFlagInternal(1);
    }

  // Still Render can get called some funky ways.
  // Interactive renders get called through the PVInteractorStyles
  // which cal ResetCameraClippingRange on the Renderer.
  // We could convert them to call a method on the module directly ...
  // Reset camera clipping range has to be called after
  // LOD flag is set, other wise, the wrong bounds could be used.
  this->Renderer->ResetCameraClippingRange();
  
  vtkTimerLog::MarkStartEvent("Interactive Render");
  this->RenderWindow->Render();
  vtkTimerLog::MarkEndEvent("Interactive Render");

  // If we do not restore the consistent original value,
  // it will get stuck in high res on satellites.
  if ( ! localRender)
    {
    this->GetProcessModule()->SetGlobalLODFlagInternal(useLOD);
    }


}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::SetUseCompositeCompression(int val)
{
  if (this->CompositeID.ID)
    {
    vtkPVProcessModule* pm = this->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetUseCompositeCompression" << val
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
    }
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

