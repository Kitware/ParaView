/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLODRenderModuleProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMLODRenderModuleProxy.h"
#include "vtkObjectFactory.h"
#include "vtkCollectionIterator.h"
#include "vtkCollection.h"
#include "vtkSMLODDisplayProxy.h"
#include "vtkPVLODPartDisplayInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkRenderWindow.h"
#include "vtkSMProxyManager.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkTimerLog.h"
#include "vtkCommand.h"

//*****************************************************************************
class vtkSMLODRenderModuleProxyObserver : public vtkCommand
{
public:
  static vtkSMLODRenderModuleProxyObserver* New()
    {
    return new vtkSMLODRenderModuleProxyObserver;
    }
  virtual void Execute(vtkObject*, unsigned long, void*)
    {
    if (this->LODRenderModuleProxy)
      {
      this->LODRenderModuleProxy->SetTotalVisibleGeometryMemorySizeValid(0);
      this->LODRenderModuleProxy->SetTotalVisibleLODGeometryMemorySizeValid(0);
      }
    }
  void SetLODRenderModuleProxy(vtkSMLODRenderModuleProxy* p)
    {
    this->LODRenderModuleProxy = p;
    }
protected:
  vtkSMLODRenderModuleProxyObserver()
    {
    this->LODRenderModuleProxy = 0;
    }
  vtkSMLODRenderModuleProxy* LODRenderModuleProxy;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSMLODRenderModuleProxy);
vtkCxxRevisionMacro(vtkSMLODRenderModuleProxy, "1.1.2.2");
//-----------------------------------------------------------------------------
vtkSMLODRenderModuleProxy::vtkSMLODRenderModuleProxy()
{
  this->LODThreshold = 0.0;
  this->LODResolution = 10;
  this->TotalVisibleGeometryMemorySizeValid = 0;
  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODGeometryMemorySize= 0;
  this->TotalVisibleLODGeometryMemorySizeValid = 0;
  this->SetDisplayXMLName("LODDisplay");
  this->Observer = vtkSMLODRenderModuleProxyObserver::New();
  this->Observer->SetLODRenderModuleProxy(this);
}

//-----------------------------------------------------------------------------
vtkSMLODRenderModuleProxy::~vtkSMLODRenderModuleProxy()
{
  this->Observer->SetLODRenderModuleProxy(0);
  this->Observer->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMLODRenderModuleProxy::AddDisplay(vtkSMDisplayProxy* disp)
{
  this->Superclass::AddDisplay(disp);
  
  vtkSMLODDisplayProxy* pDisp = vtkSMLODDisplayProxy::SafeDownCast(disp);
  if (!pDisp)
    {
    return;
    }
  pDisp->AddObserver(vtkSMLODDisplayProxy::InformationInvalidatedEvent, this->Observer);
}
//-----------------------------------------------------------------------------
void vtkSMLODRenderModuleProxy::RemoveDisplay(vtkSMDisplayProxy* disp)
{
  this->Superclass::RemoveDisplay(disp);

  vtkSMLODDisplayProxy* pDisp = vtkSMLODDisplayProxy::SafeDownCast(disp);
  if (!pDisp)
    {
    return;
    }
  pDisp->RemoveObserver(this->Observer);
}
//-----------------------------------------------------------------------------
vtkSMDisplayProxy* vtkSMLODRenderModuleProxy::CreateDisplayProxy()
{
  vtkSMDisplayProxy* pDisp = this->Superclass::CreateDisplayProxy();
  if (!pDisp)
    {
    return NULL;
    }
  
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    pDisp->GetProperty("LODResolution"));
  if (!ivp)
    {
    vtkErrorMacro("Failed to find property LODResolution on vtkSMLODDisplayProxy.");
    }
  else
    {
    ivp->SetElement(0, this->LODResolution);
    }
  // pDisp->UpdateVTKObjects(); Don't call UpdateVTKObjects as it will create the
  // parts. We don't want to explicity create the parts, they will be
  // created when the Input is set.
  return pDisp;
}

