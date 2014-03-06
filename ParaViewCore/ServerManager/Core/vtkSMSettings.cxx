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
  bool GetScalarSetting(const char* settingName, T & value)
  {
    std::vector<T> vector;
    if (!this->GetVectorSetting(settingName, vector))
      {
      return false;
      }

    // Set the value to be the first element in the retrieved vector.
    if (vector.size() >= 1 )
      {
      value = vector[0];
      }

    return true;
  }

  //----------------------------------------------------------------------------
  template< typename T >
  bool GetVectorSetting(const char* settingName, std::vector<T> & values)
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
        this->GetScalarSetting(setting[i], value);
        values.push_back(value);
        }
      }
    else
      {
      T value;
      bool success = this->GetScalarSetting(setting, value);
      if (success)
        {
        values.push_back(value);
        }
      return success;
      }

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetScalarSetting(const Json::Value & jsonValue, int & value)
  {
    if (!jsonValue.isNumeric())
      {
      return false;
      }
    value = jsonValue.asInt();

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetScalarSetting(const Json::Value & jsonValue, long long int & value)
  {
    if (!jsonValue.isNumeric())
      {
      return false;
      }
    value = jsonValue.asLargestInt();

    return true;
  }

  //----------------------------------------------------------------------------
  bool GetScalarSetting(const Json::Value & jsonValue, double & value)
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
  bool GetScalarSetting(const Json::Value & jsonValue, vtkStdString & value)
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
  // Save a property setting to a Json::Value
  void SavePropertySetting(vtkSMProperty* property, Json::Value & jsonValue)
  {
    vtkSMIntVectorProperty*    intVectorProperty    = NULL;
    vtkSMDoubleVectorProperty* doubleVectorProperty = NULL;
    vtkSMStringVectorProperty* stringVectorProperty = NULL;
    vtkSMInputProperty*        inputProperty        = NULL;

    if (intVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property))
      {
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
    else
      {
      std::cerr << "Unhandled property '" << property->GetXMLName() << "' of type '"
                << property->GetClassName() << "'\n";
      }
  }

};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMSettings);

//----------------------------------------------------------------------------
vtkSMSettings::vtkSMSettings()
{
  this->UserSettingsString = NULL;
  this->SiteSettingsString = NULL;
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

    this->SetUserSettingsString( userSettingsString );
    delete[] userSettingsString;
    }
  else
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetUserSettingsString(const char* settings)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting UserSettingsString to "
                << (settings ? settings :"(null)") );
  if ( this->UserSettingsString == NULL && settings == NULL) { return;}
  if ( this->UserSettingsString && settings && (!strcmp(this->UserSettingsString,settings))) { return;}
  delete [] this->UserSettingsString;

  if (settings)
    {
    size_t n = strlen(settings) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = settings;
    this->UserSettingsString = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
   else
    {
    this->UserSettingsString = NULL;
    }
  this->Modified();

  if (!this->UserSettingsString)
    {
    return;
    }

  // Parse the user settings
  Json::Reader reader;
  bool success = reader.parse(std::string( settings ), this->Internal->UserSettingsJSONRoot, false);
  if (!success)
    {
    vtkErrorMacro(<< "Could not parse user settings JSON");
    this->Internal->UserSettingsJSONRoot = Json::Value::null;
    }
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

    this->SetSiteSettingsString( siteSettingsString );
    delete[] siteSettingsString;
    }
  else
    {
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSMSettings::SetSiteSettingsString(const char* settings)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting SiteSettingsString to "
                << (settings ? settings :"(null)") );
  if ( this->SiteSettingsString == NULL && settings == NULL) { return;}
  if ( this->SiteSettingsString && settings && (!strcmp(this->SiteSettingsString,settings))) { return;}
  delete [] this->SiteSettingsString;

  if (settings)
    {
    size_t n = strlen(settings) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = settings;
    this->SiteSettingsString = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
   else
    {
    this->SiteSettingsString = NULL;
    }
  this->Modified();

  if ( !this->SiteSettingsString )
    {
    return;
    }

  // Parse the user settings
  Json::Reader reader;
  bool success = reader.parse(std::string( settings ), this->Internal->SiteSettingsJSONRoot, false);
  if (!success)
    {
    vtkErrorMacro(<< "Could not parse site settings JSON");
    this->Internal->SiteSettingsJSONRoot = Json::Value::null;
    }
}

