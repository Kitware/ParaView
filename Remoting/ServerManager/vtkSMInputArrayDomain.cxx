// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMInputArrayDomain.h"

#include "vtkDataObjectTypes.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <vtksys/SystemTools.hxx>

vtkStandardNewMacro(vtkSMInputArrayDomain);

//---------------------------------------------------------------------------
static const char* const vtkSMInputArrayDomainAttributeTypes[] = { "point", "cell", "field",
  "any-except-field", "vertex", "edge", "row", "any", nullptr };

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::vtkSMInputArrayDomain()
  : AttributeType(vtkSMInputArrayDomain::ANY_EXCEPT_FIELD)
{
}

//---------------------------------------------------------------------------
vtkSMInputArrayDomain::~vtkSMInputArrayDomain() = default;

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
  {
    return vtkSMDomain::IN_DOMAIN;
  }

  if (!property)
  {
    return vtkSMDomain::NOT_IN_DOMAIN;
  }

  vtkSMUncheckedPropertyHelper helper(property);
  if (helper.GetNumberOfElements() == 0)
  {
    return vtkSMDomain::NOT_IN_DOMAIN;
  }

  unsigned int skipped_count = 0;
  for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; cc++)
  {
    auto proxy = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(cc));
    const auto port = helper.GetOutputPort(cc);
    switch (this->IsInDomain(proxy, port))
    {
      case vtkSMDomain::NOT_IN_DOMAIN:
        return vtkSMDomain::NOT_IN_DOMAIN;

      case vtkSMDomain::NOT_APPLICABLE:
        ++skipped_count;
        break;

      case vtkSMDomain::IN_DOMAIN:
        break;
    }
  }

  return (helper.GetNumberOfElements() == skipped_count) ? vtkSMDomain::NOT_APPLICABLE
                                                         : vtkSMDomain::IN_DOMAIN;
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsInDomain(vtkSMSourceProxy* proxy, unsigned int outputport /*=0*/)
{
  if (!proxy)
  {
    return vtkSMDomain::NOT_IN_DOMAIN;
  }

  // Make sure the outputs are created.
  proxy->CreateOutputPorts();
  vtkPVDataInformation* info = proxy->GetDataInformation(outputport);
  if (!info)
  {
    return vtkSMDomain::NOT_IN_DOMAIN;
  }

  if (!this->DataType.empty())
  {
    if (!info->DataSetTypeIsA(this->DataType.c_str()))
    {
      return vtkSMDomain::NOT_APPLICABLE;
    }
  }

  int attribute_types_to_try[] = { vtkDataObject::POINT, vtkDataObject::CELL, vtkDataObject::FIELD,
    vtkDataObject::VERTEX, vtkDataObject::EDGE, vtkDataObject::ROW, -1 };

  for (int kk = 0; attribute_types_to_try[kk] != -1; kk++)
  {
    int attribute_type = attribute_types_to_try[kk];

    // check if attribute_type is acceptable.
    if (this->IsAttributeTypeAcceptable(attribute_type))
    {
      vtkPVDataSetAttributesInformation* dsaInfo =
        info->GetAttributeInformation(attribute_types_to_try[kk]);
      if (this->HasAcceptableArray(dsaInfo))
      {
        return vtkSMDomain::IN_DOMAIN;
      }
    }
  }

  return vtkSMDomain::NOT_IN_DOMAIN;
}

//----------------------------------------------------------------------------
bool vtkSMInputArrayDomain::IsAttributeTypeAcceptable(
  int required_type, int attribute_type, int* acceptable_as_type /*=nullptr*/)
{
  return vtkSMInputArrayDomain::IsAttributeTypeAcceptable(required_type, attribute_type,
    acceptable_as_type, vtkSMInputArrayDomain::GetAutomaticPropertyConversion());
}

