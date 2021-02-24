/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPLYWriterProxyInitializationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPLYWriterProxyInitializationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewProxy.h"

#include <cassert>

vtkStandardNewMacro(vtkSMPLYWriterProxyInitializationHelper);
//----------------------------------------------------------------------------
vtkSMPLYWriterProxyInitializationHelper::vtkSMPLYWriterProxyInitializationHelper() = default;

//----------------------------------------------------------------------------
vtkSMPLYWriterProxyInitializationHelper::~vtkSMPLYWriterProxyInitializationHelper() = default;

//----------------------------------------------------------------------------
void vtkSMPLYWriterProxyInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement*, vtkMTimeType ts)
{
  assert(proxy != nullptr);
  if (proxy->GetProperty("ColorArrayName")->GetMTime() > ts ||
    proxy->GetProperty("LookupTable")->GetMTime() > ts)
  {
    return;
  }

  vtkSMPropertyHelper input(proxy, "Input");
  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  vtkSMViewProxy* activeView = nullptr;
  if (vtkSMProxySelectionModel* viewSM = pxm->GetSelectionModel("ActiveView"))
  {
    activeView = vtkSMViewProxy::SafeDownCast(viewSM->GetCurrentProxy());
  }

  if (activeView && input.GetAsProxy())
  {
    if (vtkSMRepresentationProxy* repr = activeView->FindRepresentation(
          vtkSMSourceProxy::SafeDownCast(input.GetAsProxy()), input.GetOutputPort()))
    {
      if (repr->GetProperty("ColorArrayName") && repr->GetProperty("LookupTable"))
      {
        vtkSMPropertyHelper rca(repr, "ColorArrayName");
        vtkSMPropertyHelper(proxy, "ColorArrayName")
          .SetInputArrayToProcess(rca.GetInputArrayAssociation(), rca.GetInputArrayNameToProcess());
        vtkSMPropertyHelper(proxy, "LookupTable")
          .Set(vtkSMPropertyHelper(repr, "LookupTable").GetAsProxy());
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMPLYWriterProxyInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
