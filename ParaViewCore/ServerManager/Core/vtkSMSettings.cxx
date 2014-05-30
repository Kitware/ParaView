/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSettings.h"

#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"

#include <vtksys/ios/sstream>
#include "vtk_jsoncpp.h"

#include <algorithm>
#include <cfloat>

//----------------------------------------------------------------------------
namespace {
class SettingsCollection {
public:
  Json::Value Value;
  double      Priority;
};

bool SortByPriority(const SettingsCollection & r1,
                    const SettingsCollection & r2)
{
  return (r1.Priority > r2.Priority);
}

} // end anonymous namespace

class vtkSMSettings::vtkSMSettingsInternal {
public:
  std::vector< SettingsCollection > SettingCollections;
  bool SettingCollectionsAreSorted;

  //----------------------------------------------------------------------------
  // Description:
  // Sort setting collections by priority, from highest to lowest
  void SortSettingCollections()
  {
    // Sort the settings roots by priority (highest to lowest)
    std::stable_sort(this->SettingCollections.begin(), this->SettingCollections.end(), SortByPriority);
    this->SettingCollectionsAreSorted = true;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Splits a JSON path into branch and leaf components. This is needed
  // to build trees with the JsonCpp library.
  static void SeparateBranchFromLeaf(const char* jsonPath, std::string & root, std::string & leaf)
  {
    root.clear();
    leaf.clear();

    // Chop off leaf setting
    std::string jsonPathString(jsonPath);
    size_t lastPeriod = jsonPathString.find_last_of('.');
    root = jsonPathString.substr(0, lastPeriod);
    leaf = jsonPathString.substr(lastPeriod+1);
  }

  //----------------------------------------------------------------------------
  // Description:
  // See if given setting is defined
  bool HasSetting(const char* settingName)
  {
    Json::Value value = this->GetSetting(settingName);

    return !value.isNull();
  }

  //----------------------------------------------------------------------------
  // Description: Get a Json::Value given a setting name. Returns the
  // highest-priority setting defined in the setting collections, and
  // null if it isn't defined in any of the collections.
  //
  // String format is:
  // "." => root node
  // ".[n]" => elements at index 'n' of root node (an array value)
  // ".name" => member named 'name' of root node (an object value)
  // ".name1.name2.name3"
  // ".[0][1][2].name1[3]"
  const Json::Value & GetSetting(const char* settingName)
  {
    return this->GetSettingAtOrBelowPriority(settingName, VTK_DOUBLE_MAX);
  }

  //----------------------------------------------------------------------------
  const Json::Value & GetSettingAtOrBelowPriority(const char* settingName,
                                                  double priority)
  {
    if (!this->SettingCollectionsAreSorted)
      {
      this->SortSettingCollections();
      }

    // Iterate over settings, checking higher priority settings first
    for (size_t i = 0; i < this->SettingCollections.size(); ++i)
      {
      if (this->SettingCollections[i].Priority > priority)
        {
        continue;
        }

      Json::Path settingPath(settingName);
      const Json::Value & setting = settingPath.resolve(this->SettingCollections[i].Value);
      if (!setting.isNull())
        {
        return setting;
        }
      }

    return Json::Value::null;
  }

  //----------------------------------------------------------------------------
  template< typename T >
  bool GetSetting(const char* settingName, std::vector<T> & values)
  {
    values.clear();

    Json::Value setting = this->GetSetting(settingName);
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
  bool GetPropertySetting(const char* settingName,
                          vtkSMIntVectorProperty* property)
  {
    if (!property)
      {
      return false;
      }

    vtkSMDomain* domain = property->FindDomain("vtkSMEnumerationDomain");
    vtkSMEnumerationDomain* enumDomain = vtkSMEnumerationDomain::SafeDownCast(domain);
    if (enumDomain)
      {
      // The enumeration property could be either text or value
      const Json::Value & jsonValue = this->GetSetting(settingName);
      int enumValue;
      bool hasInt = this->ConvertJsonValue(jsonValue, enumValue);
      if (hasInt)
        {
        property->SetElement(0, enumValue);
        }
      else
        {
        std::string stringValue;
        bool hasString = this->ConvertJsonValue(jsonValue, stringValue);
        if (hasString && enumDomain->HasEntryText(stringValue.c_str()))
          {
          enumValue = enumDomain->GetEntryValueForText(stringValue.c_str());
          property->SetElement(0, enumValue);
          }
        }
      }
    else
      {
      std::vector<int> vector;
      if (!this->GetSetting(settingName, vector) ||
          vector.size() != property->GetNumberOfElements())
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
      }

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetPropertySetting(const char* settingName,
                          vtkSMDoubleVectorProperty* property)
  {
    std::vector<double> vector;
    if (!property || !this->GetSetting(settingName, vector))
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
  bool GetPropertySetting(const char* settingName,
                          vtkSMStringVectorProperty* property)
  {
    std::vector<std::string> vector;
    if (!property || !this->GetSetting(settingName, vector))
      {
      return false;
      }

    vtkSmartPointer<vtkStringList> stringList = vtkSmartPointer<vtkStringList>::New();
    for (size_t i = 0; i < vector.size(); ++i)
      {
      vtkStdString vtk_string(vector[i]);
      stringList->AddString(vtk_string);
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
  bool GetPropertySetting(const char* settingName, vtkSMInputProperty* property)
  {
    vtkSMDomain * domain = property->GetDomain( "proxy_list" );
    vtkSMProxyListDomain * proxyListDomain = NULL;
    if ((proxyListDomain = vtkSMProxyListDomain::SafeDownCast(domain)))
      {
      // Now check whether this proxy is the one we want
      std::string sourceSettingString(settingName);
      sourceSettingString.append(".Selected");

      std::string sourceName;
      if (this->HasSetting(sourceSettingString.c_str()))
        {
        std::vector<std::string> selectedString;
        this->GetSetting(sourceSettingString.c_str(), selectedString);
        if (selectedString.size() > 0)
          {
          sourceName = selectedString[0];
          }
        }
      else
        {
        return false;
        }

      for (unsigned int ip = 0; ip < proxyListDomain->GetNumberOfProxies(); ++ip)
        {
        vtkSMProxy* listProxy = proxyListDomain->GetProxy(ip);
        if (listProxy)
          {
          // Recurse on the proxy
          bool success = this->GetProxySettings(settingName, listProxy);
          if (!success)
            {
            return false;
            }

          // Now check whether it was selected
          if (listProxy->GetXMLName() == sourceName)
            {
            // \TODO - probably not exactly what we need to do
            property->SetInputConnection(0, listProxy, 0);
            }
          }
        }
      }

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetProxySettings(const char* settingPrefix, vtkSMProxy* proxy)
  {
    if (!proxy)
      {
      std::cout << "Null proxy\n";
      return false;
      }

    bool overallSuccess = true;

    vtkSMPropertyIterator * iter = proxy->NewPropertyIterator();
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      vtkSMProperty* property = iter->GetProperty();
      if (!property)
        {
        continue;
        }

      if (proxy->GetXMLName() && property->GetXMLName() &&
          !property->GetNoCustomDefault())
        {
        // Build the JSON reference string
        vtksys_ios::ostringstream settingStringStream;
        settingStringStream << settingPrefix
                            << "." << proxy->GetXMLName()
                            << "." << property->GetXMLName();

        const std::string settingString = settingStringStream.str();
        const char* settingName = settingString.c_str();
        if (this->HasSetting(settingName))
          {
          bool success = false;
          if (vtkSMIntVectorProperty* intVectorProperty =
              vtkSMIntVectorProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(settingName, intVectorProperty);
            }
          else if (vtkSMDoubleVectorProperty* doubleVectorProperty =
                   vtkSMDoubleVectorProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(settingName, doubleVectorProperty);
            }
          else if (vtkSMStringVectorProperty* stringVectorProperty =
                   vtkSMStringVectorProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(settingName, stringVectorProperty);
            }
          else if (vtkSMInputProperty* inputProperty =
                   vtkSMInputProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(settingName, inputProperty);
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

    iter->Delete();

    return overallSuccess;
  }

  //----------------------------------------------------------------------------
  template< typename T >
  void SetSetting(const char* settingName, const std::vector< T > & values)
  {
    this->CreateCollectionIfNeeded();

    // Just set settings in the highest-priority settings group for now.
    std::string root, leaf;
    this->SeparateBranchFromLeaf(settingName, root, leaf);

    Json::Path settingPath(root.c_str());
    Json::Value & jsonValue = settingPath.make(this->SettingCollections[0].Value);
    jsonValue[leaf] = Json::Value::null;

    if (values.size() > 1)
      {
      jsonValue[leaf].resize(
        static_cast<Json::Value::ArrayIndex>(values.size()));

      for (size_t i = 0; i < values.size(); ++i)
        {
        jsonValue[leaf][static_cast<Json::Value::ArrayIndex>(i)] = values[i];
        }
      }
    else
      {
      jsonValue[leaf] = values[0];
      }
  }

  //----------------------------------------------------------------------------
  void SetPropertySetting(const char* settingName,
                          vtkSMIntVectorProperty* property)
  {
    Json::Path valuePath(settingName);
    Json::Value & jsonValue = valuePath.make(this->SettingCollections[0].Value);
    if (property->GetNumberOfElements() == 1)
      {
      jsonValue = property->GetElement(0);
      }
    else
      {
      jsonValue.resize(property->GetNumberOfElements());
      for (unsigned int i = 0; i < property->GetNumberOfElements(); ++i)
        {
        jsonValue[i] = property->GetElement(i);
        }
      }
  }

  //----------------------------------------------------------------------------
  void SetPropertySetting(const char* settingName,
                          vtkSMDoubleVectorProperty* property)
  {
    Json::Path valuePath(settingName);
    Json::Value & jsonValue = valuePath.make(this->SettingCollections[0].Value);
    if (property->GetNumberOfElements() == 1)
      {
      jsonValue = property->GetElement(0);
      }
    else
      {
      jsonValue.resize(property->GetNumberOfElements());
      for (unsigned int i = 0; i < property->GetNumberOfElements(); ++i)
        {
        jsonValue[i] = property->GetElement(i);
        }
      }
  }

  //----------------------------------------------------------------------------
  void SetPropertySetting(const char* settingName,
                          vtkSMStringVectorProperty* property)
  {
    Json::Path valuePath(settingName);
    Json::Value & jsonValue = valuePath.make(this->SettingCollections[0].Value);
    if (property->GetNumberOfElements() == 1)
      {
      jsonValue = property->GetElement(0);
      }
    else
      {
      jsonValue.resize(property->GetNumberOfElements());
      for (unsigned int i = 0; i < property->GetNumberOfElements(); ++i)
        {
        jsonValue[i] = property->GetElement(i);
        }
      }
  }

  //----------------------------------------------------------------------------
  void SetPropertySetting(const char* settingName,
                          vtkSMInputProperty* property)
  {
    Json::Path valuePath(settingName);
    std::cerr << "Unhandled property '" << property->GetXMLName() << "' of type '"
              << property->GetClassName() << "'\n";
  }

  //----------------------------------------------------------------------------
  // Description:
  // Save a property setting to the highest-priority collection.
  void SetPropertySetting(const char* settingName, vtkSMProperty* property)
  {
    this->CreateCollectionIfNeeded();

    if (vtkSMIntVectorProperty* intVectorProperty =
        vtkSMIntVectorProperty::SafeDownCast(property))
      {
      this->SetPropertySetting(settingName, intVectorProperty);
      }
    else if (vtkSMDoubleVectorProperty* doubleVectorProperty =
             vtkSMDoubleVectorProperty::SafeDownCast(property))
      {
      this->SetPropertySetting(settingName, doubleVectorProperty);
      }
    else if (vtkSMStringVectorProperty* stringVectorProperty =
             vtkSMStringVectorProperty::SafeDownCast(property))
      {
      this->SetPropertySetting(settingName, stringVectorProperty);
      }
    else if (vtkSMInputProperty* inputProperty =
             vtkSMInputProperty::SafeDownCast(property))
      {
      this->SetPropertySetting(settingName, inputProperty);
      }
  }

  //----------------------------------------------------------------------------
  // Description:
  // Save proxy settings to the highest-priority collection.
  void SetProxySettings(vtkSMProxy* proxy)
  {
    if (!proxy)
      {
      return;
      }

    std::string jsonPrefix(".");
    jsonPrefix.append(proxy->GetXMLGroup());

    this->SetProxySettings(jsonPrefix.c_str(), proxy);
  }

  //----------------------------------------------------------------------------
  // Description:
  // Save proxy settings in the highest-priority collection under
  // the setting prefix.
  void SetProxySettings(const char* settingPrefix, vtkSMProxy* proxy)
  {
    if (!proxy)
      {
      return;
      }

    this->CreateCollectionIfNeeded();

    double highestPriority = this->SettingCollections[0].Priority;

    // Get reference to JSON value
    vtksys_ios::ostringstream settingStringStream;
    settingStringStream << settingPrefix << "." << proxy->GetXMLName();
    std::string settingString(settingStringStream.str());
    const char* settingCString = settingString.c_str();

    Json::Path valuePath(settingCString);
    Json::Value & proxyValue = valuePath.make(this->SettingCollections[0].Value);

    bool propertySet = false;
    vtkSmartPointer<vtkSMPropertyIterator> iter;
    iter.TakeReference(proxy->NewPropertyIterator());
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      vtkSMProperty* property = iter->GetProperty();
      if (!property) continue;

      vtksys_ios::ostringstream propertySettingStringStream;
      propertySettingStringStream << settingStringStream.str() << "."
                                  << property->GetXMLName();
      std::string propertySettingString(propertySettingStringStream.str());
      const char* propertySettingCString = propertySettingString.c_str();

      if (strcmp(property->GetPanelVisibility(), "never") == 0 ||
          property->GetInformationOnly() ||
          property->GetIsInternal() ||
          property->GetNoCustomDefault())
        {
        continue;
        }
      else if (property->IsValueDefault())
        {
        // Remove existing JSON entry only if there is no lower-priority setting
        if (this->GetSettingAtOrBelowPriority(propertySettingCString,
                                              highestPriority).isNull())
          {
          proxyValue.removeMember(property->GetXMLName());
          continue;
          }
        }

      this->SetPropertySetting(propertySettingCString, property);
      propertySet = true;
      }

    // Check if the proxy Json::Value is empty. If so, remove it.
    if (!propertySet)
      {
      Json::Path parentPath(settingPrefix);
      Json::Value & parentValue = parentPath.make(this->SettingCollections[0].Value);
      parentValue.removeMember(proxy->GetXMLName());
      }
  }

  //----------------------------------------------------------------------------
  bool ConvertJsonValue(const Json::Value & jsonValue, int & value)
  {
    if (!jsonValue.isNumeric())
      {
      return false;
      }
    value = jsonValue.asInt();

    return true;
  }

  //----------------------------------------------------------------------------
  bool ConvertJsonValue(const Json::Value & jsonValue, double & value)
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
      std::cout << "Could not convert \n" << jsonValue.toStyledString() << "\n";
      }

    return true;
  }

  //----------------------------------------------------------------------------
  bool ConvertJsonValue(const Json::Value & jsonValue, std::string & value)
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
      std::cout << "Could not convert \n" << jsonValue.toStyledString() << "\n";
      }

    return true;
  }

  //----------------------------------------------------------------------------
  // Description:
  // Ensure that at least one collection exists so that when settings are set,
  // there is a place to store them. If a collection needs to be created, its
  // priority is set to DOUBLE_MAX.
  void CreateCollectionIfNeeded()
  {
    if (this->SettingCollections.size() == 0)
      {
      SettingsCollection newCollection;
      newCollection.Priority = VTK_DOUBLE_MAX;
      this->SettingCollections.push_back(newCollection);
      }
  }


};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSettings);

//----------------------------------------------------------------------------
vtkSMSettings::vtkSMSettings()
{
  this->Internal = new vtkSMSettingsInternal();
  this->Internal->SettingCollectionsAreSorted = false;
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
  if (Instance.GetPointer() == NULL)
    {
    vtkSMSettings* settings = vtkSMSettings::New();
    Instance = settings;
    settings->FastDelete();
    }

  return Instance;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::AddCollectionFromString(const std::string & settings,
                                            double priority)
{
  SettingsCollection collection;
  collection.Priority = priority;

  // If the settings string is empty, the JSON parser can't handle it.
  // Replace the empty string with {}
  std::string processedSettings(settings);
  if (processedSettings == "")
    {
    processedSettings.append("{}");
    }

  // Parse the user settings
  Json::Reader reader;
  bool success = reader.parse(processedSettings, collection.Value, true);
  if (success)
    {
    this->Internal->SettingCollections.push_back(collection);
    this->Internal->SettingCollectionsAreSorted = false;
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::AddCollectionFromFile(const std::string & fileName,
                                          double priority)
{
  std::string settingsFileName(fileName);
  std::ifstream settingsFile(settingsFileName.c_str(), ios::in | ios::binary | ios::ate);
  if (settingsFile.is_open())
    {
    std::streampos size = settingsFile.tellg();
    settingsFile.seekg(0, ios::beg);
    int stringSize = size;
    char * settingsString = new char[stringSize+1];
    settingsFile.read(settingsString, stringSize);
    settingsString[stringSize] = '\0';
    settingsFile.close();

    bool success = this->AddCollectionFromString(std::string(settingsString),
                                               priority);
    delete[] settingsString;

    return success;
    }
  else
    {
    std::string emptyString;
    return this->AddCollectionFromString(emptyString, priority);
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMSettings::ClearAllSettings()
{
  this->Internal->SettingCollections.clear();
  this->Internal->SettingCollectionsAreSorted = false;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::DistributeSettings()
{
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
      pm->GetSymmetricMPIMode() &&
      (pm->GetNumberOfLocalPartitions() > 1))
    {
    // Broadcast settings to satellite nodes
    vtkMultiProcessController * controller = pm->GetGlobalController();

    unsigned int numberOfSettingCollections;
    if (controller->GetLocalProcessId() == 0)
      {
      // Send the number of JSON settings roots
      numberOfSettingCollections = static_cast<unsigned int>(this->Internal->SettingCollections.size());
      controller->Broadcast(&numberOfSettingCollections, 1, 0);

      for (size_t i = 0; i < numberOfSettingCollections; ++i)
        {
        std::string settingsString = this->Internal->SettingCollections[i].Value.toStyledString();
        unsigned int stringSize = static_cast<unsigned int>(settingsString.size())+1;
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
bool vtkSMSettings::SaveSettings(const std::string & filePath)
{
  if (this->Internal->SettingCollections.size() == 0)
    {
    // No settings to save, so we'll always succeed.
    return true;
    }

  std::ofstream settingsFile(filePath.c_str(), ios::out | ios::binary );
  if (settingsFile.is_open())
    {
    std::string output = this->Internal->SettingCollections[0].Value.toStyledString();
    settingsFile << output;
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::HasSetting(const char* settingName)
{
  return this->Internal->HasSetting(settingName);
}

//----------------------------------------------------------------------------
unsigned int vtkSMSettings::GetSettingNumberOfElements(const char* settingName)
{
  Json::Value value = this->Internal->GetSetting(settingName);
  if (value.isArray())
    {
    return value.size();
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkSMSettings::GetSettingAsInt(const char* settingName,
                                   int defaultValue)
{
  return this->GetSettingAsInt(settingName, 0, defaultValue);
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetSettingAsDouble(const char* settingName,
                                         double defaultValue)
{
  return this->GetSettingAsDouble(settingName, 0, defaultValue);
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingAsString(const char* settingName,
                                              const std::string & defaultValue)
{
  return this->GetSettingAsString(settingName, 0, defaultValue);
}

//----------------------------------------------------------------------------
int vtkSMSettings::GetSettingAsInt(const char* settingName,
                                         unsigned int index,
                                         int defaultValue)
{
  std::vector<int> values;
  bool success = this->Internal->GetSetting(settingName, values);

  if (success && index < values.size())
    {
    return values[index];
    }

  return defaultValue;
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetSettingAsDouble(const char* settingName,
                                         unsigned int index,
                                         double defaultValue)
{
  std::vector<double> values;
  bool success = this->Internal->GetSetting(settingName, values);

  if (success && index < values.size())
    {
    return values[index];
    }

  return defaultValue;
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingAsString(const char* settingName,
                                              unsigned int index,
                                              const std::string & defaultValue)
{
  std::vector<std::string> values;
  bool success = this->Internal->GetSetting(settingName, values);

  if (success && index < values.size())
    {
    return values[index];
    }

  return defaultValue;
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingDescription(const char *settingName)
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
bool vtkSMSettings::GetProxySettings(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return false;
    }

  std::string jsonPrefix(".");
  jsonPrefix.append(proxy->GetXMLGroup());

  return this->Internal->GetProxySettings(jsonPrefix.c_str(), proxy);
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
void vtkSMSettings::SetSetting(const char* settingName, const std::string & value)
{
  this->SetSetting(settingName, 0, value);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, unsigned int index, int value)
{
  std::vector<int> values;
  this->Internal->GetSetting(settingName, values);
  if (values.size() <= index)
    {
    values.resize(index+1, 0);
    }

  values[index] = value;
  this->Internal->SetSetting(settingName, values);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, unsigned int index, double value)
{
  std::vector<double> values;
  this->Internal->GetSetting(settingName, values);
  if (values.size() <= index)
    {
    values.resize(index+1, 0);
    }

  values[index] = value;
  this->Internal->SetSetting(settingName, values);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSetting(const char* settingName, unsigned int index, const std::string & value)
{
  std::vector<std::string> values;
  this->Internal->GetSetting(settingName, values);
  if (values.size() <= index)
    {
    values.resize(index+1, "");
    }

  values[index] = value;
  this->Internal->SetSetting(settingName, values);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetProxySettings(vtkSMProxy* proxy)
{
  this->Internal->SetProxySettings(proxy);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSettingDescription(const char* settingName, const char* description)
{
  Json::Path settingPath(settingName);
  Json::Value & settingValue = settingPath.make(this->Internal->SettingCollections[0].Value);
  settingValue.setComment(description, Json::commentBefore);
}

//----------------------------------------------------------------------------
void vtkSMSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "SettingCollections:\n";
  for (size_t i = 0; i < this->Internal->SettingCollections.size(); ++i)
    {
    os << indent << indent << "Root " << i << ":\n";
    os << indent << indent << indent << "Priority: "
       << this->Internal->SettingCollections[i].Priority << "\n";
    std::stringstream ss(this->Internal->SettingCollections[i].Value.toStyledString());
    std::string line;
    while (std::getline(ss, line))
      {
      os << indent << indent << indent << line << "\n";
      }
    }
}
