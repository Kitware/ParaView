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
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkStructuredData.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"

vtkStandardNewMacro(vtkSMDimensionsDomain);
//----------------------------------------------------------------------------
vtkSMDimensionsDomain::vtkSMDimensionsDomain()
{
}

//----------------------------------------------------------------------------
vtkSMDimensionsDomain::~vtkSMDimensionsDomain()
{
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::Update(vtkSMProperty*)
{
  this->RemoveAllMaxima();
  this->RemoveAllMinima();

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetRequiredProperty("Direction"));
  if (pp)
    {
    this->Update(pp, ivp);
    this->InvokeModified();
    }
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::Update(vtkSMProxyProperty* pp,
  vtkSMIntVectorProperty* ivp)
{
  int extent[6] = {0, 0, 0, 0, 0, 0};
  this->GetExtent(pp, extent);
  if (extent[1] < extent[0] || extent[3] < extent[2] || extent[5] < extent[4])
    {
    // no valid extents provided by the data, just set the range to
    // (0,0,0,0,0,0,0)
    extent[0] = extent[1] = extent[2] = 
      extent[3] = extent[4] = extent[5] = 0;
    }
  if (ivp)
    {
    this->AddMinimum(0, 0);
    int direction = this->GetDirection(ivp);
    switch (direction)
      {
    case VTK_YZ_PLANE:
      this->AddMaximum(0, extent[1]-extent[0]);
      break;

    case VTK_XZ_PLANE:
      this->AddMaximum(0, extent[3]-extent[2]);
      break;

    case VTK_XY_PLANE:
    default:
      this->AddMaximum(0, extent[5]-extent[4]);
      }
    }
  else
    {
    this->AddMinimum(0, 0);
    this->AddMaximum(0, extent[1]-extent[0]);
    this->AddMinimum(1, 0);
    this->AddMaximum(1, extent[3]-extent[2]);
    this->AddMinimum(2, 0);
    this->AddMaximum(2, extent[5]-extent[4]);
    }
}

//----------------------------------------------------------------------------
int vtkSMDimensionsDomain::GetDirection(vtkSMIntVectorProperty* ivp)
{
  int val = ivp->GetElement(0);
  /* Unchecked values may not be set at all, in which case we get wrong results.
   * There's no API to check if unchecked values are set, we need to fix this
   * issue 
  if (ivp->GetNumberOfUncheckedElements() == 1)
    {
    val = ivp->GetUncheckedElement(0);
    }
    */
  return val;
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::GetExtent(vtkSMProxyProperty* pp, int extent[6])
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
    {
    vtkSMSourceProxy* sp =
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        (ip? ip->GetUncheckedOutputPortForConnection(i):0));
      if (!info)
        {
        continue;
        }
      info->GetExtent(extent);
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = pp->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        (ip? ip->GetOutputPortForConnection(i):0));
      if (!info)
        {
        continue;
        }
      info->GetExtent(extent);
      return;
      }
    }
  extent[0] = extent[1] = extent[2] = 
    extent[3] = extent[4] = extent[5] = 0;
}

//----------------------------------------------------------------------------
void vtkSMDimensionsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


