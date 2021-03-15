/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArrayListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArrayListDomain.h"

#include "vtkDataSetAttributes.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMUncheckedPropertyHelper.h"
#include "vtkStringList.h"

#include <algorithm>
#include <cassert>
#include <set>
#include <sstream>
#include <vector>

vtkStandardNewMacro(vtkSMArrayListDomain);

struct vtkSMArrayListDomainInformationKey
{
  std::string Location;
  std::string Name;
  int Strategy;
};

struct vtkSMArrayListDomainArrayInformation
{
  std::string ArrayName;
  bool IsPartial;

  // data association for this array on input.
  int FieldAssociation;

  // desired association for this array.
  int DomainAssociation;

  int ArrayAttributeType;

  vtkSMArrayListDomainArrayInformation()
    : IsPartial(false)
    , FieldAssociation(vtkDataObject::POINT)
    , DomainAssociation(vtkDataObject::POINT)
    , ArrayAttributeType(-1)
  {
  }

  bool operator==(const vtkSMArrayListDomainArrayInformation& other) const
  {
    return this->ArrayName == other.ArrayName && this->IsPartial == other.IsPartial &&
      this->FieldAssociation == other.FieldAssociation &&
      this->DomainAssociation == other.DomainAssociation &&
      this->ArrayAttributeType == other.ArrayAttributeType;
  }

  // helps keep the set sorted in preferred order.
  bool operator<(const vtkSMArrayListDomainArrayInformation& other) const
  {
    if (this->DomainAssociation == other.DomainAssociation)
    {
      if (this->FieldAssociation == other.FieldAssociation)
      {
        if (this->ArrayName == other.ArrayName)
        {
          // don't treat partial and non-partial arrays as different.
          // if (this->IsPartial == other.IsPartial)
          //  {
          return this->ArrayAttributeType < other.ArrayAttributeType;
          //  }
          // return this->IsPartial < other.IsPartial;
        }
        return this->ArrayName < other.ArrayName;
      }
      return this->FieldAssociation < other.FieldAssociation;
    }
    return this->DomainAssociation < other.DomainAssociation;
  }
};

class vtkSMArrayListDomainInternals
{
public:
  std::vector<vtkSMArrayListDomainInformationKey> InformationKeys;

  std::vector<int> DataTypes;

  typedef std::vector<vtkSMArrayListDomainArrayInformation> DomainValuesVector;
  DomainValuesVector DomainValues;

  typedef std::set<vtkSMArrayListDomainArrayInformation> DomainValuesSet;

  // Builds the list of acceptable arrays in result.
  void BuildArrayList(DomainValuesSet& result, vtkSMArrayListDomain* self,
    vtkSMProperty* fieldDataSelection, vtkSMInputArrayDomain* iad, vtkPVDataInformation* dataInfo);

  std::vector<std::string> GetDomainValueStrings()
  {
    std::vector<std::string> values;
    for (DomainValuesVector::const_iterator iter = this->DomainValues.begin(),
                                            end = this->DomainValues.end();
         iter != end; ++iter)
    {
      values.push_back(iter->ArrayName);
    }
    return values;
  }

  const vtkSMArrayListDomainArrayInformation* FindAttribute(int array_attribute)
  {
    for (DomainValuesVector::const_iterator iter = this->DomainValues.begin(),
                                            end = this->DomainValues.end();
         iter != end; ++iter)
    {
      if (iter->ArrayAttributeType == array_attribute)
      {
        return &(*iter);
      }
    }
    return nullptr;
  }

private:
  bool IsArrayDataTypeAcceptable(vtkPVArrayInformation* arrayInfo) const
  {
    for (size_t cc = 0, max = this->DataTypes.size(); cc < max; ++cc)
    {
      if (this->DataTypes[cc] == VTK_VOID || this->DataTypes[cc] == arrayInfo->GetDataType())
      {
        return true;
      }
    }
    return (this->DataTypes.size() == 0);
  }

