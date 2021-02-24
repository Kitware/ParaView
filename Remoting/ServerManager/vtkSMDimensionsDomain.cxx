/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDimensionsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDimensionsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkStructuredData.h"

vtkStandardNewMacro(vtkSMDimensionsDomain);
//----------------------------------------------------------------------------
vtkSMDimensionsDomain::vtkSMDimensionsDomain() = default;

//----------------------------------------------------------------------------
vtkSMDimensionsDomain::~vtkSMDimensionsDomain() = default;

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::Update(vtkSMProperty*)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->GetRequiredProperty("Input"));
  vtkSMIntVectorProperty* ivp =
    vtkSMIntVectorProperty::SafeDownCast(this->GetRequiredProperty("Direction"));
  if (pp)
  {
    this->Update(pp, ivp);
  }
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::Update(vtkSMProxyProperty* pp, vtkSMIntVectorProperty* ivp)
{
  std::vector<vtkEntry> entries;

  int extent[6] = { 0, 0, 0, 0, 0, 0 };
  this->GetExtent(pp, extent);
  if (extent[1] < extent[0] || extent[3] < extent[2] || extent[5] < extent[4])
  {
    // no valid extents provided by the data, just set the range to
    // (0,0,0,0,0,0,0)
    extent[0] = extent[1] = extent[2] = extent[3] = extent[4] = extent[5] = 0;
  }
  if (ivp)
  {
    int direction = this->GetDirection(ivp);
    switch (direction)
    {
      case VTK_YZ_PLANE:
        entries.push_back(vtkEntry(0, extent[1] - extent[0]));
        break;

      case VTK_XZ_PLANE:
        entries.push_back(vtkEntry(0, extent[3] - extent[2]));
        break;

      case VTK_XY_PLANE:
      default:
        entries.push_back(vtkEntry(0, extent[5] - extent[4]));
    }
  }
  else
  {
    entries.push_back(vtkEntry(0, extent[1] - extent[0]));
    entries.push_back(vtkEntry(0, extent[3] - extent[2]));
    entries.push_back(vtkEntry(0, extent[5] - extent[4]));
  }
  this->SetEntries(entries);
}

//----------------------------------------------------------------------------
int vtkSMDimensionsDomain::GetDirection(vtkSMIntVectorProperty* ivp)
{
  return ivp->GetUncheckedElement(0);
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::GetExtent(vtkSMProxyProperty* pp, int extent[6])
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
    {
      vtkPVDataInformation* info =
        sp->GetDataInformation((ip ? ip->GetUncheckedOutputPortForConnection(i) : 0));
      if (!info)
      {
        continue;
      }
      info->GetExtent(extent);
      return;
    }
  }

  extent[0] = extent[1] = extent[2] = extent[3] = extent[4] = extent[5] = 0;
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
