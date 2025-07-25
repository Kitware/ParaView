// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMSettings.h"

#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVLogger.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtk_jsoncpp.h"
#include <sstream>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <string>

//----------------------------------------------------------------------------
namespace
{
class SettingsCollection
{
public:
  SettingsCollection()
    : SettingsCollection(vtkSMSettings::GetUserPriority())
  {
  }

  SettingsCollection(double priority)
    : Priority(priority)
  {
  }

  Json::Value Value;
  double Priority;
};

bool SortByPriority(const SettingsCollection& r1, const SettingsCollection& r2)
{
  return (r1.Priority > r2.Priority);
}

// Potentially transform JSON to handle backwards compatibility issues
void TransformJSON(std::string& settingsJSON)
{
  std::string findString("TransferFuctionPresets");
  std::string replaceString("TransferFunctionPresets");
  size_t location = settingsJSON.find(findString);
  while (location != std::string::npos)
  {
    settingsJSON.replace(location, findString.length(), replaceString);
    location = settingsJSON.find(findString);
  }
}

} // end anonymous namespace

class vtkSMSettings::vtkSMSettingsInternal
{
public:
  vtkSMSettingsInternal() = default;

  std::vector<::SettingsCollection> SettingCollections;
  bool SettingCollectionsAreSorted = false;
  bool IsModified = false;

  void Modified() { this->IsModified = true; }

