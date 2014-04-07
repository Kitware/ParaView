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

#include "vtkObject.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkStdString.h" // needed for vtkStdString.
#include <vector> // needed for vector.

class vtkSMDoubleVectorProperty;
class vtkSMInputProperty;
class vtkSMIntVectorProperty;
class vtkSMStringVectorProperty;
class vtkSMProxy;


class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSettings : public vtkObject
{
public:
  static vtkSMSettings* New();
  vtkTypeMacro(vtkSMSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get singleton instance.
  static vtkSMSettings* GetInstance();

  // Description:
  // Add settings from a string. The string should contain valid JSON-formatted
  // text. The "priority" indicates how to treat a setting that has definitions
  // in more than one string passed to this class in multiple calls to this
  // function. If two settings strings contain values for the same
  // setting, then the setting from the string with higher priority will be
  // used.
  bool AddSettingsFromString(const std::string & settings, double priority);

  // Description:
  // The same as AddSettingsFromString, but this method reads the settings
  // string from the named file. The fileName should be a full path.
  bool AddSettingsFromFile(const std::string & fileName, double priority);

  // Description:
  // Clear out all settings.
  void ClearAllSettings();

  // Description:
  // Distribute settings to all processes if in batch symmetric mode.
  bool DistributeSettings();

  // Description:
  // Save highest priority settings to file
  bool SaveSettings(const std::string & filePath);

  // Description:
  // Check whether a setting is defined for the requested names.
  bool HasSetting(const char* settingName);

  // Description:
  // Get the number of elements in a setting.
  unsigned int GetSettingNumberOfElements(const char* settingName);

  // Description:
  // Get a vector setting as a scalar value.
  // Shortcut for GetSettingAs...
  int         GetSettingAsInt(const char* settingName,
                              int defaultValue);
  double      GetSettingAsDouble(const char* settingName,
                                 double defaultValue);
  std::string GetSettingAsString(const char* settingName,
                                 const std::string & defaultValue);

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
  // Set the property values in a vtkSMProxy from the settings file(s).
  // Searches settings from the settings root.
  bool GetProxySettings(vtkSMProxy* proxy);

  // Description:
  // Get setting description
  std::string GetSettingDescription(const char* settingName);

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
  // Set the description of a setting.
  void SetSettingDescription(const char* settingName, const char* description);

protected:
  vtkSMSettings();
  virtual ~vtkSMSettings();

private:
  vtkSMSettings(const vtkSMSettings&); // Not implemented
  void operator=(const vtkSMSettings&); // Not implemented

  class vtkSMSettingsInternal;
  vtkSMSettingsInternal * Internal;
};

#endif
