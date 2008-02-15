/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompositeTreeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompositeTreeDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMCompositeTreeDomain);
vtkCxxRevisionMacro(vtkSMCompositeTreeDomain, "1.1");
vtkCxxSetObjectMacro(vtkSMCompositeTreeDomain, Information, vtkPVDataInformation);
//----------------------------------------------------------------------------
vtkSMCompositeTreeDomain::vtkSMCompositeTreeDomain()
{
  this->Information = 0;
  this->LastInformation = 0;
  this->Mode = ALL;
}

//----------------------------------------------------------------------------
vtkSMCompositeTreeDomain::~vtkSMCompositeTreeDomain()
{
  this->SetInformation(0);
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::Update(vtkSMProperty*)
{
  this->SetInformation(0);

  vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    }
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::InvokeModifiedIfChanged()
{
  if (this->Information != this->LastInformation ||
    (this->Information && this->UpdateTime < this->Information->GetMTime()))
    {
    this->LastInformation = this->Information;
    this->UpdateTime.Modified();
    this->InvokeModified();
    }
}

//---------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::Update(vtkSMInputProperty *ip)
{
  unsigned int i;
  unsigned int numProxs = ip->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
    {
    vtkSMSourceProxy* sp =
      vtkSMSourceProxy::SafeDownCast(ip->GetUncheckedProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        ip->GetUncheckedOutputPortForConnection(i));
      if (!info)
        {
        continue;
        }
      this->SetInformation(info);
      this->InvokeModifiedIfChanged();
      return;
      }
    }

  // In case there is no valid unchecked proxy, use the actual
  // proxy values
  numProxs = ip->GetNumberOfProxies();
  for (i=0; i<numProxs; i++)
    {
    vtkSMSourceProxy* sp = 
      vtkSMSourceProxy::SafeDownCast(ip->GetProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation(
        ip->GetOutputPortForConnection(i));
      if (!info)
        {
        continue;
        }
      this->SetInformation(info);
      this->InvokeModifiedIfChanged();
      return;
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMCompositeTreeDomain::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  this->Mode = ALL;
  const char* mode = element->GetAttribute("mode");
  if (mode)
    {
    if (strcmp(mode, "all") == 0)
      {
      this->Mode = ALL;
      }
    else if (strcmp(mode, "leaves") == 0)
      {
      this->Mode = LEAVES;
      }
    else if (strcmp(mode, "non-leaves") == 0)
      {
      this->Mode = NON_LEAVES;
      }
    else
      {
      vtkErrorMacro("Unrecognized mode: " << mode);
      return 0;
      }
    }
  return 1;
}
//----------------------------------------------------------------------------
void vtkSMCompositeTreeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Information: " << this->Information << endl;
  os << indent << "Mode: ";
  switch (this->Mode)
    {
  case ALL:
    os << "ALL";
    break;
  case LEAVES:
    os << "LEAVES";
    break;
  case NON_LEAVES:
    os << "NON_LEAVES";
    break;
  default:
    os << "UNKNOWN";
    }
  os << endl;
}