//-----------------------------------------------------------------------------
void vtkSMLODRenderModuleProxy::SetLODResolution(int resolution)
{
  if (this->LODResolution == resolution)
    {
    return;
    }
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  this->LODResolution = resolution;

  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMLODDisplayProxy* pDisp = vtkSMLODDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp)
      {
      vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
        pDisp->GetProperty("LODResolution"));
      if (!ivp)
        {
        vtkErrorMacro("Failed to find property LODResolution on "
          "vtkSMLODDisplayProxy.");
        continue;
        }
      ivp->SetElement(0, this->LODResolution);
      pDisp->UpdateVTKObjects();
      }
    }
  iter->Delete();
}

//-----------------------------------------------------------------------------
unsigned long vtkSMLODRenderModuleProxy::GetTotalVisibleGeometryMemorySize()
{
  if (!this->TotalVisibleGeometryMemorySizeValid)
    {
    this->ComputeTotalVisibleMemorySize();
    }
  return this->TotalVisibleGeometryMemorySize;
}

//-----------------------------------------------------------------------------
unsigned long vtkSMLODRenderModuleProxy::GetTotalVisibleLODGeometryMemorySize()
{
  if (!this->TotalVisibleLODGeometryMemorySizeValid)
    {
    this->ComputeTotalVisibleMemorySize();
    }
  return this->TotalVisibleLODGeometryMemorySize;
}

//-----------------------------------------------------------------------------
void vtkSMLODRenderModuleProxy::ComputeTotalVisibleMemorySize()
{
  this->TotalVisibleGeometryMemorySize = 0;
  this->TotalVisibleLODGeometryMemorySize = 0;
  vtkCollectionIterator* iter = this->Displays->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkSMLODDisplayProxy* pDisp = vtkSMLODDisplayProxy::SafeDownCast(
      iter->GetCurrentObject());
    if (pDisp && pDisp->GetVisibilityCM())
      {
      vtkPVLODPartDisplayInformation* info = pDisp->GetLODInformation();
      
      if (pDisp->GetVolumeRenderMode())
        {
        // If we are volume rendering, count size of total geometry, not
        // just the surface.  This is not perfect because the source
        // may have been tetrahedralized.
        vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
          pDisp->GetProperty("Input"));
        if (pp && pp->GetNumberOfProxies() > 0)
          {
          vtkPVDataInformation* info2 = vtkSMSourceProxy::SafeDownCast(
            pp->GetProxy(0))->GetDataInformation();
          this->TotalVisibleGeometryMemorySize += 
            info2->GetMemorySize();
          }
        }
      else
        {
        this->TotalVisibleGeometryMemorySize += info->GetGeometryMemorySize();
        }
      this->TotalVisibleLODGeometryMemorySize += info->GetLODGeometryMemorySize();
      }
    }
  iter->Delete();
  this->TotalVisibleGeometryMemorySizeValid = 1;
  this->TotalVisibleLODGeometryMemorySizeValid = 1;
}

//-----------------------------------------------------------------------------
void vtkSMLODRenderModuleProxy::InteractiveRender()
{
  this->UpdateAllDisplays();

  // Used in subclass for window subsampling, but not really necessary here.
  //this->RenderWindow->SetDesiredUpdateRate(this->InteractiveUpdateRate);
  vtkPVProcessModule* pm = vtkPVProcessModule::SafeDownCast(
    vtkProcessModule::GetProcessModule());

  // We need to decide globally whether to use decimated geometry.  
  if (this->GetUseLODDecision())
    {
    pm->SetGlobalLODFlag(1);
    // We call this again because the LOD branches
    // may not have been updated.
    // this->UpdateAllDisplays(); Superclass::InteractiveRender will call this.
    }
  else
    {
    pm->SetGlobalLODFlag(0);
    }  

  this->Superclass::InteractiveRender();
}

//-----------------------------------------------------------------------------
int vtkSMLODRenderModuleProxy::GetUseLODDecision()
{
  if (this->GetTotalVisibleGeometryMemorySize() > this->LODThreshold*1000)
    {
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMLODRenderModuleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
}
