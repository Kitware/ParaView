// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMDynamicPropertiesDomain.h"

#include "vtkDynamicProperties.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringFormatter.h"
#include "json/json.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMDynamicPropertiesDomain);

//------------------------------------------------------------------------------
void vtkSMDynamicPropertiesDomain::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkSMDynamicPropertiesDomain::IsInDomain(vtkSMProperty* property)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(property);
  if (!svp)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkSMDynamicPropertiesDomain::SetDefaultValues(vtkSMProperty* outProperty, bool useUnchecked)
{
  vtkSMStringVectorProperty* svpInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->GetInfoProperty());
  vtkSMStringVectorProperty* svpOut = vtkSMStringVectorProperty::SafeDownCast(outProperty);

  if (svpOut && svpInfo && svpInfo->GetNumberOfElements() == 1)
  {
    std::string stringProperties{ svpInfo->GetElement(0) };
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;

    // Parse from string
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
    if (!reader->parse(stringProperties.c_str(),
          stringProperties.c_str() + stringProperties.length(), &root, &errs))
    {
      vtkErrorMacro("Error parsing JSON: " << errs);
      return 0;
    }
    if (root.isObject() && root.isMember(vtkDynamicProperties::PROPERTIES_KEY) &&
      root[vtkDynamicProperties::PROPERTIES_KEY].isArray())
    {
      Json::Value properties = root[vtkDynamicProperties::PROPERTIES_KEY];
      svpOut->SetNumberOfElements(properties.size() * 3);
      int outIdx = 0;
      for (const auto& property : properties)
      {
        std::string name = property[vtkDynamicProperties::NAME_KEY].asString();
        svpOut->SetElement(outIdx++, name.c_str());

        int type = property[vtkDynamicProperties::TYPE_KEY].asInt();
        svpOut->SetElement(outIdx++, vtk::to_string(type).c_str());
        if (!property.isMember(vtkDynamicProperties::DEFAULT_KEY))
        {
          vtkWarningMacro("Warning: " << name << " does not have a default value");
        }
        svpOut->SetElement(outIdx++,
          property.isMember(vtkDynamicProperties::DEFAULT_KEY)
            ? property[vtkDynamicProperties::DEFAULT_KEY].asString().c_str()
            : "0");
      }
      return 1;
    }
  }
  return this->Superclass::SetDefaultValues(outProperty, useUnchecked);
}

//------------------------------------------------------------------------------
vtkSMDynamicPropertiesDomain::vtkSMDynamicPropertiesDomain() = default;

//------------------------------------------------------------------------------
vtkSMDynamicPropertiesDomain::~vtkSMDynamicPropertiesDomain() = default;
