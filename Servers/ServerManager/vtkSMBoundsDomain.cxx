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
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMBoundsDomain);

vtkCxxSetObjectMacro(vtkSMBoundsDomain,InputInformation,vtkPVDataInformation)

//---------------------------------------------------------------------------
vtkSMBoundsDomain::vtkSMBoundsDomain()
{
  this->Mode = vtkSMBoundsDomain::NORMAL;
  this->DefaultMode = vtkSMBoundsDomain::MID;
  this->InputInformation = 0;
  this->ScaleFactor = 0.1;
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
    this->InvokeModified();
    return;
    }

  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    this->InvokeModified();
    }
}

//---------------------------------------------------------------------------
vtkPVDataInformation* vtkSMBoundsDomain::GetInputInformation()
{
 vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
    if (pp->GetNumberOfUncheckedProxies() > 0)
      {
      vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(
        pp->GetUncheckedProxy(0));
      if (sp)
        {
        return sp->GetDataInformation(
          (ip? ip->GetUncheckedOutputPortForConnection(0): 0));
        }
      }
    else if (pp->GetNumberOfProxies() > 0)
      {
      vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(
        pp->GetProxy(0));
      if (sp)
        {
        return sp->GetDataInformation(
          (ip? ip->GetOutputPortForConnection(0):0));
        }
      }

    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::UpdateOriented()
{
  vtkPVDataInformation* inputInformation = this->InputInformation;
  if (!this->InputInformation)
    {
    inputInformation = this->GetInputInformation();
    }
  if (!inputInformation)
    {
    return;
    }

  double bounds[6];
  inputInformation->GetBounds(bounds);

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
    if (normal->GetNumberOfUncheckedElements() > 2 && 
        origin->GetNumberOfUncheckedElements() > 2)
      {
      for (i=0; i<3; i++)
        {
        normalv[i] = normal->GetUncheckedElement(i);
        originv[i] = origin->GetUncheckedElement(i); 
        }
      }
    else if (normal->GetNumberOfElements() > 2 && 
             origin->GetNumberOfElements() > 2)
      {
      for (i=0; i<3; i++)
        {
        normalv[i] = normal->GetElement(i);
        originv[i] = origin->GetElement(i); 
        }
      }
    else
      {
      return;
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
    for (i=1; i<8; i++)
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
void vtkSMBoundsDomain::SetDomainValues(double bounds[6])
{
  if (this->Mode == vtkSMBoundsDomain::NORMAL)
    {
    for (int j = 0; j < 3; j++)
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
    // Never use 0 min/max.
    if (magn == 0)
      {
      magn = 1;
      }
    this->AddMinimum(0, -magn/2.0);
    this->AddMaximum(0,  magn/2.0);
    }
  else if (this->Mode == vtkSMBoundsDomain::SCALED_EXTENT)
    {
    double maxbounds = bounds[1] - bounds[0];
    maxbounds = (bounds[3] - bounds[2] > maxbounds) ? (bounds[3] - bounds[2]) : maxbounds;
    maxbounds = (bounds[5] - bounds[4] > maxbounds) ? (bounds[5] - bounds[4]) : maxbounds;
    maxbounds *= this->ScaleFactor;
    // Never use 0 maxbounds.
    if (maxbounds == 0)
      {
      maxbounds = this->ScaleFactor;
      }
    this->AddMinimum(0, 0);
    this->AddMaximum(0, maxbounds);
    }

}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::Update(vtkSMProxyProperty *pp)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
  unsigned int i;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        (ip? ip->GetUncheckedOutputPortForConnection(i):0));
      this->UpdateFromInformation(info);
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
        (ip? ip->GetOutputPortForConnection(i): 0));
      this->UpdateFromInformation(info);
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::UpdateFromInformation(vtkPVDataInformation* info)
{
  if (!info)
    {
    return;
    }
  double bounds[6];
  info->GetBounds(bounds);
  this->SetDomainValues(bounds);
}

//---------------------------------------------------------------------------
int vtkSMBoundsDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (!dvp)
    {
    vtkErrorMacro("vtkSMBoundsDomain only works on vtkSMDoubleVectorProperty.");
    return 0;
    }

  int status = 0;
  switch (this->Mode)
    {
  case vtkSMBoundsDomain::NORMAL:
      {
      for (unsigned int cc=0; cc < dvp->GetNumberOfElements(); cc++)
        {
        if (this->GetMaximumExists(cc) && this->GetMinimumExists(cc))
          {
          double value = 0.0;
          switch (this->DefaultMode)
            {
           case vtkSMBoundsDomain::MIN:
             value = this->GetMinimum(cc);
             break;

           case vtkSMBoundsDomain::MAX:
             value = this->GetMaximum(cc);
             break;

           case vtkSMBoundsDomain::MID:
           default:
            value = (this->GetMaximum(cc) + this->GetMinimum(cc))/2.0;
            break;
            }
          dvp->SetElement(cc, value);
          status = 1; 
          }
        }
      }
    break;
  case vtkSMBoundsDomain::SCALED_EXTENT:
      {
      for (unsigned int cc=0; cc < dvp->GetNumberOfElements(); cc++)
        {
        if (this->GetMaximumExists(cc))
          {
          dvp->SetElement(cc, this->GetMaximum(cc));
          status = 1;
          }
        }
      }
    break;

  case vtkSMBoundsDomain::MAGNITUDE:
      {
      if (this->GetMinimumExists(0) && this->GetMaximumExists(0))
        {
        double val = (this->GetMinimum(0)+this->GetMaximum(0))/2.0;
        dvp->SetElement(0, val);
        status = 1;
        }
      }
    break;

  default:
    break;
    }
  return status;
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
    else if (strcmp(mode, "scaled_extent") == 0)
      {
      this->Mode = vtkSMBoundsDomain::SCALED_EXTENT;
      }
    else
      {
      vtkErrorMacro("Unrecognized mode: " << mode);
      return 0;
      }
    }

  const char* default_mode = element->GetAttribute("default_mode");
  if (default_mode)
    {
    if (strcmp(default_mode, "min") == 0)
      {
      this->DefaultMode = vtkSMBoundsDomain::MIN;
      }
    else if (strcmp(default_mode, "max") == 0)
      {
      this->DefaultMode = vtkSMBoundsDomain::MAX;
      }
    if (strcmp(default_mode, "mid") == 0)
      {
      this->DefaultMode = vtkSMBoundsDomain::MID;
      }
    }

  const char* scalefactor = element->GetAttribute("scale_factor");
  if (scalefactor)
    {
    sscanf(scalefactor,"%lf", &this->ScaleFactor);
    }
  
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "ScaleFactor: " << this->ScaleFactor << endl;
  os << indent << "DefaultMode: " << this->DefaultMode << endl;
}
