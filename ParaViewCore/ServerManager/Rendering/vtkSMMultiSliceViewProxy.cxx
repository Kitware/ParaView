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

#include "vtkBoundingBox.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSourceProxy.h"

#include <assert.h>
#include <vector>

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
const char* vtkSMMultiSliceViewProxy::GetRepresentationType(
  vtkSMSourceProxy* producer, int outputPort)
{
  if (!producer)
    {
    return 0;
    }

  assert("Session should be valid" && this->GetSession());
  vtkSMSessionProxyManager* pxm = this->GetSessionProxyManager();

  // Choose which type of representation proxy to create.
  vtkSMProxy* prototype = pxm->GetPrototypeProxy("representations",
    "CompositeMultiSliceRepresentation");
  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    prototype->GetProperty("Input"));
  pp->RemoveAllUncheckedProxies();
  pp->AddUncheckedInputConnection(producer, outputPort);
  bool sg = (pp->IsInDomains()>0);
  pp->RemoveAllUncheckedProxies();
  return sg? "CompositeMultiSliceRepresentation" : NULL;
}


//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMMultiSliceViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* proxy, int outputPort)
{
  vtkSMRepresentationProxy* repr = this->Superclass::CreateDefaultRepresentation(
    proxy,outputPort);
  if (repr)
    {
    this->InitDefaultSlices(vtkSMSourceProxy::SafeDownCast(proxy), outputPort);
    return repr;
    }

  // Currently only images can be shown
  // vtkErrorMacro("This view only supports Multi-Slice representation.");
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMMultiSliceViewProxy::InitDefaultSlices(
  vtkSMSourceProxy* source, int opport)
{
  if (!source)
    {
    return;
    }
  double bounds[6] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                       VTK_DOUBLE_MAX, VTK_DOUBLE_MIN,
                       VTK_DOUBLE_MAX, VTK_DOUBLE_MIN};
  vtkPVDataInformation* info = source->GetDataInformation(opport);
  if(info)
    {
    info->GetBounds(bounds);
    if(vtkBoundingBox::IsValid(bounds))
      {
      double center[3];
      for(int i=0;i<3;i++)
        {
        center[i] = (bounds[2*i] + bounds[2*i+1])/2.0;
        }

      // Add orthogonal X,Y,Z slices based on center position.
      std::vector<double> xSlices =
        vtkSMPropertyHelper(this, "XSlicesValues").GetDoubleArray();
      std::vector<double> ySlices =
        vtkSMPropertyHelper(this, "YSlicesValues").GetDoubleArray();
      std::vector<double> zSlices =
        vtkSMPropertyHelper(this, "ZSlicesValues").GetDoubleArray();

      xSlices.push_back(center[0]);
      ySlices.push_back(center[1]);
      zSlices.push_back(center[2]);

      vtkSMPropertyHelper(this, "XSlicesValues").Set(&xSlices[0],
       static_cast<unsigned int>(xSlices.size()));
      vtkSMPropertyHelper(this, "YSlicesValues").Set(&ySlices[0],
       static_cast<unsigned int>(ySlices.size()));
      vtkSMPropertyHelper(this, "ZSlicesValues").Set(&zSlices[0],
       static_cast<unsigned int>(ySlices.size()));
      }
    }
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
