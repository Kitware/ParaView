/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoundsDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBoundsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMBoundsDomain);
vtkCxxRevisionMacro(vtkSMBoundsDomain, "1.1");

vtkSMBoundsDomain::vtkSMBoundsDomain()
{
}

vtkSMBoundsDomain::~vtkSMBoundsDomain()
{
}

void vtkSMBoundsDomain::Update(vtkSMProperty *prop)
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  
  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    }
}

void vtkSMBoundsDomain::Update(vtkSMProxyProperty *pp)
{
  unsigned int i, j;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation();
      if (!info)
        {
        return;
        }
      double bounds[6];
      info->GetBounds(bounds);
      for (j = 0; j < 3; j++)
        {
        this->AddMinimum(j, bounds[2*j]);
        this->AddMaximum(j, bounds[2*j+1]);
        }
      return;
      }
    }
}

void vtkSMBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
