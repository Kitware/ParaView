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
#include "vtkPVSession.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
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

//----------------------------------------------------------------------------
class vtkSMSettings::vtkSMSettingsInternal {
public:
  Json::Value UserSettingsJSONRoot;
  Json::Value SiteSettingsJSONRoot;

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
  // Description:
  // See if given setting is defined in user settings
  bool HasUserSetting(const char* settingName)
  {
    Json::Value value = this->GetUserSetting(settingName);

    return !value.isNull();
  }

  //----------------------------------------------------------------------------
  // Description:
  // See if given setting is defined in user settings
  bool HasSiteSetting(const char* settingName)
  {
    Json::Value value = this->GetSiteSetting(settingName);

    return !value.isNull();
  }

  //----------------------------------------------------------------------------
  // Description:
  // Get a Json::Value given a string. Returns the setting defined in the user
  // settings file if it is defined, falls back to the setting defined in the
  // site settings file if it is defined, and null if it isn't defined in
  // either the user or settings file.
  //
  // String format is:
  // "." => root node
  // ".[n]" => elements at index 'n' of root node (an array value)
  // ".name" => member named 'name' of root node (an object value)
  // ".name1.name2.name3"
  // ".[0][1][2].name1[3]"
  const Json::Value & GetSetting(const char* settingName)
  {
    // Try user-specific settings first
    const Json::Value & userSetting = this->GetUserSetting(settingName);
    if (!userSetting)
      {
      const Json::Value & siteSetting = this->GetSiteSetting(settingName);
      return siteSetting;
      }

    return userSetting;
  }

  //----------------------------------------------------------------------------
  const Json::Value & GetUserSetting(const char* settingName)
  {
    // Convert setting string to path
    const std::string settingString(settingName);
    Json::Path userSettingsPath(settingString);

    const Json::Value & userSetting = userSettingsPath.resolve(this->UserSettingsJSONRoot);

    return userSetting;
  }

  //----------------------------------------------------------------------------
  const Json::Value & GetSiteSetting(const char* settingName)
  {
    // Convert setting string to path
    Json::Path siteSettingsPath(settingName);
    const Json::Value & siteSetting = siteSettingsPath.resolve(this->SiteSettingsJSONRoot);

    return siteSetting;
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
        property->SetNumberOfElements(vector.size());
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
      property->SetNumberOfElements(vector.size());
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
      property->SetNumberOfElements(vector.size());
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

      if (proxy->GetXMLName() && property->GetXMLName())
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
    // Just set user settings for now.
    this->SetUserSetting(settingName, values);
  }