//----------------------------------------------------------------------------
bool vtkSMSettings::HasSetting(const char* settingName)
{
  return this->Internal->HasSetting(settingName);
}

//----------------------------------------------------------------------------
void vtkSMSettings::SaveProxySettings(vtkSMProxy* proxy)
{
  if (!proxy)
    {
    return;
    }

  const char* proxyGroup = proxy->GetXMLGroup();
  const char* proxyName = proxy->GetXMLName();

  if (!proxyGroup || !proxyName)
    {
    return;
    }

  // Create group node if it doesn't exist
  vtksys_ios::ostringstream settingStringStream;
  settingStringStream << "." << proxyGroup;
  std::string settingString(settingStringStream.str());
  const char* settingCString = settingString.c_str();

  if (!this->Internal->HasSetting(settingCString))
    {
    Json::Value newValue;
    this->Internal->UserSettingsJSONRoot[proxyGroup] = newValue;
    }

  // Create proxy node if it doesn't exist
  settingStringStream << "." << proxyName;
  std::cout << settingStringStream.str() << std::endl;
  settingString = settingStringStream.str();
  settingCString = settingString.c_str();

  if (!this->Internal->HasSetting(settingCString))
    {
    Json::Value newValue;
    this->Internal->UserSettingsJSONRoot[proxyGroup][proxyName] = newValue;
    }

  Json::Value proxyValue = this->Internal->GetSetting(settingCString);
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
      if (!this->Internal->HasSiteSetting(propertySettingCString))
        {
        std::cout << "Removing member " << property->GetXMLName() << std::endl;
        this->Internal->
          UserSettingsJSONRoot[proxyGroup][proxyName].removeMember(property->GetXMLName());
        continue;
        }
      }

    Json::Value propertyValue;
    this->Internal->SavePropertySetting(property, propertyValue);
    if (!propertyValue.isNull())
      {
      this->Internal->
        UserSettingsJSONRoot[proxyGroup][proxyName][property->GetXMLName()] = propertyValue;
      }
    }

  std::cout << this->Internal->UserSettingsJSONRoot.toStyledString();
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetScalarSetting(const char* settingName, int & value)
{
  return this->Internal->GetScalarSetting(settingName, value);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetScalarSetting(const char* settingName, long long int & value)
{
  return this->Internal->GetScalarSetting(settingName, value);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetScalarSetting(const char* settingName, double & value)
{
  return this->Internal->GetScalarSetting(settingName, value);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetScalarSetting(const char* settingName, vtkStdString & value)
{
  return this->Internal->GetScalarSetting(settingName, value);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetVectorSetting(const char* settingName, std::vector<int> & values)
{
  return this->Internal->GetVectorSetting(settingName, values);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetVectorSetting(const char* settingName, std::vector<long long int> & values)
{
  return this->Internal->GetVectorSetting(settingName, values);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetVectorSetting(const char* settingName, std::vector<double> & values)
{
  return this->Internal->GetVectorSetting(settingName, values);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::GetVectorSetting(const char* settingName, std::vector<vtkStdString> & values)
{
  return this->Internal->GetVectorSetting(settingName, values);
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SetProxySettings(vtkSMProxy* proxy, const char* jsonPath)
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
      settingStringStream << jsonPath
                          << "." << proxy->GetXMLName()
                          << "." << property->GetXMLName();

      const std::string settingString = settingStringStream.str();
      const char* settingCString = settingString.c_str();
      if (this->HasSetting(settingCString))
        {
        vtkSMIntVectorProperty*    intVectorProperty = NULL;
        vtkSMDoubleVectorProperty* doubleVectorProperty = NULL;
        vtkSMStringVectorProperty* stringVectorProperty = NULL;
        vtkSMInputProperty*        inputProperty = NULL;

        bool success = false;
        if (intVectorProperty = vtkSMIntVectorProperty::SafeDownCast(property))
          {
          success = this->SetPropertySetting(intVectorProperty, settingCString);
          }
        else if (doubleVectorProperty = vtkSMDoubleVectorProperty::SafeDownCast(property))
          {
          success = this->SetPropertySetting(doubleVectorProperty, settingCString);
          }
        else if (stringVectorProperty = vtkSMStringVectorProperty::SafeDownCast(property))
          {
          success = this->SetPropertySetting(stringVectorProperty, settingCString);
          }
        else if (inputProperty = vtkSMInputProperty::SafeDownCast(property))
          {
          success = this->SetPropertySetting(inputProperty, settingCString);
          }
        else
          {
          vtkErrorMacro(<< "Unhandled vtkSMProperty type '" << property->GetClassName() << "'");
          overallSuccess = false;
          }

        if (!success)
          {
          vtkErrorMacro(<< "Setting property " << settingStringStream.str() << " failed");
          overallSuccess = false;
          }
        }
      }
    }

  iter->Delete();

  return overallSuccess;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SetPropertySetting(vtkSMIntVectorProperty* property,
                                       const char* jsonPath)
{
  vtkSMDomain* domain = property->FindDomain("vtkSMEnumerationDomain");
  vtkSMEnumerationDomain* enumDomain = vtkSMEnumerationDomain::SafeDownCast(domain);
  if (enumDomain)
    {
    // The enumeration property could be either text or value
    int enumValue;
    bool hasInt = this->GetScalarSetting(jsonPath, enumValue);
    if (hasInt)
      {
      property->SetElement(0, enumValue);
      }
    else
      {
      vtkStdString stringValue;
      bool hasString = this->GetScalarSetting(jsonPath, stringValue);
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
    if (!this->GetVectorSetting(jsonPath, vector) ||
        vector.size() != property->GetNumberOfElements())
      {
      return false;
      }
    property->SetElements(&vector[0]);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SetPropertySetting(vtkSMDoubleVectorProperty* property,
                                       const char* jsonPath)
{
  std::vector<double> vector;
  if (!this->GetVectorSetting(jsonPath, vector))
    {
    return false;
    }
  property->SetElements(&vector[0]);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SetPropertySetting(vtkSMStringVectorProperty* property,
                                       const char* jsonPath)
{
  std::vector<vtkStdString> vector;
  if (!this->GetVectorSetting(jsonPath, vector))
    {
    return false;
    }

  vtkSmartPointer<vtkStringList> stringList = vtkSmartPointer<vtkStringList>::New();
  for (size_t i = 0; i < vector.size(); ++i)
    {
    stringList->AddString(vector[i]);
    }

  property->SetElements(stringList);

  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSettings::SetPropertySetting(vtkSMInputProperty* property,
                                       const char* jsonPath)
{
  std::vector<vtkStdString> vector;
  if (!this->GetVectorSetting(jsonPath, vector))
    {
    return false;
    }

  vtkSMDomain * domain = property->GetDomain( "proxy_list" );
  vtkSMProxyListDomain * proxyListDomain = NULL;
  if (proxyListDomain = vtkSMProxyListDomain::SafeDownCast(domain))
    {
    // Now check whether this proxy is the one we want
    vtkStdString sourceSettingString(jsonPath);
    sourceSettingString.append(".Selected");

    vtkStdString sourceName;
    if (this->HasSetting(sourceSettingString.c_str()))
      {
      std::vector<vtkStdString> selectedString;
      this->GetVectorSetting(sourceSettingString.c_str(), selectedString);
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
        bool success = this->SetProxySettings(listProxy, jsonPath);
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
void vtkSMSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "UserSettingsString: ";
  if ( this->UserSettingsString )
    {
    os << "\n" << this->UserSettingsString << "\n";
    }
  else
    {
    os << "(null)";
    }

  os << indent << "SiteSettingsString: ";
  if ( this->SiteSettingsString )
    {
    os << "\n" << this->SiteSettingsString << "\n";
    }
  else
    {
    os << "(null)";
    }
}
