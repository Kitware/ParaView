// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMDataAssemblyDomain.h"

#include "vtkDataAssembly.h"
#if VTK_MODULE_ENABLE_VTK_IOIOSS
#include "vtkIOSSReader.h"
#endif
#include "vtkObjectFactory.h"
#include "vtkPVDataAssemblyInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <sstream>

vtkStandardNewMacro(vtkSMDataAssemblyDomain);
//----------------------------------------------------------------------------
vtkSMDataAssemblyDomain::vtkSMDataAssemblyDomain() = default;

//----------------------------------------------------------------------------
vtkSMDataAssemblyDomain::~vtkSMDataAssemblyDomain() = default;

//=============================================================================
namespace
{
template <typename T>
T lexical_cast(const std::string& s)
{
  std::stringstream ss(s);

  T result;
  if ((ss >> result).fail() || !(ss >> std::ws).eof())
  {
    throw std::bad_cast();
  }

  return result;
}
}

//-----------------------------------------------------------------------------
int vtkSMDataAssemblyDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* entity_type_string = element->GetAttribute("entity_type");
  if (entity_type_string)
  {
    try
    {
      this->EntityType = lexical_cast<int>(entity_type_string);
    }
    catch (const std::bad_cast&)
    {
      vtkErrorMacro("Invalid entity_type_string attribute: " << entity_type_string);
      return 0;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::Update(vtkSMProperty*)
{
  if (auto tagProperty = this->GetRequiredProperty("Tag"))
  {
    // we're in "reader" mode.
    this->FetchAssembly(vtkSMPropertyHelper(tagProperty).GetAsInt());
  }
  else
  {
    // we're in "filter" mode.
    auto dinfo = this->GetInputDataInformation("Input");
    if (!dinfo)
    {
      this->ChooseAssembly({}, nullptr);
    }
    else
    {
      auto activeAssembly = this->GetRequiredProperty("ActiveAssembly");
      if (!activeAssembly)
      {
        this->ChooseAssembly("Hierarchy", dinfo->GetHierarchy());
      }
      else
      {
        const std::string name{ vtkSMUncheckedPropertyHelper(activeAssembly).GetAsString(0) };
        if (name == "Hierarchy")
        {
          this->ChooseAssembly(name, dinfo->GetHierarchy());
        }
        else if (name == "Assembly")
        {
          this->ChooseAssembly(name, dinfo->GetDataAssembly());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkDataAssembly* vtkSMDataAssemblyDomain::GetDataAssembly() const
{
  return this->Assembly;
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::ChooseAssembly(const std::string& name, vtkDataAssembly* assembly)
{
  const std::string assemblyXMLContents = assembly ? assembly->SerializeToXML(vtkIndent()) : "";
  if (this->Name != name || assembly != this->Assembly ||
    (assembly != nullptr && this->AssemblyXMLContents != assemblyXMLContents))
  {
    this->Name = name;
    this->Assembly = assembly;
    this->AssemblyXMLContents = assemblyXMLContents;
    this->DomainModified();
  }
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::FetchAssembly(int tag)
{
  if (tag == 0)
  {
    this->LastTag = 0;
    this->ChooseAssembly({}, nullptr);
  }
  else if (tag != this->LastTag)
  {
    this->LastTag = tag;

    vtkNew<vtkPVDataAssemblyInformation> info;
    info->SetMethodName("GetAssembly"); // todo: make this configurable.
    this->GetProperty()->GetParent()->GatherInformation(info);
    this->ChooseAssembly("Assembly", info->GetDataAssembly());
  }
}

//----------------------------------------------------------------------------
int vtkSMDataAssemblyDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  if (!prop)
  {
    return 0;
  }
#if VTK_MODULE_ENABLE_VTK_IOIOSS
  vtkSMPropertyHelper helper(prop);
  helper.SetUseUnchecked(use_unchecked_values);
  if (this->Assembly && this->EntityType >= 0)
  {
    std::string path;
    if (this->EntityType < vtkIOSSReader::EntityType::NUMBER_OF_ENTITY_TYPES)
    {
      path = std::string("/IOSS/") +
        vtkIOSSReader::GetDataAssemblyNodeNameForEntityType(this->EntityType);
      const int idx = this->Assembly->GetFirstNodeByPath(path.c_str());
      if (idx != -1)
      {
        helper.Set(0, path.c_str());
        return 1;
      }
      // if it's element block, and we couldn't find it, then all blocks will be element blocks.
      else if (this->EntityType == vtkIOSSReader::EntityType::ELEMENTBLOCK)
      {
        helper.Set(0, "/");
        return 1;
      }
    }
  }
#endif
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "LastTag: " << this->LastTag << endl;
  os << indent << "Name: " << this->Name << endl;
  if (this->Assembly)
  {
    this->Assembly->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Assembly: (none)" << endl;
  }
  os << indent << "EntityType: " << this->EntityType << endl;
}
