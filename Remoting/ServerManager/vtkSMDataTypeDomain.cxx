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

#include "vtkClientServerStreamInstantiator.h"
#include "vtkDataObjectTypes.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"

#include <sstream>
#include <string>
#include <vector>

//*****************************************************************************
// Internal classes
//*****************************************************************************
struct vtkSMDataTypeDomainAllowedType
{
  int Type = -1;
  std::vector<int> ChildTypes;
  bool ChildMatchAny = false;
};

struct vtkSMDataTypeDomainInternals
{
  std::vector<vtkSMDataTypeDomainAllowedType> DataTypes;
};

//*****************************************************************************
vtkStandardNewMacro(vtkSMDataTypeDomain);
//---------------------------------------------------------------------------
vtkSMDataTypeDomain::vtkSMDataTypeDomain()
  : CompositeDataSupported(true)
  , CompositeDataRequired(false)
  , DTInternals(new vtkSMDataTypeDomainInternals())
{
}

//---------------------------------------------------------------------------
vtkSMDataTypeDomain::~vtkSMDataTypeDomain()
{
  delete this->DTInternals;
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
    for (unsigned int i = 0; i < numProxs; i++)
    {
      vtkSMProxy* proxy = pp->GetUncheckedProxy(i);
      int portno = ip ? ip->GetUncheckedOutputPortForConnection(i) : 0;
      if (!this->IsInDomain(vtkSMSourceProxy::SafeDownCast(proxy), portno))
      {
        return 0;
      }
    }
    return 1;
  }

  return 0;
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::IsInDomain(vtkSMSourceProxy* proxy, int outputport /*=0*/)
{
  if (!proxy)
  {
    return 0;
  }

  if (this->DTInternals->DataTypes.size() == 0)
  {
    return 1;
  }

  // Make sure the outputs are created.
  proxy->CreateOutputPorts();

  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info || info->IsNull())
  {
    return 0;
  }

  if (info->IsCompositeDataSet() && !this->CompositeDataSupported)
  {
    return 0;
  }
  if (!info->IsCompositeDataSet() && this->CompositeDataRequired)
  {
    return 0;
  }

  for (auto& dtype : this->DTInternals->DataTypes)
  {
    if (info->DataSetTypeIsA(dtype.Type))
    {
      if (dtype.ChildTypes.size() == 0)
      {
        return 1;
      }

      size_t match_count = 0;
      for (auto& childType : dtype.ChildTypes)
      {
        match_count += info->HasDataSetType(childType) ? 1 : 0;
      }

      if (dtype.ChildMatchAny && match_count > 0)
      {
        return 1;
      }
      else if (!dtype.ChildMatchAny && match_count == dtype.ChildTypes.size())
      {
        return 1;
      }
    }
  }

  return 0;
}

//---------------------------------------------------------------------------
std::string vtkSMDataTypeDomain::GetDomainDescription() const
{
  auto& types = this->DTInternals->DataTypes;
  if (types.size() == 0)
  {
    return std::string{};
  }

  std::vector<std::string> phrases;
  for (auto& typeInfo : types)
  {
    if (typeInfo.ChildTypes.size() == 0)
    {
      phrases.push_back(vtkDataObjectTypes::GetClassNameFromTypeId(typeInfo.Type));
    }
    else
    {
      std::ostringstream subPhrase;
      subPhrase << vtkDataObjectTypes::GetClassNameFromTypeId(typeInfo.Type);
      subPhrase << " containing";
      for (size_t cc = 0; cc < typeInfo.ChildTypes.size(); ++cc)
      {
        if (cc > 0)
        {
          subPhrase << ",";
          if (cc + 1 == typeInfo.ChildTypes.size())
          {
            subPhrase << (typeInfo.ChildMatchAny ? " or" : " and");
          }
        }
        subPhrase << " " << vtkDataObjectTypes::GetClassNameFromTypeId(typeInfo.ChildTypes[cc]);
      }
      phrases.push_back(subPhrase.str());
    }
  }

  std::ostringstream stream;
  stream << "Input must be ";
  for (size_t cc = 0; cc < phrases.size(); ++cc)
  {
    if (cc > 0)
    {
      stream << ", ";
      if (cc + 1 == phrases.size())
      {
        stream << "or ";
      }
    }
    stream << phrases[cc].c_str();
  }
  return stream.str();
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  int compositeDataSupported;
  if (element->GetScalarAttribute("composite_data_supported", &compositeDataSupported))
  {
    this->CompositeDataSupported = (compositeDataSupported == 1);
  }

  int compositeDataRequired;
  if (element->GetScalarAttribute("composite_data_required", &compositeDataRequired))
  {
    this->CompositeDataRequired = (compositeDataRequired == 1);
  }

  // Loop over the top-level elements.
  for (unsigned int i = 0; i < element->GetNumberOfNestedElements(); ++i)
  {
    vtkPVXMLElement* selement = element->GetNestedElement(i);
    if (strcmp("DataType", selement->GetName()) != 0)
    {
      continue;
    }
    const char* value = selement->GetAttribute("value");
    if (!value)
    {
      vtkErrorMacro(<< "Can not find required attribute 'value'");
      return 0;
    }

    const int typeId = vtkDataObjectTypes::GetTypeIdFromClassName(value);
    if (typeId == -1)
    {
      vtkErrorMacro("'" << value << "' not a known data object type!");
      return 0;
    }

    vtkSMDataTypeDomainAllowedType typeInfo;
    typeInfo.Type = typeId;

    const char* childMatch = selement->GetAttributeOrDefault("child_match", "any");
    if (strcmp(childMatch, "any") != 0 && strcmp(childMatch, "all") != 0)
    {
      vtkErrorMacro("'child_match' can only be 'any' or 'all', got '" << childMatch << "'.");
      return 0;
    }
    typeInfo.ChildMatchAny = (strcmp(childMatch, "any") == 0);
    for (unsigned int cc = 0; cc < selement->GetNumberOfNestedElements(); ++cc)
    {
      vtkPVXMLElement* childElement = selement->GetNestedElement(cc);
      if (strcmp("DataType", childElement->GetName()) != 0)
      {
        continue;
      }
      const char* childValue = childElement->GetAttribute("value");
      if (!childValue)
      {
        vtkErrorMacro("Missing required attribute 'value'. ");
        return 0;
      }

      const auto childTypeId = vtkDataObjectTypes::GetTypeIdFromClassName(childValue);
      if (childTypeId == -1)
      {
        vtkErrorMacro("'" << childValue << "' not a known data object type!");
        return 0;
      }

      typeInfo.ChildTypes.push_back(childTypeId);
    }

    this->DTInternals->DataTypes.push_back(typeInfo);
  }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDataTypeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
