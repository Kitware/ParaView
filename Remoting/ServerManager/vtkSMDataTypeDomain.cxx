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
#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVCompositeDataInformation.h"
#include "vtkPVCompositeDataInformationIterator.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"

#include <map>
#include <string>
#include <vector>

//*****************************************************************************
// Internal classes
//*****************************************************************************
struct vtkSMDataTypeDomainAllowedType
{
  vtkSMDataTypeDomainAllowedType() = delete;
  vtkSMDataTypeDomainAllowedType(std::string Name, bool HasChildren = false, bool MatchAny = false,
    std::vector<std::string> Children = {});
  std::string typeName;
  bool hasChildren;
  bool childMatchAny;
  std::vector<std::string> childAllowedTypes;
};

struct vtkSMDataTypeDomainInternals
{
  std::vector<vtkSMDataTypeDomainAllowedType> DataTypes;
};

vtkSMDataTypeDomainAllowedType::vtkSMDataTypeDomainAllowedType(
  std::string Name, bool HasChildren, bool MatchAny, std::vector<std::string> Children)
  : typeName(Name)
  , hasChildren(HasChildren)
  , childMatchAny(MatchAny)
  , childAllowedTypes(Children){};
//*****************************************************************************
namespace vtkSMDataTypeDomainCache
{
static std::map<std::string, vtkSmartPointer<vtkDataObject> > DataObjectMap;

// Only instantiate classes once and use cache after...
static vtkDataObject* GetDataObjectOfType(const char* classname)
{
  if (classname == NULL)
  {
    return 0;
  }

  // Since we can not instantiate these classes, we'll replace
  // them with a subclass
  if (strcmp(classname, "vtkDataSet") == 0)
  {
    classname = "vtkImageData";
  }
  else if (strcmp(classname, "vtkPointSet") == 0)
  {
    classname = "vtkPolyData";
  }
  else if (strcmp(classname, "vtkCompositeDataSet") == 0)
  {
    classname = "vtkHierarchicalDataSet";
  }
  else if (strcmp(classname, "vtkUnstructuredGridBase") == 0)
  {
    classname = "vtkUnstructuredGrid";
  }

  std::map<std::string, vtkSmartPointer<vtkDataObject> >::iterator it;
  it = DataObjectMap.find(classname);
  if (it != DataObjectMap.end())
  {
    return it->second.GetPointer();
  }

  auto object = vtkClientServerStreamInstantiator::CreateInstance(classname);
  vtkDataObject* dobj = vtkDataObject::SafeDownCast(object);
  if (!dobj)
  {
    if (object)
    {
      object->Delete();
    }
    return 0;
  }

  DataObjectMap[classname] = dobj;
  dobj->Delete();
  return dobj;
}
}

//*****************************************************************************
vtkStandardNewMacro(vtkSMDataTypeDomain);
//---------------------------------------------------------------------------
vtkSMDataTypeDomain::vtkSMDataTypeDomain()
{
  this->DTInternals = new vtkSMDataTypeDomainInternals;
  this->CompositeDataSupported = 1;
  this->CompositeDataRequired = 0;
}

//---------------------------------------------------------------------------
vtkSMDataTypeDomain::~vtkSMDataTypeDomain()
{
  delete this->DTInternals;
}

//---------------------------------------------------------------------------
unsigned int vtkSMDataTypeDomain::GetNumberOfDataTypes()
{
  return static_cast<unsigned int>(this->DTInternals->DataTypes.size());
}

//---------------------------------------------------------------------------
const char* vtkSMDataTypeDomain::GetDataTypeName(unsigned int idx)
{
  return this->DTInternals->DataTypes[idx].typeName.c_str();
}

//---------------------------------------------------------------------------
bool vtkSMDataTypeDomain::DataTypeHasChildren(unsigned int idx)
{
  return this->DTInternals->DataTypes[idx].hasChildren;
}

//---------------------------------------------------------------------------
const char* vtkSMDataTypeDomain::GetDataTypeChildMatchTypeAsString(unsigned int idx)
{
  return this->DTInternals->DataTypes[idx].childMatchAny ? "any" : "all";
}

