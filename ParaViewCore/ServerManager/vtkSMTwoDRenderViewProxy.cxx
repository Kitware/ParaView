/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTwoDRenderViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTwoDRenderViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"


vtkStandardNewMacro(vtkSMTwoDRenderViewProxy);
//----------------------------------------------------------------------------
vtkSMTwoDRenderViewProxy::vtkSMTwoDRenderViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMTwoDRenderViewProxy::~vtkSMTwoDRenderViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMTwoDRenderViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  vtkSMProxyManager* pxm = this->GetProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "ImageSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "ImageSliceRepresentation"));
    vtkSMPropertyHelper(repr, "UseXYPlane").Set(1);
    return repr;
    }

  // Currently only images can be shown
  vtkErrorMacro("This view only supports showing images.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMTwoDRenderViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


