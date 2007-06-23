/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIceTMultiDisplayRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIceTMultiDisplayRenderViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMIceTMultiDisplayRenderViewProxy);
vtkCxxRevisionMacro(vtkSMIceTMultiDisplayRenderViewProxy, "1.1");
//-----------------------------------------------------------------------------
vtkSMIceTMultiDisplayRenderViewProxy::vtkSMIceTMultiDisplayRenderViewProxy()
{
  this->CollectGeometryThreshold = 100.0;
  this->StillReductionFactor = 1;
}

//-----------------------------------------------------------------------------
vtkSMIceTMultiDisplayRenderViewProxy::~vtkSMIceTMultiDisplayRenderViewProxy()
{
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::EndCreateVTKObjects()
{
  // Obtain information about the tiles from the process module options.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  int *tileDims =  pm->GetOptions()->GetTileDimensions();
  this->TileDimensions[0] = tileDims[0];
  this->TileDimensions[1] = tileDims[1];
  int *tileMulls =  pm->GetOptions()->GetTileMullions();
  this->TileMullions[0] = tileMulls[0];
  this->TileMullions[1] = tileMulls[1];

  // Make the server-side windows fullscreen 
  // (unless PV_ICET_WINDOW_BORDERS is set)
  vtkSMIntVectorProperty* ivp;
  if (!getenv("PV_ICET_WINDOW_BORDERS"))
    {
    ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->RenderWindowProxy->GetProperty("FullScreen"));
    if (ivp)
      {
      ivp->SetElement(0, 1);
      }
    else
      {
      vtkErrorMacro("Failed to find property FullScreen on RenderWindowProxy.");
      }
    }

  this->Superclass::EndCreateVTKObjects();

  // We always render on the server-side when using tile displays.
  ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->RenderSyncManager->GetProperty("UseCompositing"));
  if (ivp)
    {
    ivp->SetElement(0, 1); 
    }
  this->RenderSyncManager->UpdateProperty("UseCompositing");
}

//-----------------------------------------------------------------------------
void vtkSMIceTMultiDisplayRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "CollectGeometryThreshold: " 
    << this->CollectGeometryThreshold << endl;
  os << indent << "StillReductionFactor: " 
    << this->StillReductionFactor << endl;
}
