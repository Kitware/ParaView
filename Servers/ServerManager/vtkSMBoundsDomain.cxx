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
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMBoundsDomain);
vtkCxxRevisionMacro(vtkSMBoundsDomain, "1.4");

vtkCxxSetObjectMacro(vtkSMBoundsDomain,InputInformation,vtkPVDataInformation)

//---------------------------------------------------------------------------
vtkSMBoundsDomain::vtkSMBoundsDomain()
{
  this->Mode = vtkSMBoundsDomain::NORMAL;
  this->InputInformation = 0;
}

//---------------------------------------------------------------------------
vtkSMBoundsDomain::~vtkSMBoundsDomain()
{
  this->SetInputInformation(0);
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::Update(vtkSMProperty*)
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  
  if (this->Mode == vtkSMBoundsDomain::ORIENTED_MAGNITUDE)
    {
    this->UpdateOriented();
    return;
    }

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    }
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::UpdateOriented()
{
  if (!this->InputInformation)
    {
    return;
    }

  double bounds[6];
  this->InputInformation->GetBounds(bounds);

  vtkSMDoubleVectorProperty *normal = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetRequiredProperty("Normal"));
  vtkSMDoubleVectorProperty *origin = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetRequiredProperty("Origin"));
  if (normal && origin)
    {
    double points[8][3];
    
    double xmin = bounds[0];
    double xmax = bounds[1];
    double ymin = bounds[2];
    double ymax = bounds[3];
    double zmin = bounds[4];
    double zmax = bounds[5];

    points[0][0] = xmin; points[0][1] = ymin; points[0][2] = zmin;
    points[1][0] = xmax; points[1][1] = ymax; points[1][2] = zmax;
    points[2][0] = xmin; points[2][1] = ymin; points[2][2] = zmax;
    points[3][0] = xmin; points[3][1] = ymax; points[3][2] = zmax;
    points[4][0] = xmin; points[4][1] = ymax; points[4][2] = zmin;
    points[5][0] = xmax; points[5][1] = ymax; points[5][2] = zmin;
    points[6][0] = xmax; points[6][1] = ymin; points[6][2] = zmin;
    points[7][0] = xmax; points[7][1] = ymin; points[7][2] = zmax;

    double normalv[3], originv[3];

    unsigned int i;
    for (i=0; i<3; i++)
      {
      normalv[i] = normal->GetUncheckedElement(i);
      originv[i] = origin->GetUncheckedElement(i); 
      }

    unsigned int j;
    double dist[8];

    for(i=0; i<8; i++)
      {
      dist[i] = 0;
      for(j=0; j<3; j++)
        {
        dist[i] += (points[i][j] - originv[j])*normalv[j];
        }
      }

    double min = dist[0], max = dist[0];
    for (int i=1; i<8; i++)
      {
      if ( dist[i] < min )
        {
        min = dist[i];
        }
      if ( dist[i] > max )
        {
        max = dist[i];
        }
      }
    this->AddMinimum(0, min);
    this->AddMaximum(0, max);
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
    else if (strcmp(mode, "oriented_magnitude") == 0)
      {
      this->Mode = vtkSMBoundsDomain::ORIENTED_MAGNITUDE;
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