  bool AreArrayInformationKeysAcceptable(vtkPVArrayInformation* arrayInfo)
  {
    for (size_t cc = 0, max = this->InformationKeys.size(); cc < max; ++cc)
    {
      vtkSMArrayListDomainInformationKey& key = this->InformationKeys[cc];
      int hasInfo = arrayInfo->HasInformationKey(key.Location.c_str(), key.Name.c_str());
      if (hasInfo && key.Strategy == vtkSMArrayListDomain::REJECT_KEY)
      {
        return false;
      }
      if (!hasInfo && key.Strategy == vtkSMArrayListDomain::NEED_KEY)
      {
        return false;
      }
    }
    return true;
  }
};

//---------------------------------------------------------------------------
void vtkSMArrayListDomainInternals::BuildArrayList(
  vtkSMArrayListDomainInternals::DomainValuesSet& result, vtkSMArrayListDomain* self,
  vtkSMProperty* fieldDataSelection, vtkSMInputArrayDomain* iad, vtkPVDataInformation* dataInfo)
{
  // Recover association
  int association = vtkSMInputArrayDomain::ANY;
  if (iad)
  {
    association = iad->GetAttributeType();
  }

  if (fieldDataSelection)
  {
    vtkSMUncheckedPropertyHelper helper(fieldDataSelection);
    // Support for ArrayListDomain containing FieldDataSelection property
    // and a single parameter, see vtkPSciViz* in filters.xml
    if (helper.GetNumberOfElements() == 1)
    {
      association = helper.GetAsInt(0);
    }
    // Support for ArrayListDomain containing FieldDataSelection property
    // and a 5 parameters, see SetInputArrayToProcess in filters.xml
    else if (helper.GetNumberOfElements() == 5)
    {
      association = helper.GetAsInt(3);
    }
    else
    {
      vtkWarningWithObjectMacro(
        self, "FieldDataSelection property not supported, will be ignored.");
    }
  }

  // validate association.
  if (association < vtkSMInputArrayDomain::POINT ||
    association >= vtkSMInputArrayDomain::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    vtkWarningWithObjectMacro(self, "Invalid association. Ignoring.");
    association = vtkSMInputArrayDomain::ANY;
  }

  if (self->GetNoneString() && result.size() == 0)
  {
    vtkSMArrayListDomainArrayInformation info;
    info.ArrayName = self->GetNoneString();
    info.IsPartial = false;
    info.FieldAssociation = 0;
    info.DomainAssociation = 0;
    result.insert(info);
  }

  // iterate over attributes arrays in dataInfo and add acceptable arrays to the
  // domain.
  for (int type = vtkSMInputArrayDomain::POINT;
       type < vtkSMInputArrayDomain::NUMBER_OF_ATTRIBUTE_TYPES; type++)
  {
    if (type == vtkSMInputArrayDomain::ANY)
    {
      continue;
    }
    vtkPVDataSetAttributesInformation* attrInfo = dataInfo->GetAttributeInformation(type);
    int acceptable_as = type;
    if (attrInfo == nullptr ||
      !vtkSMInputArrayDomain::IsAttributeTypeAcceptable(association, type, &acceptable_as))
    {
      continue;
    }
    assert(acceptable_as != vtkSMInputArrayDomain::ANY &&
      acceptable_as != vtkSMInputArrayDomain::ANY_EXCEPT_FIELD);

    // iterate over all arrays and add them to the list, if acceptable.
    for (int idx = 0, maxIdx = attrInfo->GetNumberOfArrays(); idx < maxIdx; ++idx)
    {
      vtkPVArrayInformation* arrayInfo = attrInfo->GetArrayInformation(idx);
      if (arrayInfo == nullptr)
      {
        continue;
      }

      // First check if the array is acceptable by the domain.
      int acceptedNumberOfComponents = 0;
      if (iad != nullptr)
      {
        acceptedNumberOfComponents = iad->IsArrayAcceptable(arrayInfo);
        if (acceptedNumberOfComponents == -1)
        {
          continue;
        }
      }

      // Then, check if the array name is acceptable
      if (self->IsFilteredArray(dataInfo, type, arrayInfo->GetName()))
      {
        continue;
      }

      // Next, check if the array is acceptable based on the data-type
      // limitations specified on self.
      if (!this->IsArrayDataTypeAcceptable(arrayInfo))
      {
        continue;
      }

      // Next, check if the array is acceptable based on the information-key
      // magic.
      if (!this->AreArrayInformationKeysAcceptable(arrayInfo))
      {
        continue;
      }

      // ACCEPTABLE ARRAY FOUND!!!
      // Add it to the list.
      if (acceptedNumberOfComponents == 0 ||
        acceptedNumberOfComponents == arrayInfo->GetNumberOfComponents())
      {
        // the array is directly acceptable (no need to split out components)
        vtkSMArrayListDomainArrayInformation info;
        info.ArrayName = arrayInfo->GetName();
        info.IsPartial = (arrayInfo->GetIsPartial() != 0);
        info.FieldAssociation = acceptable_as;
        info.DomainAssociation = type;
        info.ArrayAttributeType = attrInfo->IsArrayAnAttribute(idx);
        result.insert(info);
      }
      else
      {
        // array has number of components that doesn't directly match the required
        // component count. The array is being accepted due to automatic property
        // conversion. So we need to split up components and add them individually.
        assert(acceptedNumberOfComponents == 1 &&
          vtkSMInputArrayDomain::GetAutomaticPropertyConversion() == true &&
          arrayInfo->GetNumberOfComponents() > 1);

        for (int cc = 0, maxCC = arrayInfo->GetNumberOfComponents(); cc <= maxCC; ++cc)
        {
          if (cc == maxCC && arrayInfo->GetDataType() == VTK_STRING)
          {
            // magnitude is added only for numeric arrays.
            continue;
          }
          vtkSMArrayListDomainArrayInformation info;
          info.ArrayName = vtkSMArrayListDomain::CreateMangledName(arrayInfo, cc);
          info.IsPartial = (arrayInfo->GetIsPartial() != 0);
          info.FieldAssociation = acceptable_as;
          info.DomainAssociation = type;
          info.ArrayAttributeType = attrInfo->IsArrayAnAttribute(idx);
          result.insert(info);
        }
      }
    } // end of for each array
  }   // end of for each attribute type
}

