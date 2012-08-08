/*=========================================================================

  Program:   ParaView
  Module:    vtkSMMultiSliceViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMMultiSliceViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include <assert.h>

vtkStandardNewMacro(vtkSMMultiSliceViewProxy);
//----------------------------------------------------------------------------
vtkSMMultiSliceViewProxy::vtkSMMultiSliceViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMMultiSliceViewProxy::~vtkSMMultiSliceViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMMultiSliceViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* source, int opport)
{
  if (!source)
    {
    return 0;
    }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Update with time to avoid domains updating without time later.
  vtkSMSourceProxy* sproxy = vtkSMSourceProxy::SafeDownCast(source);
  if (sproxy)
    {
    double view_time = vtkSMPropertyHelper(this, "ViewTime").GetAsDouble();
    sproxy->UpdatePipeline(view_time);
    }

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "CompositeMultiSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(source, opport);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  if (sg)
    {
    vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy("representations", "CompositeMultiSliceRepresentation"));
    return repr;
    }

  // Currently only images can be shown
  vtkErrorMacro("This view only supports Multi-Slice representation.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMMultiSliceViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkSMMultiSliceViewProxy::IsSelectVisiblePointsAvailable()
{
  // The original dataset and the slice don't share the same points
  return "Multi-Slice View do not allow point selection";
}
