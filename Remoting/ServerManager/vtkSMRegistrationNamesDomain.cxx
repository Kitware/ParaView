// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMRegistrationNamesDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <set>
#include <string>

class vtkSMRegistrationNamesDomain::vtkInternals
{
public:
  std::set<std::string> RegistrationGroups;
};

vtkStandardNewMacro(vtkSMRegistrationNamesDomain);
//----------------------------------------------------------------------------
vtkSMRegistrationNamesDomain::vtkSMRegistrationNamesDomain()
  : Internals(new vtkSMRegistrationNamesDomain::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMRegistrationNamesDomain::~vtkSMRegistrationNamesDomain() = default;

//----------------------------------------------------------------------------
int vtkSMRegistrationNamesDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  auto& internals = (*this->Internals);
  internals.RegistrationGroups.clear();
  if (auto registrationGroup = element->GetAttribute("registration_group"))
  {
    internals.RegistrationGroups.insert(registrationGroup);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSMRegistrationNamesDomain::SetDefaultValues(
  vtkSMProperty* property, bool use_unchecked_values)
{
  const auto& internals = (*this->Internals);
  auto pxm = this->GetSessionProxyManager();

  std::vector<std::string> values;
  if (auto proxiesProperty = this->GetRequiredProperty("Proxies"))
  {
    vtkSMUncheckedPropertyHelper helper(proxiesProperty);
    for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
    {
      auto proxy = helper.GetAsProxy(cc);
      if (proxy == nullptr)
      {
        values.push_back("None");
      }
      else
      {
        for (const auto& group : internals.RegistrationGroups)
        {
          if (auto name = pxm->GetProxyName(group.c_str(), proxy))
          {
            values.push_back(name);
            break;
          }
        }
      }
    }
  }

  if (auto svp = vtkSMStringVectorProperty::SafeDownCast(property))
  {
    if (use_unchecked_values)
    {
      svp->SetUncheckedElements(values);
    }
    else
    {
      svp->SetElements(values);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkSMRegistrationNamesDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
