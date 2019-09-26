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
#include "vtkPVDataInformation.h"
#include "vtkPVInstantiator.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"

#include <map>
#include <string>
#include <vector>

//*****************************************************************************
// Internal classes
//*****************************************************************************
struct vtkSMDataTypeDomainInternals
{
  std::vector<vtkStdString> DataTypes;
};
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

  vtkObject* object = vtkPVInstantiator::CreateInstance(classname);
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

  // hypertree grid's dataset API is somewhat broken.
  // prevent it from going into filters that have not been
  // whitelisted.
  if (strcmp(info->GetDataClassName(), "vtkHyperTreeGrid") == 0)
  {
    for (unsigned int i = 0; i < numTypes; i++)
    {
      if (strcmp(this->GetDataType(i), "vtkHyperTreeGrid") == 0 ||
        strcmp(this->GetDataType(i), "vtkMultiBlockDataSet") == 0)
      {
        return 1;
      }
    }
    return 0;
  }

  for (unsigned int i = 0; i < numTypes; i++)
  {
    // Unfortunately, vtkDataSet, vtkPointSet, and vtkUnstructuredGridBase have
    // to be handled specially. These classes are abstract and can not be
    // instantiated.
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
      if ((strcmp(this->GetDataType(i), "vtkPointSet") == 0) ||
        (strcmp(this->GetDataType(i), "vtkDataSet") == 0))
      {
        return 1;
      }
      else
      {
        continue;
      }
    }
    if (strcmp(info->GetDataClassName(), "vtkUnstructuredGridBase") == 0)
    {
      if ((strcmp(this->GetDataType(i), "vtkPointSet") == 0) ||
        (strcmp(this->GetDataType(i), "vtkDataSet") == 0) ||
        (strcmp(this->GetDataType(i), "vtkUnstructuredGridBase") == 0))
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
      vtkSMDataTypeDomainCache::GetDataObjectOfType(info->GetCompositeDataClassName());
    for (unsigned int i = 0; i < numTypes; i++)
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

    this->DTInternals->DataTypes.push_back(value);
  }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDataTypeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
