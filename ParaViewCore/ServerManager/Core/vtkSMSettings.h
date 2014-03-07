/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMSettings
// .SECTION Description
// vtkSMSettings provides the underlying mechanism for setting default property
// values in ParaView
#ifndef __vtkSMSettings_h
#define __vtkSMSettings_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkObject.h"
#include "vtkStdString.h"

class vtkSMDoubleVectorProperty;
class vtkSMInputProperty;
class vtkSMIntVectorProperty;
class vtkSMStringVectorProperty;
class vtkSMProxy;

#include <vector>

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkSMSettings : public vtkObject
{
public:
  static vtkSMSettings* New();
  vtkTypeMacro(vtkSMSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get singleton instance.
  static vtkSMSettings* GetInstance();

  // Description:
  // Load user settings from default location. On linux/unix, this is
  // $HOME/.pvsettings.user.js. On Windows, this is under
  // %USERPROFILE%/.pvsettings.js. Returns true on success, false
  // otherwise.
  bool LoadUserSettings();

  // Description:
  // Load user settings from a specified file. Returns true on
  // success, false otherwise.
  bool LoadUserSettings(const char* fileName);

  // Description:
  // Set user-specific settings. These are stored in a home directory.
  virtual void SetUserSettingsString(const char* settings);
  vtkGetStringMacro(UserSettingsString);;

  // Description:
  // Load site settings from default location TBD. Returns true on success,
  // false otherwise.
  bool LoadSiteSettings();

  // Description:
  // Load site settings from a file. Returns true on success, false
  // otherwise.
  bool LoadSiteSettings(const char* fileName);

  // Description:
  // Set site-specific settings. These are stored in a location TBD.
  virtual void SetSiteSettingsString(const char* settings);
  vtkGetStringMacro(SiteSettingsString);

  // Description:
  // Save non-default settings in the current user settings.
  void SetProxySettings(vtkSMProxy* proxy);

  // Description:
  // Set setting pointed to by jsonPath
  bool SetScalarSetting(const char* jsonPath, int value);
  bool SetScalarSetting(const char* jsonPath, const char* value);

  // Description:
  // Check whether a setting is defined for the requested names.
  bool HasSetting(const char* settingName);

  // Description:
  // Get setting as a scalar value
  bool GetScalarSetting(const char* settingName, int & value);
  bool GetScalarSetting(const char* settingName, double & value);
  bool GetScalarSetting(const char* settingName, long long int & value);
  bool GetScalarSetting(const char* settingName, vtkStdString & value);

  // Description:
  // Get setting as a vector of various numerical types. Returns true on
  // success, false on failure. These methods work on values given as
  // JSON arrays of numbers as well as singular JSON numbers.
  bool GetVectorSetting(const char* settingName, std::vector<int> & values);
  bool GetVectorSetting(const char* settingName, std::vector<long long int> & values);
  bool GetVectorSetting(const char* settingName, std::vector<double> & values);

  // Description:
  // Get setting as a vector of string types. Returns true on
  // success, false on failure. This method works on values given as
  // JSON arrays of strings as well as singular JSON strings.
  bool GetVectorSetting(const char* settingName, std::vector<vtkStdString> & values);

  // Description:
  // Set the property values in a vtkSMProxy from a Json path.
  // The jsonPrefix specifies the root level of the Json tree where this method
  // should look for settings. The group and name of the proxy are
  // appended to the jsonPrefix automatically.
  bool GetProxySettings(vtkSMProxy* proxy, const char* jsonPrefix);

  // Description:
  // Get int vector from JSON path and set it in the property.
  bool GetPropertySetting(vtkSMIntVectorProperty* property, const char* jsonPath);

  // Description:
  // Get double vector from JSON path and set it in the property.
  bool GetPropertySetting(vtkSMDoubleVectorProperty* property, const char* jsonPath);

  // Description:
  // Get string vector from JSON path and set it in the property.
  bool GetPropertySetting(vtkSMStringVectorProperty* property, const char* jsonPath);

  // Description:
  // Get input property from JSON path and set it in the property.
  bool GetPropertySetting(vtkSMInputProperty* property, const char* jsonPath);

protected:
  vtkSMSettings();
  virtual ~vtkSMSettings();

private:
  vtkSMSettings(const vtkSMSettings&); // Purposely not implemented
  void operator=(const vtkSMSettings&); // Purposely not implemented

  // User-specified settings
  char* UserSettingsString;

  // Site-specific settings
  char* SiteSettingsString;

  class vtkSMSettingsInternal;
  vtkSMSettingsInternal * Internal;
};

#endif
