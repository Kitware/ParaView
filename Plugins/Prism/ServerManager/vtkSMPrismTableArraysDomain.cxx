// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMPrismTableArraysDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <sstream>

//=============================================================================
vtkStandardNewMacro(vtkSMPrismTableArraysDomain);

//=============================================================================
vtkSMPrismTableArraysDomain::vtkSMPrismTableArraysDomain() = default;

//=============================================================================
vtkSMPrismTableArraysDomain::~vtkSMPrismTableArraysDomain() = default;

//=============================================================================
void vtkSMPrismTableArraysDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DefaultArrayId: " << this->DefaultArrayId << endl;
}

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
int vtkSMPrismTableArraysDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* default_array_id = element->GetAttribute("default_array_id");
  if (default_array_id)
  {
    try
    {
      this->DefaultArrayId = lexical_cast<int>(default_array_id);
    }
    catch (const std::bad_cast&)
    {
      vtkErrorMacro("Invalid default_array_id attribute: " << default_array_id);
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSMPrismTableArraysDomain::Update(vtkSMProperty*)
{
  auto flatArraysOfTablesProp = this->GetRequiredProperty("FlatArraysOfTables");
  auto tableIdProp = this->GetRequiredProperty("TableId");

  if (!flatArraysOfTablesProp || !tableIdProp)
  {
    vtkErrorMacro("Missing required properties.");
    return;
  }

  vtkSMUncheckedPropertyHelper flatArraysOfTablesHelper(flatArraysOfTablesProp);
  vtkSMUncheckedPropertyHelper tableIdHelper(tableIdProp);

  const int tableId = tableIdHelper.GetAsInt();

  // convert the flat arrays of tables to a map of table id to arrays
  std::map<int, std::vector<std::string>> arraysOfTables;
  int currentTableId = -1;
  for (unsigned int i = 0; i < flatArraysOfTablesHelper.GetNumberOfElements(); ++i)
  {
    const auto str = flatArraysOfTablesHelper.GetAsString(i);
    try
    {
      int value = lexical_cast<int>(str);
      currentTableId = value;
    }
    catch (const std::bad_cast&)
    {
      if (currentTableId != -1)
      {
        arraysOfTables[currentTableId].push_back(str);
      }
    }
  }

  if (arraysOfTables.find(tableId) != arraysOfTables.end())
  {
    this->SetStrings(arraysOfTables[tableId]);
  }
  else
  {
    this->SetStrings({});
  }
}

//-----------------------------------------------------------------------------
int vtkSMPrismTableArraysDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
  {
    return 0;
  }

  const auto& strings = this->GetStrings();
  if (this->DefaultArrayId >= 0 && this->DefaultArrayId < static_cast<int>(strings.size()))
  {
    svp->SetElement(0, strings[this->DefaultArrayId].c_str());
    return 1;
  }
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}
