/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScatterPlotViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScatterPlotViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMScatterPlotViewProxy);
//----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy::vtkSMScatterPlotViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMScatterPlotViewProxy::~vtkSMScatterPlotViewProxy()
{
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMScatterPlotViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // if the user gave a default representation, use it.
  if (this->DefaultRepresentationName)
    {
    vtkSmartPointer<vtkSMProxy> p;
    p.TakeReference(pxm->NewProxy("representations", this->DefaultRepresentationName));
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(p);
    if (repr)
      {
      repr->Register(this);
      return repr;
      }
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
                                                 "ScatterPlotRepresentation");

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
        pxm->NewProxy("representations", "ScatterPlotRepresentation"));
    return repr;
    }

  // Currently only images can be shown 
  vtkErrorMacro("This view only supports dataset representation.");
  return 0;
}