//---------------------------------------------------------------------------
vtkSMArrayListDomain::vtkSMArrayListDomain()
{
  this->AttributeType = vtkDataSetAttributes::SCALARS;
  this->InputDomainName = nullptr;
  this->NoneString = nullptr;
  this->ALDInternals = new vtkSMArrayListDomainInternals;
  this->PickFirstAvailableArrayByDefault = true;
}

//---------------------------------------------------------------------------
vtkSMArrayListDomain::~vtkSMArrayListDomain()
{
  this->SetInputDomainName(nullptr);
  this->SetNoneString(nullptr);
  delete this->ALDInternals;
  this->ALDInternals = nullptr;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::IsArrayPartial(unsigned int idx)
{
  if (static_cast<size_t>(idx) < this->ALDInternals->DomainValues.size())
  {
    return this->ALDInternals->DomainValues[idx].IsPartial ? 1 : 0;
  }

  vtkErrorMacro("Index out of range: " << idx);
  return 0;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::GetDomainAssociation(unsigned int idx)
{
  if (static_cast<size_t>(idx) < this->ALDInternals->DomainValues.size())
  {
    return this->ALDInternals->DomainValues[idx].DomainAssociation;
  }

  vtkErrorMacro("Index out of range: " << idx);
  return -1;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::GetFieldAssociation(unsigned int idx)
{
  if (static_cast<size_t>(idx) < this->ALDInternals->DomainValues.size())
  {
    return this->ALDInternals->DomainValues[idx].FieldAssociation;
  }

  vtkErrorMacro("Index out of range: " << idx);
  return -1;
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* input = this->GetRequiredProperty("Input");
  if (!input)
  {
    vtkErrorMacro("Missing require property 'Input'. Update failed.");
    return;
  }

  vtkPVDataInformation* dataInfo = this->GetInputDataInformation("Input");
  if (!dataInfo)
  {
    return;
  }

  vtkSMProperty* fieldDataSelection = this->GetRequiredProperty("FieldDataSelection");
  vtkSMInputArrayDomain* iad = this->InputDomainName
    ? vtkSMInputArrayDomain::SafeDownCast(input->GetDomain(this->InputDomainName))
    : vtkSMInputArrayDomain::FindApplicableDomain(input);

  // we use a set so that the list gets sorted as well as helps us
  // avoid duplicates esp. when processing two datainformation objects.
  vtkSMArrayListDomainInternals::DomainValuesSet set;
  this->ALDInternals->BuildArrayList(set, this, fieldDataSelection, iad, dataInfo);

  vtkPVDataInformation* extraInfo = this->GetExtraDataInformation();
  if (extraInfo)
  {
    this->ALDInternals->BuildArrayList(set, this, fieldDataSelection, iad, extraInfo);
  }

  vtkSMArrayListDomainInternals::DomainValuesVector values;
  values.insert(values.end(), set.begin(), set.end());
  if (values != this->ALDInternals->DomainValues)
  {
    this->ALDInternals->DomainValues = values;

    // ensures that we fire DomainModifiedEvent only once.
    DeferDomainModifiedEvents defer(this);

    this->SetStrings(this->ALDInternals->GetDomainValueStrings());

    // since `this->SetStrings()` may not necessarily have changed even though
    // the arrays changed (e.g. BUG #18628), we explicitly fire domain modified.
    // Note DeferDomainModifiedEvents ensures that the event gets fired only once.
    this->DomainModified();
  }
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* input_domain_name = element->GetAttribute("input_domain_name");
  if (input_domain_name)
  {
    this->SetInputDomainName(input_domain_name);
  }

  // Search for attribute type with matching name.
  const char* none_string = element->GetAttribute("none_string");
  if (none_string)
  {
    this->SetNoneString(none_string);
  }

  const char* key_locations = element->GetAttribute("key_locations");
  const char* key_names = element->GetAttribute("key_names");
  const char* key_strategies = element->GetAttribute("key_strategies");
  if (key_locations == nullptr)
  {
    // Default value : add a needed information key vtkAbstractArray::GUI_HIDE
    this->AddInformationKey("vtkAbstractArray", "GUI_HIDE", vtkSMArrayListDomain::REJECT_KEY);
  }
  else
  {
    std::stringstream locations(key_locations);
    std::stringstream names(key_names);
    std::stringstream strategies(key_strategies);
    std::string location, name, strategy, last_strategy;
    int strat;
    // default value for the strategy
    last_strategy = "need_key";
    while (locations >> location)
    {
      if (!(names >> name))
      {
        vtkErrorMacro("The number of key names must be equal to the number of key locations");
        break;
      }
      if (!(strategies >> strategy))
      {
        strategy = last_strategy;
      }
      last_strategy = strategy;

      if (strategy == "need_key" || strategy == "NEED_KEY")
      {
        strat = vtkSMArrayListDomain::NEED_KEY;
      }
      else if (strategy == "reject_key" || strategy == "REJECT_KEY")
      {
        strat = vtkSMArrayListDomain::REJECT_KEY;
      }
      else
      {
        // maybe the strategy was coded as an integer.
        strat = atoi(strategy.c_str());
      }
      this->AddInformationKey(location.c_str(), name.c_str(), strat);
    }
  }

  // Search for attribute type to use when picking a default.
  const char* attribute_type = element->GetAttribute("attribute_type");
  if (attribute_type)
  {
    for (unsigned int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; ++i)
    {
      if (strcmp(vtkDataSetAttributes::GetAttributeTypeAsString(i), attribute_type) == 0)
      {
        this->SetAttributeType(i);
        break;
      }
    }
  }

  const char* data_type = element->GetAttribute("data_type");
  if (data_type)
  {
    // data_type can be a space delimited list of types
    // vlaid for the domain
    std::istringstream dataTypeStream(data_type);

    while (dataTypeStream.good())
    {
      std::string thisType;
      dataTypeStream >> thisType;

      if (thisType == "VTK_VOID")
      {
        this->ALDInternals->DataTypes.push_back(VTK_VOID);
      }
      else if (thisType == "VTK_BIT")
      {
        this->ALDInternals->DataTypes.push_back(VTK_BIT);
      }
      else if (thisType == "VTK_CHAR")
      {
        this->ALDInternals->DataTypes.push_back(VTK_CHAR);
      }
      else if (thisType == "VTK_SIGNED_CHAR")
      {
        this->ALDInternals->DataTypes.push_back(VTK_SIGNED_CHAR);
      }
      else if (thisType == "VTK_UNSIGNED_CHAR")
      {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_CHAR);
      }
      else if (thisType == "VTK_SHORT")
      {
        this->ALDInternals->DataTypes.push_back(VTK_SHORT);
      }
      else if (thisType == "VTK_UNSIGNED_SHORT")
      {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_SHORT);
      }
      else if (thisType == "VTK_INT")
      {
        this->ALDInternals->DataTypes.push_back(VTK_INT);
      }
      else if (thisType == "VTK_UNSIGNED_INT")
      {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_INT);
      }
      else if (thisType == "VTK_LONG")
      {
        this->ALDInternals->DataTypes.push_back(VTK_LONG);
      }
      else if (thisType == "VTK_FLOAT")
      {
        this->ALDInternals->DataTypes.push_back(VTK_FLOAT);
      }
      else if (thisType == "VTK_DOUBLE")
      {
        this->ALDInternals->DataTypes.push_back(VTK_DOUBLE);
      }
      else if (thisType == "VTK_ID_TYPE")
      {
        this->ALDInternals->DataTypes.push_back(VTK_ID_TYPE);
      }
      else if (thisType == "VTK_STRING")
      {
        this->ALDInternals->DataTypes.push_back(VTK_STRING);
      }
      else if (thisType == "VTK_OPAQUE")
      {
        this->ALDInternals->DataTypes.push_back(VTK_OPAQUE);
      }
      else if (thisType == "VTK_LONG_LONG")
      {
        this->ALDInternals->DataTypes.push_back(VTK_LONG_LONG);
      }
      else if (thisType == "VTK_UNSIGNED_LONG_LONG")
      {
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_LONG_LONG);
      }
      else if (thisType == "VTK_DATA_ARRAY")
      {
        this->ALDInternals->DataTypes.push_back(VTK_BIT);
        this->ALDInternals->DataTypes.push_back(VTK_CHAR);
        this->ALDInternals->DataTypes.push_back(VTK_SIGNED_CHAR);
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_CHAR);
        this->ALDInternals->DataTypes.push_back(VTK_SHORT);
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_SHORT);
        this->ALDInternals->DataTypes.push_back(VTK_INT);
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_INT);
        this->ALDInternals->DataTypes.push_back(VTK_LONG);
        this->ALDInternals->DataTypes.push_back(VTK_FLOAT);
        this->ALDInternals->DataTypes.push_back(VTK_DOUBLE);
        this->ALDInternals->DataTypes.push_back(VTK_ID_TYPE);
        this->ALDInternals->DataTypes.push_back(VTK_LONG_LONG);
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_LONG);
        this->ALDInternals->DataTypes.push_back(VTK_UNSIGNED_LONG_LONG);
      }
      else
      {
        int maybeType = atoi(thisType.c_str()); //?
        this->ALDInternals->DataTypes.push_back(maybeType);
      }
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
  {
    return 0;
  }

  vtkSMPropertyHelper helper(prop);
  helper.SetUseUnchecked(use_unchecked_values);

  // If vtkSMStringVectorProperty has a default value which is in domain, just
  // use it.
  const char* defaultValue = svp->GetDefaultValue(0);
  unsigned int temp;
  if (defaultValue)
  {
    if (this->IsInDomain(defaultValue, temp))
    {
      // Support for SetInputArrayToProcess with single default value array name
      if (helper.GetNumberOfElements() == 5)
      {
        helper.Set(4, defaultValue);
        return 1;
      }
      // Standard default value
      else if (helper.GetNumberOfElements() == 1)
      {
        helper.Set(0, defaultValue);
        return 1;
      }
    }
    else if (helper.GetNumberOfElements() == 5)
    {
      // Support for set input array to process with full length default values
      // default_values="idx;port;connection;fieldAsso;arrayName"
      defaultValue = svp->GetDefaultValue(4);
      if (this->IsInDomain(defaultValue, temp))
      {
        helper.Set(4, defaultValue);

        // The Default FieldAssociation can be incorrect so we use the actual
        // in-domain array field association, in a way the arrayName
        // defaultValue can override the fieldAssociation default value
        helper.Set(3, this->ALDInternals->DomainValues[temp].FieldAssociation);

        // Other value, idx, port and connexion cannot be changed
        // and already correspond to the default values thanks to
        // vtkSIStringVectorProperty::ReadXMLAttributes
        return 1;
      }
    }
  }

  const vtkSMArrayListDomainArrayInformation* info =
    this->ALDInternals->FindAttribute(this->AttributeType);
  if (info == nullptr && this->ALDInternals->DomainValues.size() > 0)
  {
    if (this->PickFirstAvailableArrayByDefault == true)
    {
      info = &this->ALDInternals->DomainValues[0];
    }
    else
    {
      // check for None string
      if (this->NoneString)
      {
        for (const auto& currentDomainValue : this->ALDInternals->DomainValues)
        {
          if (currentDomainValue.ArrayName == this->NoneString)
          {
            info = &currentDomainValue;
            break;
          }
        }
      }
    }
  }
  if (info)
  {
    if (helper.GetNumberOfElements() == 5)
    {
      vtkNew<vtkStringList> values;
      if (use_unchecked_values)
      {
        svp->GetUncheckedElements(values.GetPointer());
      }
      else
      {
        svp->GetElements(values.GetPointer());
      }

      std::ostringstream ass;
      ass << info->FieldAssociation;
      values->SetString(3, ass.str().c_str());
      values->SetString(4, info->ArrayName.c_str());
      if (use_unchecked_values)
      {
        svp->SetUncheckedElements(values.GetPointer());
      }
      else
      {
        svp->SetElements(values.GetPointer());
      }
      return 1;
    }
    else if (svp->GetNumberOfElements() == 1)
    {
      helper.Set(0, info->ArrayName.c_str());
      return 1;
    }
  }

  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::AddInformationKey(
  const char* location, const char* name, int strategy)
{
  vtkSMArrayListDomainInformationKey key;
  key.Location = location;
  key.Name = name;
  key.Strategy = strategy;
  this->ALDInternals->InformationKeys.push_back(key);
  return static_cast<unsigned int>(this->ALDInternals->InformationKeys.size() - 1);
}

