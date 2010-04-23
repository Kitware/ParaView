/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayListInformationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMDomainIterator.h"

vtkStandardNewMacro(vtkSMArrayListInformationHelper);
//-----------------------------------------------------------------------------
vtkSMArrayListInformationHelper::vtkSMArrayListInformationHelper()
{
  this->ListDomainName = 0;
}

//-----------------------------------------------------------------------------
vtkSMArrayListInformationHelper::~vtkSMArrayListInformationHelper()
{
  this->SetListDomainName(0);
}

//-----------------------------------------------------------------------------
void vtkSMArrayListInformationHelper::UpdateProperty(
  vtkIdType vtkNotUsed(connectionId),  int vtkNotUsed(serverIds), 
  vtkClientServerID vtkNotUsed(objectId), 
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMStringVectorProperty was needed.");
    return;
    }

  vtkSMArrayListDomain* ild = 0;
  if (this->ListDomainName)
    {
    ild = vtkSMArrayListDomain::SafeDownCast(
      prop->GetDomain(this->ListDomainName));
    }
  else
    {
    vtkSMDomainIterator* di = prop->NewDomainIterator();
    di->Begin();
    while (!di->IsAtEnd())
      {
      ild = vtkSMArrayListDomain::SafeDownCast(di->GetDomain());
      if (ild)
        {
        break;
        }
      di->Next();
      }
    di->Delete();
    }

  if (ild)
    {
    unsigned int num_string = ild->GetNumberOfStrings();
    svp->SetNumberOfElements(num_string);
    for (unsigned int cc=0; cc < num_string; cc++)
      {
      svp->SetElement(cc, ild->GetString(cc));
      }
    }
}

//-----------------------------------------------------------------------------
int vtkSMArrayListInformationHelper::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }
  const char* list_domain_name = element->GetAttribute("list_domain_name");
  if (list_domain_name)
    {
    this->SetListDomainName(list_domain_name);
    }
  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMArrayListInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ListDomainName: " << 
    (this->ListDomainName? this->ListDomainName : "(none)") << endl;
}
