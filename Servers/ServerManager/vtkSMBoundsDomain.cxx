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
#include "vtkPVXMLElement.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMBoundsDomain);
vtkCxxRevisionMacro(vtkSMBoundsDomain, "1.3");

//---------------------------------------------------------------------------
vtkSMBoundsDomain::vtkSMBoundsDomain()
{
  this->Mode = vtkSMBoundsDomain::NORMAL;
}

//---------------------------------------------------------------------------
vtkSMBoundsDomain::~vtkSMBoundsDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::Update(vtkSMProperty*)
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

//---------------------------------------------------------------------------
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
      if (this->Mode == vtkSMBoundsDomain::NORMAL)
        {
        for (j = 0; j < 3; j++)
          {
          this->AddMinimum(j, bounds[2*j]);
          this->AddMaximum(j, bounds[2*j+1]);
          }
        }
      else if (this->Mode == vtkSMBoundsDomain::MAGNITUDE)
        {
        double magn = sqrt((bounds[1]-bounds[0]) * (bounds[1]-bounds[0]) +
                           (bounds[3]-bounds[2]) * (bounds[3]-bounds[2]) +
                           (bounds[5]-bounds[4]) * (bounds[5]-bounds[4]));
        this->AddMinimum(0, -magn);
        this->AddMaximum(0,  magn);
        }
      return;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMBoundsDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  const char* mode = element->GetAttribute("mode");

  if (mode)
    {
    if (strcmp(mode, "normal") == 0)
      {
      this->Mode = vtkSMBoundsDomain::NORMAL;
      }
    else if (strcmp(mode, "magnitude") == 0)
      {
      this->Mode = vtkSMBoundsDomain::MAGNITUDE;
      }
    else
      {
      vtkErrorMacro("Unrecognized mode: " << mode);
      return 0;
      }
    }
  
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Mode: " << this->Mode << endl;
}