  //----------------------------------------------------------------------------
  template< typename T >
  void SetUserSetting(const char* settingName, const std::vector< T > & values)
  {
    std::string root, leaf;
    this->SeparateBranchFromLeaf(settingName, root, leaf);

    Json::Path settingPath(root.c_str());
    Json::Value & jsonValue = settingPath.make(this->UserSettingsJSONRoot);
    jsonValue[leaf] = Json::Value::null;

    if (values.size() > 1)
      {
      jsonValue[leaf].resize(values.size());

      for (size_t i = 0; i < values.size(); ++i)
        {
        jsonValue[leaf][(unsigned int)i] = values[i];
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
    Json::Value & jsonValue = valuePath.make(this->UserSettingsJSONRoot);
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
    Json::Value & jsonValue = valuePath.make(this->UserSettingsJSONRoot);
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
    Json::Value & jsonValue = valuePath.make(this->UserSettingsJSONRoot);
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
  // Set a property setting to a Json::Value
  void SetPropertySetting(const char* settingName, vtkSMProperty* property)
  {
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
  // Save proxy settings
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
  // Save proxy settings at a given JSON path
  void SetProxySettings(const char* settingPrefix, vtkSMProxy* proxy)
  {
    if (!proxy)
      {
      return;
      }

    // Get reference to JSON value
    vtksys_ios::ostringstream settingStringStream;
    settingStringStream << settingPrefix << "." << proxy->GetXMLName();
    std::string settingString(settingStringStream.str());
    const char* settingCString = settingString.c_str();

    Json::Path valuePath(settingCString);
    Json::Value & proxyValue = valuePath.make(this->UserSettingsJSONRoot);

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
          property->GetIsInternal())
        {
        continue;
        }
      else if (property->IsValueDefault())
        {
        // Remove existing JSON entry only if there is no site setting
        if (!this->HasSiteSetting(propertySettingCString))
          {
          proxyValue.removeMember(property->GetXMLName());
          continue;
          }
        }

      this->SetPropertySetting(propertySettingCString, property);
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

};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSettings);

//----------------------------------------------------------------------------
vtkSMSettings::vtkSMSettings()
{
  this->Internal = new vtkSMSettingsInternal();
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
bool vtkSMSettings::LoadSettings()
{
  vtkSMSettings * settings = vtkSMSettings::GetInstance();
  vtkProcessModule *pm = vtkProcessModule::GetProcessModule();
  if (pm->GetProcessType() == vtkProcessModule::PROCESS_CLIENT ||
      (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
       pm->GetPartitionId() == 0))
    {
    settings->LoadSiteSettings();
    settings->LoadUserSettings();
    }
  if (pm->GetProcessType() == vtkProcessModule::PROCESS_BATCH &&
      pm->GetSymmetricMPIMode())
    {
    // Broadcast settings to satellite nodes
    vtkMultiProcessController * controller = pm->GetGlobalController();
    unsigned int stringSize;
    if (controller->GetLocalProcessId() == 0)
      {
      std::string siteSettingsString = settings->GetSiteSettingsAsString().c_str();
      stringSize = static_cast<unsigned int>(siteSettingsString.size())+1;
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        controller->Broadcast(const_cast<char*>(siteSettingsString.c_str()), stringSize, 0);
        }

      std::string userSettingsString = settings->GetUserSettingsAsString();
      stringSize = static_cast<unsigned int>(userSettingsString.size())+1;
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        controller->Broadcast(const_cast<char*>(userSettingsString.c_str()), stringSize, 0);
        }
      }
    else // Satellites
      {
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        char* siteSettingsString = new char[stringSize];
        controller->Broadcast(siteSettingsString, stringSize, 0);
        settings->SetSiteSettingsFromString(siteSettingsString);
        delete[] siteSettingsString;
        }
      else
        {
        settings->SetSiteSettingsFromString(NULL);
        }
      controller->Broadcast(&stringSize, 1, 0);
      if (stringSize > 0)
        {
        char* userSettingsString = new char[stringSize];
        controller->Broadcast(userSettingsString, stringSize, 0);
        settings->SetUserSettingsFromString(userSettingsString);
        delete[] userSettingsString;
        }
      else
        {
        settings->SetUserSettingsFromString(NULL);
        }
      }
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::LoadUserSettings()
{
  std::string fileName = GetUserSettingsFilePath();
  return this->LoadUserSettings(fileName.c_str());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::LoadUserSettings(const char* fileName)
{
  std::string userSettingsFileName(fileName);
  std::ifstream userSettingsFile(userSettingsFileName.c_str(), ios::in | ios::binary | ios::ate);
  if (userSettingsFile.is_open())
    {
    std::streampos size = userSettingsFile.tellg();
    userSettingsFile.seekg(0, ios::beg);
    int stringSize = size;
    char * userSettingsString = new char[stringSize+1];
    userSettingsFile.read(userSettingsString, stringSize);
    userSettingsString[stringSize] = '\0';
    userSettingsFile.close();

    this->SetUserSettingsFromString( userSettingsString );
    delete[] userSettingsString;
    }
  else
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetUserSettingsFromString(const char* settings)
{
  // Blank out the settings
  this->Internal->UserSettingsJSONRoot.clear();

  if (settings == NULL || strcmp(settings, "") == 0)
    {
    return;
    }

  // Parse the user settings
  Json::Reader reader;
  bool success = reader.parse(std::string(settings), this->Internal->UserSettingsJSONRoot, true);
  if (!success)
    {
    vtkErrorMacro(<< "Could not parse user settings JSON");
    this->Internal->UserSettingsJSONRoot = Json::Value::null;
    }
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetUserSettingsAsString()
{
  return this->Internal->UserSettingsJSONRoot.toStyledString();
}

//----------------------------------------------------------------------------
bool vtkSMSettings::LoadSiteSettings()
{
  std::string fileName = this->GetSiteSettingsFilePath();
  return this->LoadSiteSettings(fileName.c_str());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::LoadSiteSettings(const char* fileName)
{
  std::string siteSettingsFileName(fileName);
  std::ifstream siteSettingsFile(siteSettingsFileName.c_str(), ios::in | ios::binary | ios::ate);
  if (siteSettingsFile.is_open())
    {
    std::streampos size = siteSettingsFile.tellg();
    siteSettingsFile.seekg(0, ios::beg);
    int stringSize = size;
    char * siteSettingsString = new char[stringSize+1];
    siteSettingsFile.read(siteSettingsString, stringSize);
    siteSettingsString[stringSize] = '\0';
    siteSettingsFile.close();

    this->SetSiteSettingsFromString( siteSettingsString );
    delete[] siteSettingsString;
    }
  else
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SaveSettings()
{
  std::ofstream userSettingsFile(this->GetUserSettingsFilePath().c_str(), ios::out | ios::binary );
  if (userSettingsFile.is_open())
    {
    std::string userOutput = this->GetUserSettingsAsString();
    userSettingsFile << userOutput;
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSiteSettingsFromString(const char* settings)
{
  // Blank out the settings
  this->Internal->SiteSettingsJSONRoot.clear();

  if (settings == NULL || strcmp(settings, "") == 0)
    {
    return;
    }

  // Parse the user settings
  Json::Reader reader;
  bool success = reader.parse(std::string(settings), this->Internal->SiteSettingsJSONRoot);
  if (!success)
    {
    vtkErrorMacro(<< "Could not parse site settings JSON");
    this->Internal->SiteSettingsJSONRoot = Json::Value::null;
    }
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSiteSettingsAsString()
{
  return this->Internal->SiteSettingsJSONRoot.toStyledString();
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
  return this->GetSettingAsInt(settingName, defaultValue);
}

//----------------------------------------------------------------------------
double vtkSMSettings::GetSettingAsDouble(const char* settingName,
                                         double defaultValue)
{
  return this->GetSettingAsDouble(settingName, defaultValue);
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingAsString(const char* settingName,
                                              const std::string & defaultValue)
{
  return this->GetSettingAsString(settingName, defaultValue);
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
  if (!this->Internal->HasUserSetting(settingName))
    {
    return;
    }

  Json::Path settingPath(settingName);
  Json::Value & settingValue = settingPath.make(this->Internal->UserSettingsJSONRoot);
  settingValue.setComment(description, Json::commentBefore);
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSettingsFilePathRoot()
{
  // Not sure where this should go. For now, read a file from user directory
#if defined(WIN32)
  const char* appData = getenv("APPDATA");
  if (!appData)
    {
    return std::string();
    }
  std::string separator("\\");
  std::string fileName(appData);
  if (fileName[fileName.size()-1] != separator[0])
    {
    fileName.append(separator);
    }
  fileName += "ParaView" + separator;
#else
  const char* home = getenv("HOME");
  if (!home)
    {
    return std::string();
    }
  std::string separator("/");
  std::string fileName(home);
  if (fileName[fileName.size()-1] != separator[0])
    {
    fileName.append(separator);
    }
  fileName += ".config" + separator + "ParaView" + separator;
#endif

  return fileName;
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetUserSettingsFilePath()
{
  std::string fileName(this->GetSettingsFilePathRoot());
  fileName.append("Settings.json");

  return fileName;
}

//----------------------------------------------------------------------------
std::string vtkSMSettings::GetSiteSettingsFilePath()
{
  // FIXME - hmm, site settings will probably be somewhere else
  std::string fileName(this->GetSettingsFilePathRoot());
  fileName.append("SiteSettings.json");

  return fileName;
}

//----------------------------------------------------------------------------
void vtkSMSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "UserSettings:\n";
  os << this->Internal->UserSettingsJSONRoot.toStyledString();

  os << indent << "SiteSettings:\n";
  os << this->Internal->SiteSettingsJSONRoot.toStyledString();
}