//---------------------------------------------------------------------------
const std::vector<std::string>& vtkSMDataTypeDomain::GetDataTypeChildren(unsigned int idx)
{
  return this->DTInternals->DataTypes[idx].childAllowedTypes;
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
  if (!info->GetCompositeDataClassName() && this->CompositeDataRequired)
  {
    return 0;
  }

  // Get an actual instance of the same type as the data represented
  // by the information object. This is later used to check match
  // with IsA.
  vtkDataObject* dobj = vtkSMDataTypeDomainCache::GetDataObjectOfType(info->GetDataClassName());
  if (!dobj)
  {
    const char* classname = info->GetDataClassName();
    if (classname && classname[0] != '\0')
    {
      vtkWarningMacro("Unable to create instance of class '" << classname << "'.");
    }
    return 0;
  }

  for (unsigned int i = 0; i < numTypes; i++)
  {
    if (this->DataTypeHasChildren(i))
    {
      continue;
    }
    // Unfortunately, vtkDataSet, vtkPointSet, and vtkUnstructuredGridBase have
    // to be handled specially. These classes are abstract and can not be
    // instantiated.
    if (strcmp(info->GetDataClassName(), "vtkDataSet") == 0)
    {
      if (strcmp(this->GetDataTypeName(i), "vtkDataSet") == 0)
      {
        return 1;
      }
      continue;
    }
    if (strcmp(info->GetDataClassName(), "vtkPointSet") == 0)
    {
      if ((strcmp(this->GetDataTypeName(i), "vtkPointSet") == 0) ||
        (strcmp(this->GetDataTypeName(i), "vtkDataSet") == 0))
      {
        return 1;
      }
      continue;
    }
    if (strcmp(info->GetDataClassName(), "vtkUnstructuredGridBase") == 0)
    {
      if ((strcmp(this->GetDataTypeName(i), "vtkPointSet") == 0) ||
        (strcmp(this->GetDataTypeName(i), "vtkDataSet") == 0) ||
        (strcmp(this->GetDataTypeName(i), "vtkUnstructuredGridBase") == 0))
      {
        return 1;
      }
      continue;
    }
    if (dobj->IsA(this->GetDataTypeName(i)))
    {
      return 1;
    }
  }

  if (info->GetCompositeDataClassName())
  {
    vtkDataObject* cDobj =
      vtkSMDataTypeDomainCache::GetDataObjectOfType(info->GetCompositeDataClassName());
    // Composite data with specified child
    for (unsigned int i = 0; i < numTypes; i++)
    {
      if (cDobj->IsA(this->GetDataTypeName(i)))
      {
        if (!this->DataTypeHasChildren(i))
        {
          return 1;
        }
        // Check child types
        bool childMatchAny =
          strcmp(this->GetDataTypeChildMatchTypeAsString(i), "any") == 0 ? true : false;
        bool allChildrenMatched = true;
        auto& dataTypes = this->GetDataTypeChildren(i);
        vtkNew<vtkPVCompositeDataInformationIterator> iter;
        iter->SetDataInformation(info);
        for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
        {
          vtkPVDataInformation* childInfo = iter->GetCurrentDataInformation();
          if (!childInfo)
          {
            continue;
          }
          // Skip non-leaf children
          vtkPVCompositeDataInformation* compositeInfo = childInfo->GetCompositeDataInformation();
          if (compositeInfo->GetDataIsComposite() && !compositeInfo->GetDataIsMultiPiece())
          {
            continue;
          }
          vtkDataObject* childObj =
            vtkSMDataTypeDomainCache::GetDataObjectOfType(childInfo->GetDataClassName());
          bool childMatched = false;
          for (auto& type : dataTypes)
          {
            if (childObj->IsA(type.c_str()))
            {
              childMatched = true;
              break;
            }
          }
          if (childMatched && childMatchAny)
          {
            return 1;
          }
          else if (!childMatched)
          {
            allChildrenMatched = false;
          }
        }
        if (!childMatchAny && info->GetNumberOfBlockLeafs(true) && allChildrenMatched)
        {
          return 1;
        }
      }
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMDataTypeDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  int compositeDataSupported;
  if (element->GetScalarAttribute("composite_data_supported", &compositeDataSupported))
  {
    this->SetCompositeDataSupported(compositeDataSupported);
  }

  int compositeDataRequired;
  if (element->GetScalarAttribute("composite_data_required", &compositeDataRequired))
  {
    this->SetCompositeDataRequired(compositeDataRequired);
  }

  // Loop over the top-level elements.
  unsigned int i;
  for (i = 0; i < element->GetNumberOfNestedElements(); ++i)
  {
    vtkPVXMLElement* selement = element->GetNestedElement(i);
    if (strcmp("DataType", selement->GetName()) != 0)
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
    // For composite data with specified child data type
    const char* childMatch = selement->GetAttribute("child_match");
    if (childMatch)
    {
      bool compositeChildMatchAny = true;
      if (strcmp("any", childMatch) == 0)
      {
        compositeChildMatchAny = true;
      }
      else if (strcmp("all", childMatch) == 0)
      {
        compositeChildMatchAny = false;
      }
      else
      {
        vtkErrorMacro("The value of child_match should be \"any\" or \"all\"."
          << "Can not parse domain xml.");
        return 0;
      }

      if (!selement->GetNumberOfNestedElements())
      {
        vtkErrorMacro(<< "child_match set but no nested datatypes."
                      << "Can not parse domain xml.");
        return 0;
      }
      unsigned int j;
      std::vector<std::string> childValues;
      for (j = 0; j < selement->GetNumberOfNestedElements(); ++j)
      {
        vtkPVXMLElement* childElement = selement->GetNestedElement(j);
        if (strcmp("DataType", childElement->GetName()) != 0)
        {
          continue;
        }
        const char* childValue = childElement->GetAttribute("value");
        if (!childValue)
        {
          vtkErrorMacro(<< "Can not find required attribute: value. "
                        << "Can not parse domain xml.");
          return 0;
        }
        childValues.push_back(childValue);
      }
      this->DTInternals->DataTypes.emplace_back(value, true, compositeChildMatchAny, childValues);
    }
    else if (selement->GetNumberOfNestedElements())
    {
      vtkErrorMacro(<< "Child type set but child_match not specified."
                    << "Can not parse domain xml.");
      return 0;
    }
    else
    {
      this->DTInternals->DataTypes.emplace_back(value);
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDataTypeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