//----------------------------------------------------------------------------
bool vtkSMInputArrayDomain::IsAttributeTypeAcceptable(
  int required_type, int attribute_type, int* acceptable_as_type, bool auto_convert)
{
  if (acceptable_as_type)
  {
    *acceptable_as_type = attribute_type;
  }

  if (required_type == ANY_EXCEPT_FIELD && attribute_type == FIELD)
  {
    return false;
  }

  if (required_type == ANY || required_type == ANY_EXCEPT_FIELD)
  {
    return attribute_type == POINT || attribute_type == CELL || attribute_type == FIELD ||
      attribute_type == EDGE || attribute_type == VERTEX || attribute_type == ROW;
  }

  switch (attribute_type)
  {
    case vtkDataObject::POINT:
      if (required_type == POINT)
      {
        return true;
      }
      if (required_type == CELL && auto_convert)
      {
        // this a POINT array, however since auto_convert is ON,
        // this array can is acceptable as a CELL array. In other words, caller
        // can pretend this array is a CELL array and VTK pipeline will take care
        // of it.
        if (acceptable_as_type)
        {
          *acceptable_as_type = CELL;
        }
        return true;
      }
      break;

    case vtkDataObject::CELL:
      if (required_type == CELL)
      {
        return true;
      }
      if (required_type == POINT && auto_convert)
      {
        // this a CELL array, however since auto_convert is ON,
        // this array can is acceptable as a POINT array. In other words, caller
        // can pretend this array is a POINT array and VTK pipeline will take care
        // of it.
        if (acceptable_as_type)
        {
          *acceptable_as_type = POINT;
        }
        return true;
      }
      break;

    default:
      break;
  }

  return required_type == attribute_type;
}

//----------------------------------------------------------------------------
int vtkSMInputArrayDomain::IsArrayAcceptable(vtkPVArrayInformation* arrayInfo)
{
  if (arrayInfo == nullptr)
  {
    return -1;
  }

  int numberOfComponents = arrayInfo->GetNumberOfComponents();

  // Empty AcceptableNumbersOfComponents means any number of components
  // are acceptable
  if (this->AcceptableNumbersOfComponents.empty())
  {
    return numberOfComponents;
  }

  bool acceptableConversion = false;
  for (unsigned int i = 0; i < this->AcceptableNumbersOfComponents.size(); i++)
  {
    // 0 means all are ok
    if (this->AcceptableNumbersOfComponents[i] == 0)
    {
      return numberOfComponents;
    }

    // Standard use, where the AcceptableNumbersOfComponents is the
    // number of components of the array
    else if (this->AcceptableNumbersOfComponents[i] == numberOfComponents)
    {
      return numberOfComponents;
    }

    // Conversion use, only if activated, and only if AcceptableNumbersOfComponents is 1
    // Also we keep trying to check if there are other better AcceptableNumbersOfComponents
    else if (this->GetAutoConvertProperties() && this->AcceptableNumbersOfComponents[i] == 1 &&
      numberOfComponents > 1)
    {
      acceptableConversion = true;
    }
  }
  return acceptableConversion ? 1 : -1;
}

//----------------------------------------------------------------------------
bool vtkSMInputArrayDomain::IsAttributeTypeAcceptable(int attributeType)
{
  return vtkSMInputArrayDomain::IsAttributeTypeAcceptable(
    this->AttributeType, attributeType, nullptr, this->GetAutoConvertProperties());
}

//----------------------------------------------------------------------------
bool vtkSMInputArrayDomain::HasAcceptableArray(vtkPVDataSetAttributesInformation* attrInfo)
{
  for (int idx = 0, num = attrInfo->GetNumberOfArrays(); idx < num; ++idx)
  {
    vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(idx);
    if (this->IsArrayAcceptable(arrayInfo) != -1)
    {
      return true;
    }
  }

  return false;
}

