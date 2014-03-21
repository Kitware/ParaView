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
  // Check whether a setting is defined for the requested names.
  bool HasSetting(const char* settingName);

  // Description:
  // Set setting of a given name
  void SetScalarSetting(const char* settingName, int value);
  void SetScalarSetting(const char* settingName, double value);
  void SetScalarSetting(const char* settingName, long long int value);
  void SetScalarSetting(const char* settingName, const vtkStdString & value);

  // Description:
  // Set vector setting at a location given by the jsonPath
  void SetVectorSetting(const char* settingName, const std::vector<int> & values);
  void SetVectorSetting(const char* settingName, const std::vector<long long int> & values);
  void SetVectorSetting(const char* settingName, const std::vector<double> & values);
  void SetVectorSetting(const char* settingName, const std::vector<vtkStdString> & values);

  // Description:
  // Save non-default settings in the current user settings.
  void SetProxySettings(vtkSMProxy* proxy);

  // Description:
  // Get setting as a scalar value
  int           GetScalarSettingAsInt(const char* settingName);
  double        GetScalarSettingAsDouble(const char* settingName);
  long long int GetScalarSettingAsLongLongInt(const char* settingName);
  vtkStdString  GetScalarSettingAsString(const char* settingName);

  // Description:
  // Get setting as a vector of a certain type. Returns true on
  // success, false on failure. These methods work on values given as
  // JSON arrays of numbers as well as singular JSON numbers.
  std::vector<int> GetVectorSettingAsInts(const char* settingName);
  std::vector<long long int> GetVectorSettingAsLongLongInts(const char* settingName);
  std::vector<double> GetVectorSettingAsDoubles(const char* settingName);
  std::vector<vtkStdString> GetVectorSettingAsStrings(const char* settingName);

  // Description:
  // Set the property values in a vtkSMProxy from a Json path.
  // The jsonPrefix specifies the root level of the Json tree where this method
  // should look for settings. The group and name of the proxy are
  // appended to the jsonPrefix automatically.
  bool GetProxySettings(vtkSMProxy* proxy, const char* jsonPrefix);

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