//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::RemoveInformationKey(const char* location, const char* name)
{
  std::vector<vtkSMArrayListDomainInformationKey>::iterator it =
    this->ALDInternals->InformationKeys.begin();
  int index = 0;
  while (it != this->ALDInternals->InformationKeys.end())
  {
    vtkSMArrayListDomainInformationKey& key = *it;
    if (key.Location == location && key.Name == name)
    {
      this->ALDInternals->InformationKeys.erase(it);
      return index;
    }
    it++;
    index++;
  }
  return 0;
}

//---------------------------------------------------------------------------
unsigned int vtkSMArrayListDomain::GetNumberOfInformationKeys()
{
  return static_cast<unsigned int>(this->ALDInternals->InformationKeys.size());
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::RemoveAllInformationKeys()
{
  this->ALDInternals->InformationKeys.clear();
}

//---------------------------------------------------------------------------
const char* vtkSMArrayListDomain::GetInformationKeyLocation(unsigned int index)
{
  if (index < this->ALDInternals->InformationKeys.size())
  {
    return this->ALDInternals->InformationKeys[index].Location.c_str();
  }
  return nullptr;
}

//---------------------------------------------------------------------------
const char* vtkSMArrayListDomain::GetInformationKeyName(unsigned int index)
{
  if (index < this->ALDInternals->InformationKeys.size())
  {
    return this->ALDInternals->InformationKeys[index].Name.c_str();
  }
  return nullptr;
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::GetInformationKeyStrategy(unsigned int index)
{
  if (index < this->ALDInternals->InformationKeys.size())
  {
    return this->ALDInternals->InformationKeys[index].Strategy;
  }
  return -1;
}

//---------------------------------------------------------------------------
void vtkSMArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AttributeType: " << this->AttributeType << endl;
  int nDataTypes = static_cast<int>(this->ALDInternals->DataTypes.size());
  for (int i = 0; i < nDataTypes; ++i)
  {
    os << indent << "DataType: " << this->ALDInternals->DataTypes[i] << endl;
  }

  for (unsigned int i = 0; i < this->ALDInternals->InformationKeys.size(); i++)
  {
    vtkSMArrayListDomainInformationKey& key = this->ALDInternals->InformationKeys[i];
    os << key.Location << "::" << key.Name << " ";
    if (key.Strategy == vtkSMArrayListDomain::NEED_KEY)
    {
      os << "NEED_KEY";
    }
    else if (key.Strategy == vtkSMArrayListDomain::REJECT_KEY)
    {
      os << "REJECT_KEY";
    }
    else
    {
      os << "UNKNOWN KEY STRATEGY : " << key.Strategy;
    }
    os << endl;
  }
}

//---------------------------------------------------------------------------
std::string vtkSMArrayListDomain::CreateMangledName(vtkPVArrayInformation* arrayInfo, int component)
{
  std::ostringstream stream;
  if (component != arrayInfo->GetNumberOfComponents())
  {
    stream << arrayInfo->GetName() << "_" << arrayInfo->GetComponentName(component);
  }
  else
  {
    stream << arrayInfo->GetName() << "_Magnitude";
  }
  return stream.str();
}

//---------------------------------------------------------------------------
std::string vtkSMArrayListDomain::ArrayNameFromMangledName(const char* name)
{
  std::string extractedName = name;
  size_t pos = extractedName.rfind('_');
  if (pos == std::string::npos)
  {
    return extractedName;
  }
  return extractedName.substr(0, pos);
}

//---------------------------------------------------------------------------
int vtkSMArrayListDomain::ComponentIndexFromMangledName(
  vtkPVArrayInformation* info, const char* name)
{
  std::string extractedName = name;
  size_t pos = extractedName.rfind('_');
  if (pos == std::string::npos)
  {
    return -1;
  }
  std::string compName = extractedName.substr(pos + 1, extractedName.length() - pos);
  int numComps = info->GetNumberOfComponents();
  if (compName == "Magnitude")
  {
    return numComps;
  }
  for (int i = 0; i < numComps; ++i)
  {
    if (compName == info->GetComponentName(i))
    {
      return i;
    }
  }
  return -1;
}

//---------------------------------------------------------------------------
bool vtkSMArrayListDomain::IsFilteredArray(vtkPVDataInformation*, int, const char*)
{
  return false;
}
