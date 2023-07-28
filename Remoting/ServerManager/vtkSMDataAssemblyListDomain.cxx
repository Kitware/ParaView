// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMDataAssemblyListDomain.h"

#include "vtkDataAssembly.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMStringVectorProperty.h"

#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMDataAssemblyListDomain);
//----------------------------------------------------------------------------
vtkSMDataAssemblyListDomain::vtkSMDataAssemblyListDomain() = default;
//----------------------------------------------------------------------------
vtkSMDataAssemblyListDomain::~vtkSMDataAssemblyListDomain() = default;

//----------------------------------------------------------------------------
void vtkSMDataAssemblyListDomain::Update(vtkSMProperty*)
{
  auto dinfo = this->GetInputDataInformation("Input");
  if (!dinfo)
  {
    this->SetStrings({});
    return;
  }

  std::vector<std::string> strings;
  // when we start supporting multiple named assemblies, this will need to be
  // updated.
  if (dinfo->GetHierarchy())
  {
    strings.emplace_back("Hierarchy");
  }
  if (auto dataAssembly = dinfo->GetDataAssembly())
  {
    const auto hasChildren = dataAssembly->GetNumberOfChildren(dataAssembly->GetRootNode()) > 0;
    if (hasChildren)
    {
      strings.emplace_back("Assembly");
    }
  }
  this->SetStrings(strings);
}

//----------------------------------------------------------------------------
int vtkSMDataAssemblyListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  auto stringVectorProp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!stringVectorProp)
  {
    return 0;
  }
  vtkSMPropertyHelper helper(stringVectorProp);
  helper.SetUseUnchecked(use_unchecked_values);
  const auto& strings = this->GetStrings();
  if (!this->BackwardCompatibilityMode &&
    std::find(strings.begin(), strings.end(), "Assembly") != strings.end())
  {
    helper.Set(0, "Assembly");
    return 1;
  }
  else if (std::find(strings.begin(), strings.end(), "Hierarchy") != strings.end())
  {
    helper.Set(0, "Hierarchy");
    return 1;
  }
  else
  {
    // empty string.
    helper.Set(0, "");
    return 1;
  }
}

//----------------------------------------------------------------------------
void vtkSMDataAssemblyListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
