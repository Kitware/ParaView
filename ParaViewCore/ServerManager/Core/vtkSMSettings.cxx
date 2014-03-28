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

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMProxyListDomain.h"
#include "vtkStringList.h"
#include "vtkPVSession.h"

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
  void SetSetting(const char* settingName, const std::vector< T > & values)
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
  void SetSettingDescription(const char* settingName, const char* description)
  {
    Json::Value value = this->GetSetting(settingName);

    std::string root, leaf;
    this->SeparateBranchFromLeaf(settingName, root, leaf);

    //Json::Path settingPath(root.c_str());
    Json::Path settingPath(settingName);
    const Json::Value & testValue = settingPath.resolve(this->UserSettingsJSONRoot);
    if (!testValue)
      {
      return;
      }

    // Setting exists, now set the description.
    Json::Value & settingValue = settingPath.make(this->UserSettingsJSONRoot);
    settingValue.setComment(description, Json::commentBefore);
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
  bool GetProxySettings(vtkSMProxy* proxy, const char* jsonPrefix)
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
        settingStringStream << jsonPrefix
                            << "." << proxy->GetXMLName()
                            << "." << property->GetXMLName();

        const std::string settingString = settingStringStream.str();
        const char* settingCString = settingString.c_str();
        if (this->HasSetting(settingCString))
          {
          vtkSMIntVectorProperty*    intVectorProperty    = NULL;
          vtkSMDoubleVectorProperty* doubleVectorProperty = NULL;
          vtkSMStringVectorProperty* stringVectorProperty = NULL;
          vtkSMInputProperty*        inputProperty        = NULL;

          bool success = false;
          if (intVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(intVectorProperty, settingCString);
            }
          else if (doubleVectorProperty = vtkSMDoubleVectorProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(doubleVectorProperty, settingCString);
            }
          else if (stringVectorProperty = vtkSMStringVectorProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(stringVectorProperty, settingCString);
            }
          else if (inputProperty = vtkSMInputProperty::SafeDownCast(property))
            {
            success = this->GetPropertySetting(inputProperty, settingCString);
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
  bool GetPropertySetting(vtkSMIntVectorProperty* property,
                          const char* jsonPath)
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
      const Json::Value & jsonValue = this->GetSetting(jsonPath);
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
      if (!this->GetSetting(jsonPath, vector) ||
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
  bool GetPropertySetting(vtkSMDoubleVectorProperty* property, const char* jsonPath)
  {
    std::vector<double> vector;
    if (!property || !this->GetSetting(jsonPath, vector))
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
  bool GetPropertySetting(vtkSMStringVectorProperty* property, const char* jsonPath)
  {
    std::vector<std::string> vector;
    if (!property || !this->GetSetting(jsonPath, vector))
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
  bool GetPropertySetting(vtkSMInputProperty* property, const char* jsonPath)
  {
    vtkSMDomain * domain = property->GetDomain( "proxy_list" );
    vtkSMProxyListDomain * proxyListDomain = NULL;
    if (proxyListDomain = vtkSMProxyListDomain::SafeDownCast(domain))
      {
      // Now check whether this proxy is the one we want
      std::string sourceSettingString(jsonPath);
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
          bool success = this->GetProxySettings(listProxy, jsonPath);
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
  // Description:
  // Set a property setting to a Json::Value
  void SetPropertySetting(vtkSMProperty* property, const char* jsonPath)
  {
    Json::Path valuePath(jsonPath);

    vtkSMIntVectorProperty*    intVectorProperty    = NULL;
    vtkSMDoubleVectorProperty* doubleVectorProperty = NULL;
    vtkSMStringVectorProperty* stringVectorProperty = NULL;
    vtkSMInputProperty*        inputProperty        = NULL;

    if (intVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property))
      {
      Json::Value & jsonValue = valuePath.make(this->UserSettingsJSONRoot);
      if (intVectorProperty->GetNumberOfElements() == 1)
        {
        jsonValue = intVectorProperty->GetElement(0);
        }
      else
        {
        jsonValue.resize(intVectorProperty->GetNumberOfElements());
        for (unsigned int i = 0; i < intVectorProperty->GetNumberOfElements(); ++i)
          {
          jsonValue[i] = intVectorProperty->GetElement(i);
          }
        }
      }
    else if (doubleVectorProperty = vtkSMDoubleVectorProperty::SafeDownCast(property))
      {
      Json::Value & jsonValue = valuePath.make(this->UserSettingsJSONRoot);
      if (doubleVectorProperty->GetNumberOfElements() == 1)
        {
        jsonValue = doubleVectorProperty->GetElement(0);
        }
      else
        {
        jsonValue.resize(doubleVectorProperty->GetNumberOfElements());
        for (unsigned int i = 0; i < doubleVectorProperty->GetNumberOfElements(); ++i)
          {
          jsonValue[i] = doubleVectorProperty->GetElement(i);
          }
        }
      }
    else if (stringVectorProperty = vtkSMStringVectorProperty::SafeDownCast(property))
      {
      Json::Value & jsonValue = valuePath.make(this->UserSettingsJSONRoot);
      if (stringVectorProperty->GetNumberOfElements() == 1)
        {
        jsonValue = stringVectorProperty->GetElement(0);
        }
      else
        {
        jsonValue.resize(stringVectorProperty->GetNumberOfElements());
        for (unsigned int i = 0; i < stringVectorProperty->GetNumberOfElements(); ++i)
          {
          jsonValue[i] = stringVectorProperty->GetElement(i);
          }
        }
      }
    else if (inputProperty = vtkSMInputProperty::SafeDownCast(property))
      {
      std::cerr << "Unhandled property '" << property->GetXMLName() << "' of type '"
                << property->GetClassName() << "'\n";
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

    this->SetProxySettings(proxy, jsonPrefix.c_str());
  }

  //----------------------------------------------------------------------------
  // Description:
  // Save proxy settings at a given JSON path
  void SetProxySettings(vtkSMProxy* proxy, const char* jsonPath)
  {
    if (!proxy)
      {
      return;
      }

    // Get reference to JSON value
    vtksys_ios::ostringstream settingStringStream;
    settingStringStream << jsonPath << "." << proxy->GetXMLName();
    std::string settingString(settingStringStream.str());
    const char* settingCString = settingString.c_str();

    Json::Path valuePath(settingCString);
    Json::Value & proxyValue = valuePath.make(this->UserSettingsJSONRoot);

    vtkSMPropertyIterator * iter = proxy->NewPropertyIterator();
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
      
      this->SetPropertySetting(property, propertySettingCString);
      }
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
bool vtkSMSettings::LoadUserSettings()
{
#if defined(WIN32)
  std::string fileName(getenv("USERPROFILE"));
  std::string separator("\\");
#else
  std::string fileName(getenv("HOME"));
  std::string separator("/");
#endif

  if (fileName.size() > 0)
    {
    if (fileName[fileName.size()-1] != separator[0])
      {
      fileName.append(separator);
      }
    }
  else
    {
    // Might want to return false here instead of trying the root
    // directory.
    fileName.append(separator);
    }

  fileName.append(".pvsettings.js");

  return this->LoadUserSettings(fileName.c_str());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::LoadUserSettings(const char* fileName)
{
  std::string userSettingsFileName(fileName);
  std::ifstream userSettingsFile(userSettingsFileName.c_str(), ios::in | ios::binary | ios::ate );
  if ( userSettingsFile.is_open() )
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
  // Not sure where this should go. For now, read a file from user directory
#if defined(WIN32)
  std::string fileName(getenv("USERPROFILE"));
  std::string separator("\\");
#else
  std::string fileName(getenv("HOME"));
  std::string separator("/");
#endif

  if (fileName.size() > 0)
    {
    if (fileName[fileName.size()-1] != separator[0])
      {
      fileName.append(separator);
      }
    }
  else
    {
    // Might want to return false here instead of trying the root
    // directory.
    fileName.append(separator);
    }

  fileName.append(".pvsitesettings.js");

  return this->LoadSiteSettings(fileName.c_str());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::LoadSiteSettings(const char* fileName)
{
  std::string siteSettingsFileName(fileName);
  std::ifstream siteSettingsFile(siteSettingsFileName.c_str(), ios::in | ios::binary | ios::ate );
  if ( siteSettingsFile.is_open() )
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
void vtkSMSettings::SetSettingDescription(const char* settingName, const char* description)
{
  this->Internal->SetSettingDescription(settingName, description);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetProxySettings(vtkSMProxy* proxy)
{
  this->Internal->SetProxySettings(proxy);
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
  if (value.hasComment(Json::commentBefore))
    {
    return value.getComment(Json::commentBefore);
    }
  else if (value.hasComment(Json::commentAfterOnSameLine))
    {
    return value.getComment(Json::commentAfterOnSameLine);
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

  return this->GetProxySettings(proxy, jsonPrefix.c_str());
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetProxySettings(vtkSMProxy* proxy, const char* jsonPrefix)
{
  if (!proxy)
    {
    return false;
    }

  return this->Internal->GetProxySettings(proxy, jsonPrefix);
}

//----------------------------------------------------------------------------
void vtkSMSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "UserSettings:\n";
  os << this->Internal->UserSettingsJSONRoot.toStyledString();

  os << indent << "SiteSettings:\n";
  os << this->Internal->SiteSettingsJSONRoot.toStyledString();
}