//---------------------------------------------------------------------------
int vtkSMInputArrayDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(prop, element);

  if (this->GetRequiredProperty("FieldDataSelection"))
  {
    vtkWarningMacro("vtkSMInputArrayDomain no longer supports required property "
                    "'FieldDataSelection'. Please update the domain definition.");
  }

  const char* attribute_type = element->GetAttribute("attribute_type");
  if (attribute_type)
  {
    this->SetAttributeType(attribute_type);
  }

  // Recover comma ',' separated acceptable number_of_components
  const char* numComponents = element->GetAttribute("number_of_components");
  if (numComponents)
  {
    typedef std::vector<std::string> VStrings;
    const VStrings numbers = vtksys::SystemTools::SplitString(numComponents, ',');

    this->AcceptableNumbersOfComponents.clear();
    for (VStrings::const_iterator iter = numbers.begin(); iter != numbers.end(); ++iter)
    {
      this->AcceptableNumbersOfComponents.push_back(atoi(iter->c_str()));
    }
  }

  // handle optional `data_type`.
  if (const char* dataType = element->GetAttribute("data_type"))
  {
    if (vtkDataObjectTypes::GetTypeIdFromClassName(dataType) != -1)
    {
      this->DataType = dataType;
    }
    else
    {
      vtkErrorMacro("'data_type' is set to '" << dataType << "' which is not a known data type.");
    }
  }
  else
  {
    this->DataType.clear();
  }

  if (const char* autoConvert = element->GetAttributeOrDefault("auto_convert_association", "0"))
  {
    this->AutoConvertProperties = std::atoi(autoConvert) == 1;
  }

  return 1;
}

//---------------------------------------------------------------------------
const char* vtkSMInputArrayDomain::GetAttributeTypeAsString()
{
  if (this->AttributeType < vtkSMInputArrayDomain::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    return vtkSMInputArrayDomainAttributeTypes[this->AttributeType];
  }

  return "(invalid)";
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SetAttributeType(const char* type)
{
  if (type == nullptr)
  {
    vtkErrorMacro("No type specified");
    return;
  }

  for (int cc = 0; cc < vtkSMInputArrayDomain::NUMBER_OF_ATTRIBUTE_TYPES; ++cc)
  {
    if (strcmp(type, vtkSMInputArrayDomainAttributeTypes[cc]) == 0)
    {
      this->SetAttributeType(cc);
      return;
    }
  }
  // for old code where none==field.
  if (strcmp(type, "none") == 0)
  {
    this->SetAttributeType(vtkSMInputArrayDomain::FIELD);
    return;
  }
  vtkErrorMacro("Unrecognized attribute type: " << type);
}

//---------------------------------------------------------------------------
std::vector<int> vtkSMInputArrayDomain::GetAcceptableNumbersOfComponents() const
{
  return this->AcceptableNumbersOfComponents;
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::SetAutomaticPropertyConversion(bool convert)
{
  vtkPVGeneralSettings::GetInstance()->SetAutoConvertProperties(convert);
}

//---------------------------------------------------------------------------
bool vtkSMInputArrayDomain::GetAutoConvertProperties()
{
  bool autoConvert =
    vtkSMInputArrayDomain::GetAutomaticPropertyConversion() || this->AutoConvertProperties;
  vtkDebugMacro("Return AutoConvertProperties: " << autoConvert);
  return autoConvert;
}

//---------------------------------------------------------------------------
bool vtkSMInputArrayDomain::GetAutomaticPropertyConversion()
{
  return vtkPVGeneralSettings::GetInstance()->GetAutoConvertProperties();
}

//---------------------------------------------------------------------------
vtkSMInputArrayDomain* vtkSMInputArrayDomain::FindApplicableDomain(vtkSMProperty* property)
{
  auto iter = property->NewDomainIterator();
  vtkSMInputArrayDomain* chosen = nullptr;
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
  {
    if (auto iad = vtkSMInputArrayDomain::SafeDownCast(iter->GetDomain()))
    {
      chosen = chosen ? chosen : iad;
      if (iad->IsInDomain(property) == vtkSMDomain::IN_DOMAIN)
      {
        chosen = iad;
        break;
      }
    }
  }
  iter->Delete();
  return chosen;
}

//---------------------------------------------------------------------------
void vtkSMInputArrayDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AcceptableNumbersOfComponents: ";
  if (this->AcceptableNumbersOfComponents.empty())
  {
    os << "0" << endl;
  }
  else
  {
    for (unsigned int i = 0; i < this->AcceptableNumbersOfComponents.size(); i++)
    {
      os << this->AcceptableNumbersOfComponents[i] << " " << endl;
    }
  }
  os << indent << "AttributeType: " << this->AttributeType << " ("
     << this->GetAttributeTypeAsString() << ")" << endl;
  os << indent << "DataType: " << (this->DataType.empty() ? "(none)" : this->DataType.c_str())
     << endl;
}