  //----------------------------------------------------------------------------
  // Description:
  // Sort setting collections by priority, from highest to lowest
  void SortSettingCollections()
  {
    // Sort the settings roots by priority (highest to lowest)
    std::stable_sort(
      this->SettingCollections.begin(), this->SettingCollections.end(), ::SortByPriority);
    this->SettingCollectionsAreSorted = true;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Splits a JSON path into branch and leaf components. This is needed
  // to build trees with the JsonCpp library.
  static void SeparateBranchFromLeaf(const char* jsonPath, std::string& root, std::string& leaf)
  {
    root.clear();
    leaf.clear();

    // Chop off leaf setting
    std::string jsonPathString(jsonPath);
    size_t lastPeriod = jsonPathString.find_last_of('.');
    root = jsonPathString.substr(0, lastPeriod);
    leaf = jsonPathString.substr(lastPeriod + 1);
  }

  //----------------------------------------------------------------------------
  // Description:
  // See if given setting is defined
  bool HasSetting(const char* settingName, double maxPriority)
  {
    // Json::Value value = this->GetSetting(settingName);
    Json::Value value = this->GetSettingAtOrBelowPriority(settingName, maxPriority);

    return !value.isNull();
  }

  //----------------------------------------------------------------------------
  // Description: Get a Json::Value given a setting name. Returns the
  // highest-priority setting defined in the setting collections, and
  // nullptr if it isn't defined in any of the collections.
  //
  // String format is:
  // "." => root node
  // ".[n]" => elements at index 'n' of root node (an array value)
  // ".name" => member named 'name' of root node (an object value)
  // ".name1.name2.name3"
  // ".[0][1][2].name1[3]"
  const Json::Value& GetSetting(const char* settingName)
  {
    return this->GetSettingAtOrBelowPriority(settingName, vtkSMSettings::GetUserPriority());
  }

  //----------------------------------------------------------------------------
  const Json::Value& GetSettingBelowPriority(const char* settingName, double priority)
  {
    this->SortCollectionsIfNeeded();

    // Iterate over settings, checking higher priority settings first
    for (size_t i = 0; i < this->SettingCollections.size(); ++i)
    {
      if (this->SettingCollections[i].Priority >= priority)
      {
        continue;
      }

      Json::Path settingPath(settingName);
      const Json::Value& setting = settingPath.resolve(this->SettingCollections[i].Value);
      if (!setting.isNull())
      {
        return setting;
      }
    }

    return Json::Value::nullSingleton();
  }

  //----------------------------------------------------------------------------
  const Json::Value& GetSettingAtOrBelowPriority(const char* settingName, double maxPriority)
  {
    this->SortCollectionsIfNeeded();

    // Iterate over settings, checking higher priority settings first
    for (size_t i = 0; i < this->SettingCollections.size(); ++i)
    {
      if (this->SettingCollections[i].Priority > maxPriority)
      {
        continue;
      }

      Json::Path settingPath(settingName);
      const Json::Value& setting = settingPath.resolve(this->SettingCollections[i].Value);
      if (!setting.isNull())
      {
        return setting;
      }
    }

    return Json::Value::nullSingleton();
  }

  //----------------------------------------------------------------------------
  template <typename T>
  bool GetSetting(const char* settingName, std::vector<T>& values, double maxPriority)
  {
    values.clear();

    Json::Value setting = this->GetSettingAtOrBelowPriority(settingName, maxPriority);
    if (!setting)
    {
      return false;
    }

    if (setting.isObject())
    {
      return false;
    }
    else if (setting.isArray())
    {
      for (Json::Value::ArrayIndex i = 0; i < setting.size(); ++i)
      {
        T value;
        this->ConvertJsonValue(setting[i], value);
        values.push_back(value);
      }
    }
    else
    {
      T value;
      bool success = this->ConvertJsonValue(setting, value);
      if (success)
      {
        values.push_back(value);
      }
      return success;
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(const char* settingName, vtkSMProperty* property, double maxPriority)
  {
    if (!property || !this->HasSetting(settingName, maxPriority))
    {
      return false;
    }

    bool success = false;
    if (vtkSMIntVectorProperty* intVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property))
    {
      success = this->GetPropertySetting(settingName, intVectorProperty, maxPriority);
    }
    else if (vtkSMDoubleVectorProperty* doubleVectorProperty =
               vtkSMDoubleVectorProperty::SafeDownCast(property))
    {
      success = this->GetPropertySetting(settingName, doubleVectorProperty, maxPriority);
    }
    else if (vtkSMStringVectorProperty* stringVectorProperty =
               vtkSMStringVectorProperty::SafeDownCast(property))
    {
      success = this->GetPropertySetting(settingName, stringVectorProperty, maxPriority);
    }
    else if (vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(property))
    {
      success = this->GetPropertySetting(settingName, inputProperty, maxPriority);
    }
    else if (vtkSMProxyProperty* proxyProperty = vtkSMProxyProperty::SafeDownCast(property))
    {
      success = this->GetPropertySetting(settingName, proxyProperty, maxPriority);
    }

    return success;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(
    const char* settingName, vtkSMIntVectorProperty* property, double maxPriority)
  {
    if (!property)
    {
      return false;
    }

    auto enumDomain = property->FindDomain<vtkSMEnumerationDomain>();
    if (enumDomain)
    {
      // The enumeration property could be either text or value
      const Json::Value& jsonValue = this->GetSettingAtOrBelowPriority(settingName, maxPriority);
      int enumValue;
      bool hasInt = this->ConvertJsonValue(jsonValue, enumValue);
      if (hasInt)
      {
        property->SetElement(0, enumValue);
        return true;
      }
      else
      {
        std::string stringValue;
        bool hasString = this->ConvertJsonValue(jsonValue, stringValue);
        if (hasString && enumDomain->HasEntryText(stringValue.c_str()))
        {
          enumValue = enumDomain->GetEntryValueForText(stringValue.c_str());
          property->SetElement(0, enumValue);
          return true;
        }
      }
    }
    std::vector<int> vector;
    if (!this->GetSetting(settingName, vector, maxPriority))
    {
      return false;
    }
    if (property->GetRepeatable())
    {
      property->SetNumberOfElements(static_cast<unsigned int>(vector.size()));
    }
    if (vector.size() != property->GetNumberOfElements())
    {
      return false;
    }

    property->SetElements(&vector[0]);

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(
    const char* settingName, vtkSMDoubleVectorProperty* property, double maxPriority)
  {
    std::vector<double> vector;
    if (!property || !this->GetSetting(settingName, vector, maxPriority))
    {
      return false;
    }
    else if (property->GetRepeatable())
    {
      property->SetNumberOfElements(static_cast<unsigned int>(vector.size()));
    }
    else if (vector.size() != property->GetNumberOfElements())
    {
      return false;
    }

    property->SetElements(&vector[0]);

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(
    const char* settingName, vtkSMStringVectorProperty* property, double maxPriority)
  {
    std::vector<std::string> vector;
    if (!property || !this->GetSetting(settingName, vector, maxPriority))
    {
      return false;
    }

    vtkSmartPointer<vtkStringList> stringList = vtkSmartPointer<vtkStringList>::New();
    for (size_t i = 0; i < vector.size(); ++i)
    {
      std::string vtk_string(vector[i]);
      stringList->AddString(vtk_string.c_str());
    }

    if (property->GetRepeatable())
    {
      property->SetNumberOfElements(static_cast<unsigned int>(vector.size()));
    }
    else if (vector.size() != property->GetNumberOfElements())
    {
      return false;
    }

    property->SetElements(stringList);

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(const char* settingName, vtkSMInputProperty* property, double maxPriority)
  {
    vtkSMProxyProperty* proxyProp = property;
    bool ret = this->GetPropertySetting(settingName, proxyProp, maxPriority);
    property->SetInputConnection(0, property->GetProxy(0), 0);
    return ret;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(const char* settingName, vtkSMProxyProperty* property, double maxPriority)
  {
    auto proxyListDomain = property->FindDomain<vtkSMProxyListDomain>();
    if (proxyListDomain)
    {
      // Now check whether this proxy is the one we want
      for (unsigned int ip = 0; ip < proxyListDomain->GetNumberOfProxies(); ++ip)
      {
        vtkSMProxy* listProxy = proxyListDomain->GetProxy(ip);
        if (listProxy)
        {
          std::string sourceSettingString(settingName);
          bool success =
            this->GetProxySettings(sourceSettingString.c_str(), listProxy, maxPriority);
          if (!success)
          {
            return false;
          }

          sourceSettingString.append(".");
          sourceSettingString.append(listProxy->GetXMLName());
          sourceSettingString.append(".Selected");
          Json::Value jsonValue =
            this->GetSettingAtOrBelowPriority(sourceSettingString.c_str(), maxPriority);

          if (jsonValue.asBool())
          {
            vtkSMPropertyHelper helper(property);
            helper.Set(listProxy);
          }
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetProxySettings(const char* settingPrefix, vtkSMProxy* proxy, double maxPriority)
  {
    if (!proxy)
    {
      return false;
    }

    bool overallSuccess = true;

    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(proxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      vtkSMProperty* property = iter->GetProperty();
      if (!property)
      {
        continue;
      }

      // Check to see if we save only to QSettings or to both QSettings
      // and the JSON file.
      if (property->GetHints())
      {
        vtkPVXMLElement* saveInQSettingsHint =
          property->GetHints()->FindNestedElementByName("SaveInQSettings");
        if (saveInQSettingsHint)
        {
          int bothSettings = 0;
          saveInQSettingsHint->GetScalarAttribute("both", &bothSettings);
          if (!bothSettings)
          {
            continue;
          }
        }
      }

      const char* proxyName = proxy->GetXMLName();
      const char* propertyName = iter->GetKey();
      if (proxyName && propertyName && !property->GetNoCustomDefault())
      {
        // Build the JSON reference string
        std::ostringstream settingStringStream;
        settingStringStream << settingPrefix << "." << proxyName << "." << propertyName;

        const std::string settingString = settingStringStream.str();
        const char* settingName = settingString.c_str();
        if (this->HasSetting(settingName, maxPriority))
        {
          bool success = false;
          if (vtkSMIntVectorProperty* intVectorProperty =
                vtkSMIntVectorProperty::SafeDownCast(property))
          {
            success = this->GetPropertySetting(settingName, intVectorProperty, maxPriority);
          }
          else if (vtkSMDoubleVectorProperty* doubleVectorProperty =
                     vtkSMDoubleVectorProperty::SafeDownCast(property))
          {
            success = this->GetPropertySetting(settingName, doubleVectorProperty, maxPriority);
          }
          else if (vtkSMStringVectorProperty* stringVectorProperty =
                     vtkSMStringVectorProperty::SafeDownCast(property))
          {
            success = this->GetPropertySetting(settingName, stringVectorProperty, maxPriority);
          }
          else if (vtkSMInputProperty* inputProperty = vtkSMInputProperty::SafeDownCast(property))
          {
            success = this->GetPropertySetting(settingName, inputProperty, maxPriority);
          }
          else if (vtkSMProxyProperty* proxyProperty = vtkSMProxyProperty::SafeDownCast(property))
          {
            success = this->GetPropertySetting(settingName, proxyProperty, maxPriority);
          }
          else
          {
            overallSuccess = false;
          }

          if (!success)
          {
            overallSuccess = false;
          }
        }
      }
    }

    return overallSuccess;
  }

  //----------------------------------------------------------------------------
  template <typename T>
  void SetSetting(const char* settingName, const std::vector<T>& values)
  {
    this->CreateCollectionIfNeeded();
    this->SortCollectionsIfNeeded();

    // Just set settings in the highest-priority settings group for now.
    std::string root, leaf;
    this->SeparateBranchFromLeaf(settingName, root, leaf);

    std::vector<T> previousValues;
    this->GetSetting(settingName, previousValues, vtkSMSettings::GetUserPriority());

    Json::Path settingPath(root);
    Json::Value& jsonValue = settingPath.make(this->SettingCollections[0].Value);

    if (values.size() > 1)
    {
      if (!jsonValue[leaf].isArray())
      {
        jsonValue[leaf] = Json::Value::nullSingleton();
      }
      jsonValue[leaf].resize(static_cast<Json::Value::ArrayIndex>(values.size()));

      bool modified = values.size() != previousValues.size();
      for (size_t i = 0; i < values.size(); ++i)
      {
        modified |= (i < previousValues.size() && (previousValues[i] != values[i]));
        jsonValue[leaf][static_cast<Json::Value::ArrayIndex>(i)] = values[i];
      }
      if (modified)
      {
        this->Modified();
      }
    }
    else
    {
      if (previousValues.empty() || previousValues[0] != values[0])
      {
        jsonValue[leaf] = values[0];
        this->Modified();
      }
    }
  }

  //----------------------------------------------------------------------------
  bool SetPropertySetting(const char* settingName, vtkSMIntVectorProperty* property)
  {
    Json::Path valuePath(settingName);
    Json::Value& jsonValue = valuePath.make(this->SettingCollections[0].Value);
    if (property->GetNumberOfElements() == 1)
    {
      if (jsonValue.isArray())
      {
        // Reset to nullptr so that we aren't setting a value on a Json::Value array
        jsonValue = Json::Value::nullSingleton();
        this->Modified();
      }

      if (jsonValue.isNull() || jsonValue.asInt() != property->GetElement(0))
      {
        jsonValue = property->GetElement(0);
        this->Modified();
      }
    }
    else
    {
      if (!jsonValue.isArray() && !jsonValue.isNull())
      {
        // Reset to nullptr so that the jsonValue.resize() operation works
        jsonValue = Json::Value::nullSingleton();
        this->Modified();
      }

      jsonValue.resize(property->GetNumberOfElements());
      for (unsigned int i = 0; i < property->GetNumberOfElements(); ++i)
      {
        if (jsonValue[i].isNull() || jsonValue[i].asInt() != property->GetElement(i))
        {
          jsonValue[i] = property->GetElement(i);
          this->Modified();
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool SetPropertySetting(const char* settingName, vtkSMDoubleVectorProperty* property)
  {
    Json::Path valuePath(settingName);
    Json::Value& jsonValue = valuePath.make(this->SettingCollections[0].Value);
    if (property->GetNumberOfElements() == 1)
    {
      if (jsonValue.isArray())
      {
        // Reset to nullptr so that we aren't setting a value on a Json::Value array
        jsonValue = Json::Value::nullSingleton();
        this->Modified();
      }

      if (jsonValue.isNull() || jsonValue.asDouble() != property->GetElement(0))
      {
        jsonValue = property->GetElement(0);
        this->Modified();
      }
    }
    else
    {
      if (!jsonValue.isArray() && !jsonValue.isNull())
      {
        // Reset to nullptr so that the jsonValue.resize() operation works
        jsonValue = Json::Value::nullSingleton();
        this->Modified();
      }

      jsonValue.resize(property->GetNumberOfElements());
      for (unsigned int i = 0; i < property->GetNumberOfElements(); ++i)
      {
        if (jsonValue[i].isNull() || jsonValue[i].asDouble() != property->GetElement(i))
        {
          jsonValue[i] = property->GetElement(i);
          this->Modified();
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool SetPropertySetting(const char* settingName, vtkSMStringVectorProperty* property)
  {
    Json::Path valuePath(settingName);
    Json::Value& jsonValue = valuePath.make(this->SettingCollections[0].Value);
    if (property->GetNumberOfElements() == 1)
    {
      if (jsonValue.isArray())
      {
        // Reset to nullptr so that we aren't setting a value on a Json::Value array
        jsonValue = Json::Value::nullSingleton();
        this->Modified();
      }

      if (jsonValue.isNull() || strcmp(jsonValue.asCString(), property->GetElement(0)) != 0)
      {
        jsonValue = property->GetElement(0);
        this->Modified();
      }
    }
    else
    {
      if (!jsonValue.isArray() && !jsonValue.isNull())
      {
        // Reset to nullptr so that the jsonValue.resize() operation works
        jsonValue = Json::Value::nullSingleton();
        this->Modified();
      }

      jsonValue.resize(property->GetNumberOfElements());
      for (unsigned int i = 0; i < property->GetNumberOfElements(); ++i)
      {
        if (jsonValue[i].isNull() || strcmp(jsonValue[i].asCString(), property->GetElement(i)) != 0)
        {
          jsonValue[i] = property->GetElement(i);
          this->Modified();
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool SetPropertySetting(const char* settingName, vtkSMProxyProperty* property)
  {
    auto proxyListDomain = property->FindDomain<vtkSMProxyListDomain>();
    const char* currentProxyName = property->GetProxy(0)->GetXMLName();
    if (proxyListDomain)
    {
      for (unsigned int ip = 0; ip < proxyListDomain->GetNumberOfProxies(); ++ip)
      {
        vtkSMProxy* listProxy = proxyListDomain->GetProxy(ip);
        if (listProxy)
        {
          if (!this->SetProxySettings(settingName, listProxy, nullptr, false))
          {
            return false;
          }

          std::string sourceSettingString(settingName);
          sourceSettingString.append(".");
          sourceSettingString.append(listProxy->GetXMLName());
          sourceSettingString.append(".Selected");

          Json::Path valuePath(sourceSettingString);
          Json::Value& jsonValue = valuePath.make(this->SettingCollections[0].Value);
          jsonValue = (strcmp(listProxy->GetXMLName(), currentProxyName) == 0);
          this->Modified();
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Set a property setting in the highest-priority collection.
  bool SetPropertySetting(const char* settingName, vtkSMProperty* property)
  {
    this->CreateCollectionIfNeeded();
    this->SortCollectionsIfNeeded();

    if (vtkSMIntVectorProperty* intVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property))
    {
      return this->SetPropertySetting(settingName, intVectorProperty);
    }
    else if (vtkSMDoubleVectorProperty* doubleVectorProperty =
               vtkSMDoubleVectorProperty::SafeDownCast(property))
    {
      return this->SetPropertySetting(settingName, doubleVectorProperty);
    }
    else if (vtkSMStringVectorProperty* stringVectorProperty =
               vtkSMStringVectorProperty::SafeDownCast(property))
    {
      return this->SetPropertySetting(settingName, stringVectorProperty);
    }
    else if (vtkSMProxyProperty* proxyProperty = vtkSMProxyProperty::SafeDownCast(property))
    {
      return this->SetPropertySetting(settingName, proxyProperty);
    }

    return false;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Set proxy settings to the highest-priority collection.
  bool SetProxySettings(
    vtkSMProxy* proxy, vtkSMPropertyIterator* propertyIt, bool skipPropertiesWithDynamicDomains)
  {
    if (!proxy)
    {
      return false;
    }

    std::string jsonPrefix(".");
    jsonPrefix.append(proxy->GetXMLGroup());

    return this->SetProxySettings(
      jsonPrefix.c_str(), proxy, propertyIt, skipPropertiesWithDynamicDomains);
  }

  //----------------------------------------------------------------------------
  // Description:
  // Set proxy settings in the highest-priority collection under
  // the setting prefix.
  bool SetProxySettings(const char* settingPrefix, vtkSMProxy* proxy,
    vtkSMPropertyIterator* propertyIt, bool skipPropertiesWithDynamicDomains)
  {
    if (!proxy)
    {
      return false;
    }
    this->CreateCollectionIfNeeded();
    this->SortCollectionsIfNeeded();

    double highestPriority = this->SettingCollections[0].Priority;

    // Get reference to JSON value
    const char* proxyName = proxy->GetXMLName();
    std::ostringstream settingStringStream;
    settingStringStream << settingPrefix << "." << proxyName;
    std::string settingString(settingStringStream.str());
    const char* settingCString = settingString.c_str();

    Json::Path valuePath(settingCString);
    Json::Value& proxyValue = valuePath.make(this->SettingCollections[0].Value);

    bool propertySet = false;
    vtkSmartPointer<vtkSMPropertyIterator> iter;
    if (propertyIt)
    {
      iter = propertyIt;
    }
    else
    {
      iter.TakeReference(proxy->NewPropertyIterator());
    }
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
      vtkSMProperty* property = iter->GetProperty();
      if (!property)
        continue;

      // Check to see if we save only to QSettings or to both QSettings
      // and the JSON file.
      if (property->GetHints())
      {
        vtkPVXMLElement* saveInQSettingsHint =
          property->GetHints()->FindNestedElementByName("SaveInQSettings");
        if (saveInQSettingsHint)
        {
          int bothSettings = 0;
          saveInQSettingsHint->GetScalarAttribute("both", &bothSettings);
          if (!bothSettings)
          {
            continue;
          }
        }
      }

      const char* propertyName = iter->GetKey();
      std::ostringstream propertySettingStringStream;
      propertySettingStringStream << settingStringStream.str() << "." << propertyName;
      std::string propertySettingString(propertySettingStringStream.str());
      const char* propertySettingCString = propertySettingString.c_str();

      if (strcmp(property->GetPanelVisibility(), "never") == 0 || property->GetInformationOnly() ||
        property->GetIsInternal() || property->GetNoCustomDefault())
      {
        continue;
      }
      else if (skipPropertiesWithDynamicDomains && property->HasDomainsWithRequiredProperties())
      {
        // skip properties that have domains that change at runtime. Such
        // properties are typically serialized in state files not in settings.
        continue;
      }
      else if (property->IsValueDefault())
      {
        // Remove existing JSON entry only if there is no
        // lower-priority setting.  That's because if there is a
        // lower-priority setting that is not default, we want to be
        // able to set the value back to the default in the higher
        // priority setting collection.
        const Json::Value& lowerPriorityValue =
          this->GetSettingBelowPriority(propertySettingCString, highestPriority);
        if (lowerPriorityValue.isNull())
        {
          // Allocated as done in Json::Value removeMember(const char* key).
          Json::Value removedValue;
          if (proxyValue.removeMember(property->GetXMLName(), &removedValue) &&
            !removedValue.isNull())
          {
            this->Modified();
          }
          continue;
        }
      }

      if (this->SetPropertySetting(propertySettingCString, property))
      {
        propertySet = true;
      }
    }

    // If no property was set, remove the proxy entry.
    if (!propertySet)
    {
      Json::Path parentPath(settingPrefix);
      Json::Value& parentValue = parentPath.make(this->SettingCollections[0].Value);
      parentValue.removeMember(proxyName);

      if (parentValue.empty())
      {
        std::string parentRoot, parentLeaf;
        SeparateBranchFromLeaf(settingPrefix, parentRoot, parentLeaf);
        parentRoot.append(".");
        // Need special handling when parent root is "." because
        // Json::Path::make() doesn't appear to handle it correctly.
        if (parentRoot == ".")
        {
          this->SettingCollections[0].Value.removeMember(parentLeaf);
        }
        else
        {
          Json::Path parentRootPath(parentRoot);
          Json::Value& parentRootValue = parentRootPath.make(this->SettingCollections[0].Value);
          parentRootValue.removeMember(parentLeaf);
        }
      }
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool ConvertJsonValue(const Json::Value& jsonValue, int& value)
  {
    if (!jsonValue.isNumeric())
    {
      return false;
    }
    value = jsonValue.asInt();

    return true;
  }

  //----------------------------------------------------------------------------
  bool ConvertJsonValue(const Json::Value& jsonValue, double& value)
  {
    if (!jsonValue.isNumeric())
    {
      return false;
    }

    try
    {
      value = jsonValue.asDouble();
    }
    catch (...)
    {
      vtkGenericWarningMacro(<< "Could not convert \n" << jsonValue.toStyledString());
    }

    return true;
  }

  //----------------------------------------------------------------------------
  bool ConvertJsonValue(const Json::Value& jsonValue, std::string& value)
  {
    if (!jsonValue.isString())
    {
      return false;
    }

    try
    {
      value = jsonValue.asString();
    }
    catch (...)
    {
      vtkGenericWarningMacro(<< "Could not convert \n" << jsonValue.toStyledString());
    }

    return true;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Looks for a collection that has a priority greater or equal to UserPriority.
  // If no one exists, create a collection at UserPriority.
  void CreateCollectionIfNeeded()
  {
    for (const auto& collection : this->SettingCollections)
    {
      if (collection.Priority >= vtkSMSettings::GetUserPriority())
      {
        return;
      }
    }

    ::SettingsCollection newCollection(vtkSMSettings::GetUserPriority());
    this->SettingCollections.push_back(newCollection);
    this->IsModified = true;
    this->SettingCollectionsAreSorted = false;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Sort the collections if needed
  void SortCollectionsIfNeeded()
  {
    if (!this->SettingCollectionsAreSorted)
    {
      this->SortSettingCollections();
      this->SettingCollectionsAreSorted = true;
    }
  }
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSettings);

//----------------------------------------------------------------------------
vtkSMSettings::vtkSMSettings()
{
  this->Internal = new vtkSMSettingsInternal();

  if (vtksys::SystemTools::GetEnv("PV_SETTINGS_DEBUG") != nullptr)
  {
    vtkWarningMacro("`PV_SETTINGS_DEBUG` environment variable has been deprecated."
                    "Please use `PARAVIEW_LOG_APPLICATION_VERBOSITY=INFO` instead.");
    vtkPVLogger::SetApplicationVerbosity(vtkLogger::VERBOSITY_INFO);
  }
}

//----------------------------------------------------------------------------
vtkSMSettings::~vtkSMSettings()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkSMSettings* vtkSMSettings::GetInstance()
{
  static vtkSmartPointer<vtkSMSettings> Instance;
  if (Instance.GetPointer() == nullptr)
  {
    vtkSMSettings* settings = vtkSMSettings::New();
    Instance = settings;
    settings->FastDelete();
  }

  return Instance;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::AddCollectionFromString(const std::string& settings, double priority)
{
  ::SettingsCollection collection(priority);

  vtkVLogF(
    PARAVIEW_LOG_APPLICATION_VERBOSITY(), "loading settings JSON string '%s'", settings.c_str());

  // If the settings string is empty, the JSON parser can't handle it.
  // Replace the empty string with {}
  std::string processedSettings(settings);
  if (processedSettings.empty())
  {
    processedSettings.append("{}");
  }

  // Take care of any backwards compatibility issues
  ::TransformJSON(processedSettings);

  Json::CharReaderBuilder builder;
  builder["collectComments"] = true;

  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

  const char* input = processedSettings.c_str();
  bool success = reader->parse(input, input + strlen(input), &collection.Value, nullptr);
  if (success)
  {
    this->Internal->SettingCollections.push_back(collection);
    this->Internal->SettingCollectionsAreSorted = false;
    this->Internal->Modified();
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "successfully parsed settings string");
    return true;
  }
  else
  {
    vtkErrorMacro(<< "Failed to parse settings from JSON" << endl << processedSettings << endl);
  }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::AddCollectionFromFile(const std::string& fileName, double priority)
{
  vtksys::ifstream settingsFile(fileName.c_str(), ios::in | ios::binary | ios::ate);
  if (settingsFile.is_open())
  {
    std::streampos size = settingsFile.tellg();
    settingsFile.seekg(0, ios::beg);
    int stringSize = size;
    char* settingsString = new char[stringSize + 1];
    settingsFile.read(settingsString, stringSize);
    settingsString[stringSize] = '\0';
    settingsFile.close();

    bool success = this->AddCollectionFromString(std::string(settingsString), priority);
    delete[] settingsString;

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "load settings file `%s` -- %s",
      fileName.c_str(), (success ? "succeeded" : "failed"));
    return success;
  }
  else
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "load settings file `%s` -- missing (skipped)",
      fileName.c_str());
    return false;
  }
}

//----------------------------------------------------------------------------
void vtkSMSettings::ClearAllSettings()
{
  this->Internal->SettingCollections.clear();
  this->Internal->SettingCollectionsAreSorted = false;
  this->Internal->IsModified = false;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::DistributeSettings()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH && pm->GetSymmetricMPIMode() &&
    (pm->GetNumberOfLocalPartitions() > 1))
  {
    // Broadcast settings to satellite nodes
    vtkMultiProcessController* controller = pm->GetGlobalController();

    unsigned int numberOfSettingCollections;
    if (controller->GetLocalProcessId() == 0)
    {
      // Send the number of JSON settings roots
      numberOfSettingCollections =
        static_cast<unsigned int>(this->Internal->SettingCollections.size());
      controller->Broadcast(&numberOfSettingCollections, 1, 0);

      for (size_t i = 0; i < numberOfSettingCollections; ++i)
      {
        std::string settingsString = this->Internal->SettingCollections[i].Value.toStyledString();
        unsigned int stringSize = static_cast<unsigned int>(settingsString.size()) + 1;
        controller->Broadcast(&stringSize, 1, 0);
        if (stringSize > 0)
        {
          controller->Broadcast(const_cast<char*>(settingsString.c_str()), stringSize, 0);
          controller->Broadcast(&this->Internal->SettingCollections[i].Priority, 1, 0);
        }
      }
    }
    else // Satellites
    {
      // Get the number of JSON settings roots
      controller->Broadcast(&numberOfSettingCollections, 1, 0);

      for (unsigned int i = 0; i < numberOfSettingCollections; ++i)
      {
        unsigned int stringSize = 0;
        controller->Broadcast(&stringSize, 1, 0);
        if (stringSize > 0)
        {
          char* settingsString = new char[stringSize];
          controller->Broadcast(settingsString, stringSize, 0);
          double priority = 0.0;
          controller->Broadcast(&priority, 1, 0);
          this->AddCollectionFromString(std::string(settingsString), priority);
          delete[] settingsString;
        }
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SaveSettingsToFile(const std::string& filePath)
{
  if (this->Internal->SettingCollections.empty())
  {
    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "settings empty, hence not saved.");
    return true;
  }

  this->Internal->SortCollectionsIfNeeded();

  return this->SaveSettingsToFile(filePath, this->Internal->SettingCollections[0].Priority);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SaveSettingsToFile(const std::string& filePath, double priority)
{
  if (this->Internal->SettingCollections.empty() || !this->Internal->IsModified)
  {
    // No settings to save, so we'll always succeed.
    vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(),
      (!this->Internal->SettingCollections.empty() && this->Internal->IsModified == false),
      "settings not modified, hence not saved.");
    vtkVLogIfF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), (this->Internal->SettingCollections.empty()),
      "settings empty, hence not saved.");
    return true;
  }

  // Get directory component of filePath and create it if it doesn't exist.
  std::string directory = vtksys::SystemTools::GetParentDirectory(filePath);
  vtksys::Status createdDirectory = vtksys::SystemTools::MakeDirectory(directory.c_str());
  if (!createdDirectory.IsSuccess())
  {
    vtkWarningMacro(<< "Directory '" << directory << "' does not exist and could "
                    << "not be created.");
    return false;
  }

  vtksys::ofstream settingsFile(filePath.c_str(), ios::out | ios::binary);
  if (settingsFile.is_open())
  {
    for (const auto& collection : this->Internal->SettingCollections)
    {
      if (collection.Priority == priority)
      {
        std::string output = collection.Value.toStyledString();
        settingsFile << output;
      }
    }

    this->Internal->IsModified = false;

    vtkVLogF(PARAVIEW_LOG_APPLICATION_VERBOSITY(), "saving settings to '%s'", filePath.c_str());
    return true;
  }

  vtkVLogF(
    PARAVIEW_LOG_APPLICATION_VERBOSITY(), "could not save settings to '%s'!", filePath.c_str());
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::HasSetting(const char* settingName)
{
  return this->Internal->HasSetting(settingName, vtkSMSettings::GetUserPriority());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::HasSetting(const char* settingName, double maxPriority)
{
  return this->Internal->HasSetting(settingName, maxPriority);
}

//----------------------------------------------------------------------------
unsigned int vtkSMSettings::GetSettingNumberOfElements(const char* settingName)
{
  Json::Value value = this->Internal->GetSetting(settingName);
  if (value.isArray())
  {
    return value.size();
  }

  return value.isNull() ? 0 : 1;
}

//----------------------------------------------------------------------------
int vtkSMSettings::GetSettingAsInt(const char* settingName, int defaultValue)
{
  return this->GetSettingAsInt(settingName, 0, defaultValue);
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetSettingAsDouble(const char* settingName, double defaultValue)
{
  return this->GetSettingAsDouble(settingName, 0, defaultValue);
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingAsString(
  const char* settingName, const std::string& defaultValue)
{
  return this->GetSettingAsString(settingName, 0, defaultValue);
}

//----------------------------------------------------------------------------
int vtkSMSettings::GetSettingAsInt(const char* settingName, unsigned int index, int defaultValue)
{
  std::vector<int> values;
  bool success = this->Internal->GetSetting(settingName, values, vtkSMSettings::GetUserPriority());

  if (success && index < values.size())
  {
    return values[index];
  }

  return defaultValue;
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetSettingAsDouble(
  const char* settingName, unsigned int index, double defaultValue)
{
  std::vector<double> values;
  bool success = this->Internal->GetSetting(settingName, values, vtkSMSettings::GetUserPriority());

  if (success && index < values.size())
  {
    return values[index];
  }

  return defaultValue;
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingAsString(
  const char* settingName, unsigned int index, const std::string& defaultValue)
{
  std::vector<std::string> values;
  bool success = this->Internal->GetSetting(settingName, values, vtkSMSettings::GetUserPriority());

  if (success && index < values.size())
  {
    return values[index];
  }

  return defaultValue;
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingDescription(const char* settingName)
{
  Json::Value value = this->Internal->GetSetting(settingName);
  if (!value)
  {
    return std::string();
  }

  if (value.hasComment(Json::commentBefore))
  {
    return value.getComment(Json::commentBefore);
  }

  return std::string();
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetPropertySetting(vtkSMProperty* property)
{
  return this->GetPropertySetting(property, vtkSMSettings::GetUserPriority());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetPropertySetting(vtkSMProperty* property, double maxPriority)
{
  if (!property)
  {
    return false;
  }

  std::string jsonPrefix(".");

  // TODO - not sure about the GetParent() part
  jsonPrefix.append(property->GetParent()->GetXMLGroup());

  return this->GetPropertySetting(jsonPrefix.c_str(), property, maxPriority);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetPropertySetting(const char* prefix, vtkSMProperty* property)
{
  return this->GetPropertySetting(prefix, property, vtkSMSettings::GetUserPriority());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetPropertySetting(
  const char* prefix, vtkSMProperty* property, double maxPriority)
{
  vtkSMProxy* parent = property->GetParent();

  std::string jsonPrefix(prefix);
  jsonPrefix.append(".");
  jsonPrefix.append(parent->GetXMLName());
  jsonPrefix.append(".");
  jsonPrefix.append(parent->GetPropertyName(property));

  return this->Internal->GetPropertySetting(jsonPrefix.c_str(), property, maxPriority);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetProxySettings(vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return false;
  }

  std::string jsonPrefix(".");
  jsonPrefix.append(proxy->GetXMLGroup());

  return this->GetProxySettings(jsonPrefix.c_str(), proxy);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetProxySettings(vtkSMProxy* proxy, double maxPriority)
{
  if (!proxy)
  {
    return false;
  }

  std::string jsonPrefix(".");
  jsonPrefix.append(proxy->GetXMLGroup());

  return this->GetProxySettings(jsonPrefix.c_str(), proxy, maxPriority);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetProxySettings(const char* prefix, vtkSMProxy* proxy)
{
  return this->GetProxySettings(prefix, proxy, vtkSMSettings::GetUserPriority());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetProxySettings(const char* prefix, vtkSMProxy* proxy, double maxPriority)
{
  return this->Internal->GetProxySettings(prefix, proxy, maxPriority);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, int value)
{
  this->SetSetting(settingName, 0, value);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, double value)
{
  this->SetSetting(settingName, 0, value);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, const std::string& value)
{
  this->SetSetting(settingName, 0, value);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, unsigned int index, int value)
{
  std::vector<int> values;
  this->Internal->GetSetting(settingName, values, vtkSMSettings::GetUserPriority());
  if (values.size() <= index)
  {
    values.resize(index + 1, 0);
  }

  values[index] = value;
  this->Internal->SetSetting(settingName, values);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, unsigned int index, double value)
{
  std::vector<double> values;
  this->Internal->GetSetting(settingName, values, vtkSMSettings::GetUserPriority());
  if (values.size() <= index)
  {
    values.resize(index + 1, 0);
  }

  values[index] = value;
  this->Internal->SetSetting(settingName, values);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(
  const char* settingName, unsigned int index, const std::string& value)
{
  std::vector<std::string> values;
  this->Internal->GetSetting(settingName, values, vtkSMSettings::GetUserPriority());
  if (values.size() <= index)
  {
    values.resize(index + 1, "");
  }

  values[index] = value;
  this->Internal->SetSetting(settingName, values);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetProxySettings(
  vtkSMProxy* proxy, vtkSMPropertyIterator* propertyIt, bool skipPropertiesWithDynamicDomains)
{
  this->Internal->SetProxySettings(proxy, propertyIt, skipPropertiesWithDynamicDomains);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetProxySettings(const char* prefix, vtkSMProxy* proxy,
  vtkSMPropertyIterator* propertyIt, bool skipPropertiesWithDynamicDomains)
{
  this->Internal->SetProxySettings(prefix, proxy, propertyIt, skipPropertiesWithDynamicDomains);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSettingDescription(const char* settingName, const char* description)
{
  Json::Path settingPath(settingName);
  Json::Value& settingValue = settingPath.make(this->Internal->SettingCollections[0].Value);
  settingValue.setComment(std::string(description), Json::commentBefore);
}

//----------------------------------------------------------------------------
namespace
{
template <class T>
Json::Value vtkConvertXMLElementToJSON(
  vtkSMVectorProperty* vp, const std::vector<vtkSmartPointer<vtkPVXMLElement>>& elements)
{
  // Since we need to handle enumeration domain :/.
  auto enumDomain = vp->FindDomain<vtkSMEnumerationDomain>();
  Json::Value value(Json::arrayValue);
  for (size_t cc = 0; cc < elements.size(); ++cc)
  {
    T xmlValue;
    elements[cc]->GetScalarAttribute("value", &xmlValue);
    const char* txt = enumDomain ? enumDomain->GetEntryTextForValue(xmlValue) : nullptr;
    if (txt)
    {
      value[static_cast<unsigned int>(cc)] = Json::Value(txt);
    }
    else
    {
      value[static_cast<unsigned int>(cc)] = Json::Value(xmlValue);
    }
  }
  if (vp->GetNumberOfElements() == 1 && vp->GetRepeatCommand() == 0 && value.size() == 1)
  {
    return value[0];
  }
  return value;
}

// We need a specialized template for `vtkIdType`, if compiling
// with `VTK_USE_64BIT_IDS=ON`, because JSONcpp >= 1.7.7 switched
// from at-least width (long long int) to fixed width (Int64)
// integers.  If compiling with `VTK_USE_64BIT_IDS=OFF` this is
// not needed, because `vtkIdType` represents plain (Int32)
// integers in this case.
//
// See: https://gitlab.kitware.com/paraview/paraview/-/issues/16938
#ifdef VTK_USE_64BIT_IDS
template <>
Json::Value vtkConvertXMLElementToJSON<vtkIdType>(
  vtkSMVectorProperty* vp, const std::vector<vtkSmartPointer<vtkPVXMLElement>>& elements)
{
  // Since we need to handle enumeration domain :/.
  auto enumDomain = vp->FindDomain<vtkSMEnumerationDomain>();
  Json::Value value(Json::arrayValue);
  for (size_t cc = 0; cc < elements.size(); ++cc)
  {
    vtkIdType xmlValue;
    elements[cc]->GetScalarAttribute("value", &xmlValue);
    const char* txt = enumDomain ? enumDomain->GetEntryTextForValue(xmlValue) : nullptr;
    if (txt)
    {
      value[static_cast<unsigned int>(cc)] = Json::Value(txt);
    }
    else
    {
      // We need to cast from `vtkIdType` to `Int64`-type explicitly.
      value[static_cast<unsigned int>(cc)] = Json::Value(static_cast<Json::Value::Int64>(xmlValue));
    }
  }
  if (vp->GetNumberOfElements() == 1 && vp->GetRepeatCommand() == 0 && value.size() == 1)
  {
    return value[0];
  }
  return value;
}
#endif // VTK_USE_64BIT_IDS

template <>
Json::Value vtkConvertXMLElementToJSON<std::string>(
  vtkSMVectorProperty* vp, const std::vector<vtkSmartPointer<vtkPVXMLElement>>& elements)
{
  Json::Value value(Json::arrayValue);
  for (size_t cc = 0; cc < elements.size(); ++cc)
  {
    value[static_cast<unsigned int>(cc)] = Json::Value(elements[cc]->GetAttribute("value"));
  }
  if (vp->GetNumberOfElements() == 1 && vp->GetRepeatCommand() == 0 && value.size() == 1)
  {
    return value[0];
  }
  return value;
}
}

//---------------------------------------------------------------------------
Json::Value vtkSMSettings::SerializeAsJSON(
  vtkSMProxy* proxy, vtkSMPropertyIterator* iter /*=nullptr*/)
{
  if (proxy == nullptr)
  {
    return Json::Value();
  }
  vtkSmartPointer<vtkPVXMLElement> xml;
  xml.TakeReference(proxy->SaveXMLState(/*parent=*/nullptr, iter));
  Json::Value root(Json::objectValue);
  for (unsigned int cc = 0, max = xml->GetNumberOfNestedElements(); cc < max; ++cc)
  {
    vtkPVXMLElement* propXML = xml->GetNestedElement(cc);
    if (propXML && propXML->GetName() && strcmp(propXML->GetName(), "Property") == 0)
    {
      const char* pname = propXML->GetAttribute("name");
      int number_of_elements = 0;
      if (!pname || !propXML->GetScalarAttribute("number_of_elements", &number_of_elements))
      {
        continue;
      }

      vtkSMProperty* prop = proxy->GetProperty(pname);
      if (prop == nullptr || prop->GetInformationOnly())
      {
        continue;
      }

      // parse "Element".
      std::vector<vtkSmartPointer<vtkPVXMLElement>> valueElements;
      valueElements.resize(number_of_elements);
      for (unsigned int kk = 0, maxkk = propXML->GetNumberOfNestedElements(); kk < maxkk; ++kk)
      {
        int index = 0;
        vtkPVXMLElement* elemXML = propXML->GetNestedElement(kk);
        if (elemXML && elemXML->GetName() && strcmp(elemXML->GetName(), "Element") == 0 &&
          elemXML->GetScalarAttribute("index", &index) && index >= 0 && index <= number_of_elements)
        {
          valueElements[index] = elemXML;
        }
      }
      vtkSMVectorPropertyTemplateMacro(prop,
                                       root[pname] = ::vtkConvertXMLElementToJSON<SM_TT>(
                                         vtkSMVectorProperty::SafeDownCast(prop), valueElements););
    }
  }
  return root;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::DeserializeFromJSON(vtkSMProxy* proxy, const Json::Value& value)
{
  if (!proxy || !value)
  {
    return true;
  }

  if (!value.isObject())
  {
    vtkGenericWarningMacro("Invalid JSON type. Expected an 'object'.");
    return false;
  }

  vtkNew<vtkPVXMLElement> xml;
  xml->SetName("Proxy");
  xml->AddAttribute("group", proxy->GetXMLGroup());
  xml->AddAttribute("name", proxy->GetXMLName());
  for (Json::Value::const_iterator iter = value.begin(); iter != value.end(); ++iter)
  {
    vtkSMProperty* prop = proxy->GetProperty(iter.name().c_str());
    if (!prop)
    {
      continue;
    }
    // Since we need to handle enumeration domain :/.
    auto enumDomain = prop->FindDomain<vtkSMEnumerationDomain>();

    vtkNew<vtkPVXMLElement> propXML;
    propXML->SetName("Property");
    propXML->AddAttribute("name", iter.name().c_str());
    if ((*iter).isArray() || (*iter).isObject())
    {
      propXML->AddAttribute("number_of_elements", static_cast<int>((*iter).size()));
      int index = 0;
      for (Json::Value::const_iterator elemIter = (*iter).begin(); elemIter != (*iter).end();
           ++elemIter, ++index)
      {
        vtkNew<vtkPVXMLElement> elemXML;
        elemXML->SetName("Element");
        elemXML->AddAttribute("index", index);
        std::string elemValue = (*elemIter).asString();
        if (enumDomain && enumDomain->HasEntryText(elemValue.c_str()))
        {
          std::ostringstream stream;
          stream << enumDomain->GetEntryValueForText(elemValue.c_str());
          elemValue = stream.str();
        }
        elemXML->AddAttribute("value", elemValue.c_str());
        propXML->AddNestedElement(elemXML.GetPointer());
      }
    }
    else
    {
      propXML->AddAttribute("number_of_elements", "1");
      vtkNew<vtkPVXMLElement> elemXML;
      elemXML->SetName("Element");
      elemXML->AddAttribute("index", "0");
      std::string elemValue = (*iter).asString();
      if (enumDomain && enumDomain->HasEntryText(elemValue.c_str()))
      {
        std::ostringstream stream;
        stream << enumDomain->GetEntryValueForText(elemValue.c_str());
        elemValue = stream.str();
      }
      elemXML->AddAttribute("value", elemValue.c_str());
      propXML->AddNestedElement(elemXML.GetPointer());
    }
    xml->AddNestedElement(propXML.GetPointer());
  }
  return (proxy->LoadXMLState(xml.GetPointer(), nullptr) != 0);
}

//----------------------------------------------------------------------------
void vtkSMSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "SettingCollections:\n";
  for (size_t i = 0; i < this->Internal->SettingCollections.size(); ++i)
  {
    os << indent << indent << "Root " << i << ":\n";
    os << indent << indent << indent
       << "Priority: " << this->Internal->SettingCollections[i].Priority << "\n";
    std::stringstream ss(this->Internal->SettingCollections[i].Value.toStyledString());
    std::string line;
    while (std::getline(ss, line))
    {
      os << indent << indent << indent << line << "\n";
    }
  }
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetUserPriority()
{
  return 1000.;
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetApplicationPriority()
{
  return vtkSMSettings::GetUserPriority() / 2;
}
