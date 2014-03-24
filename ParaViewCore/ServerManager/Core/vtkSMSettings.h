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
  // Set setting of a given name.
  // Shortcut for SetSetting(settingName, 0, value). Useful for setting scalar values.
  void SetSetting(const char* settingName, int value);
  void SetSetting(const char* settingName, double value);
  void SetSetting(const char* settingName, const std::string & value);

  // Description:
  // Set element of a vector setting at a location given by the jsonPath.
  void SetSetting(const char* settingName, unsigned int index, int value);
  void SetSetting(const char* settingName, unsigned int index, double value);
  void SetSetting(const char* settingName, unsigned int index, const std::string & value);

  // Description:
  // Save non-default settings in the current user settings.
  void SetProxySettings(vtkSMProxy* proxy);

  // Description:
  // Get the number of elements in a setting.
  unsigned int GetSettingNumberOfElements(const char* settingName);

  // Description:
  // Get a single element of a vector setting.
  int         GetSettingAsInt(const char* settingName,
                              unsigned int index,
                              int defaultValue);
  double      GetSettingAsDouble(const char* settingName,
                                 unsigned int index,
                                 double defaultValue);
  std::string GetSettingAsString(const char* settingName,
                                 unsigned int index,
                                 const std::string & defaultValue);

  // Description:
  // Set the property values in a vtkSMProxy from the settings file.
  // Searches settings from the settings root.
  bool GetProxySettings(vtkSMProxy* proxy);

  // Description:
  // Set the property values in a vtkSMProxy from a JSON prefix.
  // The jsonPrefix specifies the root level of the JSON tree where this method
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
