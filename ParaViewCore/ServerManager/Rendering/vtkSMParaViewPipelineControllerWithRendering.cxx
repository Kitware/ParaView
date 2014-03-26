/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParaViewPipelineControllerWithRendering.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMParaViewPipelineControllerWithRendering.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPVRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMTransferFunctionManager.h"

vtkStandardNewMacro(vtkSMParaViewPipelineControllerWithRendering);
//----------------------------------------------------------------------------
vtkSMParaViewPipelineControllerWithRendering::vtkSMParaViewPipelineControllerWithRendering()
{
}

//----------------------------------------------------------------------------
vtkSMParaViewPipelineControllerWithRendering::~vtkSMParaViewPipelineControllerWithRendering()
{
}

//----------------------------------------------------------------------------
bool vtkSMParaViewPipelineControllerWithRendering::RegisterRepresentationProxy(vtkSMProxy* proxy)
{
  if (!this->Superclass::RegisterRepresentationProxy(proxy))
    {
    return false;
    }

  if (vtkSMPVRepresentationProxy::GetUsingScalarColoring(proxy))
    {
    // setup transfer functions if none are set.
    vtkSMPropertyHelper helper(proxy, "ColorArrayName");
    const char* arrayName = helper.GetInputArrayNameToProcess();
    if (arrayName != NULL && arrayName[0] != '\0')
      {
      vtkNew<vtkSMTransferFunctionManager> mgr;
      if (vtkSMProperty* sofProperty = proxy->GetProperty("ScalarOpacityFunction"))
        {
        vtkSMProxy* sofProxy =
          mgr->GetOpacityTransferFunction(arrayName, proxy->GetSessionProxyManager());
        vtkSMPropertyHelper(sofProperty).Set(sofProxy);
        }
      if (vtkSMProperty* lutProperty = proxy->GetProperty("LookupTable"))
        {
        vtkSMProxy* lutProxy =
          mgr->GetColorTransferFunction(arrayName, proxy->GetSessionProxyManager());
        vtkSMPropertyHelper(lutProperty).Set(lutProxy);
        vtkSMPVRepresentationProxy::RescaleTransferFunctionToDataRange(proxy, true);
        }
      }
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkSMParaViewPipelineControllerWithRendering::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
