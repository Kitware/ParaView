/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataTypeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDataTypeDomain.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkStdString.h"

#include <vtkstd/vector>


vtkStandardNewMacro(vtkSMDataTypeDomain);

struct vtkSMDataTypeDomainInternals
{
  vtkstd::vector<vtkStdString> DataTypes;
};

//---------------------------------------------------------------------------
vtkSMDataTypeDomain::vtkSMDataTypeDomain()
{
  this->DTInternals = new vtkSMDataTypeDomainInternals;
  this->CompositeDataSupported = 1;
}

//---------------------------------------------------------------------------
vtkSMDataTypeDomain::~vtkSMDataTypeDomain()
{
  delete this->DTInternals;
}

//---------------------------------------------------------------------------
unsigned int vtkSMDataTypeDomain::GetNumberOfDataTypes()
{
  return this->DTInternals->DataTypes.size();
}

//---------------------------------------------------------------------------
const char* vtkSMDataTypeDomain::GetDataType(unsigned int idx)
{
  return this->DTInternals->DataTypes[idx].c_str();
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
    {
    return 1;
    }

  if (!property)
    {
    return 0;
    }

  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(property);
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);
  if (pp)
    {
    unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
    for (unsigned int i=0; i<numProxs; i++)
      {
      vtkSMProxy* proxy = pp->GetUncheckedProxy(i);
      int portno = ip? ip->GetUncheckedOutputPortForConnection(i) : 0;
      if (!this->IsInDomain( 
            vtkSMSourceProxy::SafeDownCast(proxy), portno ) )
        {
        return 0;
        }
      }
    return 1;
    }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::IsInDomain(vtkSMSourceProxy* proxy, 
  int outputport/*=0*/)
{
  if (!proxy)
    {
    return 0;
    }

  unsigned int numTypes = this->GetNumberOfDataTypes();
  if (numTypes == 0)
    {
    return 1;
    }

  // Make sure the outputs are created.
  proxy->CreateOutputPorts();
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info)
    {
    return 0;
    }

  if (info->GetCompositeDataClassName() && !this->CompositeDataSupported)
    {
    return 0;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    return 0;
    }

  // Get an actual instance of the same type as the data represented
  // by the information object. This is later used to check match
  // with IsA. See the vtkProcessModule for more information.
  vtkDataObject* dobj =  pm->GetDataObjectOfType(info->GetDataClassName());
  if (!dobj)
    {
    return 0;
    }

  for (unsigned int i=0; i<numTypes; i++)
    {
    // Unfortunately, vtkDataSet and vtkPointSet have to be handled
    // specially. These classes are abstract and can not be instantiated.
    if (strcmp(info->GetDataClassName(), "vtkDataSet") == 0)
      {
      if (strcmp(this->GetDataType(i), "vtkDataSet") == 0)
        {
        return 1;
        }
      else
        {
        continue;
        }
      }
    if (strcmp(info->GetDataClassName(), "vtkPointSet") == 0)
      {
      if ( (strcmp(this->GetDataType(i), "vtkPointSet") == 0) ||
           (strcmp(this->GetDataType(i), "vtkDataSet") == 0))
        {
        return 1;
        }
      else
        {
        continue;
        }
      }
    if (dobj->IsA(this->GetDataType(i)))
      {
      return 1;
      }
    }

  if (info->GetCompositeDataClassName())
    {
    vtkDataObject* cDobj =  
      pm->GetDataObjectOfType(info->GetCompositeDataClassName());
    for (unsigned int i=0; i<numTypes; i++)
      {
      if (cDobj->IsA(this->GetDataType(i)))
        {
        return 1;
        }
      }
    }

  return 0;
}

//---------------------------------------------------------------------------
void vtkSMDataTypeDomain::ChildSaveState(vtkPVXMLElement* domainElement)
{
  this->Superclass::ChildSaveState(domainElement);

  unsigned int size = this->GetNumberOfDataTypes();
  for(unsigned int i=0; i<size; i++)
    {
    vtkPVXMLElement* dataTypeElem = vtkPVXMLElement::New();
    dataTypeElem->SetName("DataType");
    dataTypeElem->AddAttribute("value", this->GetDataType(i));
    domainElement->AddNestedElement(dataTypeElem);
    dataTypeElem->Delete();
    }

}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  int compositeDataSupported;
  if (element->GetScalarAttribute("composite_data_supported",
                                  &compositeDataSupported))
    {
    this->SetCompositeDataSupported(compositeDataSupported);
    }

  // Loop over the top-level elements.
  unsigned int i;
  for(i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* selement = element->GetNestedElement(i);
    if ( strcmp("DataType", selement->GetName()) != 0 )
      {
      continue;
      }
    const char* value = selement->GetAttribute("value");
    if (!value)
      {
      vtkErrorMacro(<< "Can not find required attribute: value. "
                    << "Can not parse domain xml.");
      return 0;
      }

    this->DTInternals->DataTypes.push_back(value);
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDataTypeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}
