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
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVMultiDisplayPartDisplay.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkCollection.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkClientServerStream.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVMultiDisplayRenderModule);
vtkCxxRevisionMacro(vtkPVMultiDisplayRenderModule, "1.10.2.1");



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
  vtkPVProcessModule* pm = pvApp->GetProcessModule();

  // We had trouble with SGI/aliasing with compositing.
  if (this->RenderWindow->IsA("vtkOpenGLRenderWindow") &&
      (pvApp->GetProcessModule()->GetNumberOfPartitions() > 1))
    {
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->RenderWindowID 
                    << "SetMultiSamples" << 0 
                    << vtkClientServerStream::End;
    }

  this->Composite = NULL;
  this->CompositeID = pm->NewStreamObject("vtkMultiDisplayManager");
  int *tileDim = pvApp->GetTileDimensions();
  pm->GetStream()
    << vtkClientServerStream::Invoke
    << this->CompositeID << "SetTileDimensions"
    << tileDim[0] << tileDim[1]
    << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();

  if (pvApp->GetClientMode())
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetClientFlag" << 1
      << vtkClientServerStream::End;
    pm->SendStreamToClient();

    pm->GetStream()
      << vtkClientServerStream::Invoke
      << pm->GetApplicationID() << "GetProcessModule"
      << vtkClientServerStream::End
      << vtkClientServerStream::Invoke
      << vtkClientServerStream::LastResult << "GetSocketController"
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
    pm->SendStreamToClientAndServer();
    }
  else
    {
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetZeroEmpty" << 1
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
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
  pm->SendStreamToClientAndServer();
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
  vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();

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

  if (this->PVApplication && this->CompositeID.ID != 0)
    {
    pm->GetStream() 
      << vtkClientServerStream::Invoke 
      << this->CompositeID
      << "SetImageReductionFactor" << 1 
      << vtkClientServerStream::End;
    pm->SendStreamToClient();
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
  if (this->CompositeID.ID)
    {
    vtkPVProcessModule* pm = this->PVApplication->GetProcessModule();
    pm->GetStream()
      << vtkClientServerStream::Invoke
      << this->CompositeID << "SetUseCompositeCompression" << val
      << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
}

//----------------------------------------------------------------------------
void vtkPVMultiDisplayRenderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

