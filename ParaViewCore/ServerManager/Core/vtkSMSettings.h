/*=========================================================================

  Program:   ParaView

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSMSettings
 *
 * vtkSMSettings provides the underlying mechanism for setting default property
 * values in ParaView.
 *
 * This class is a singleton class. Instances should be retrieved with the
 * GetInstance() method.
 *
 * This class provides the facilities for defining a
 * linear hierarchy of setting collections. A setting collection is a group of
 * not-necessarily-related settings defined in a string or text file. The text
 * defining a setting collection is formatted in JSON. Collections can be added
 * using the methods AddSettingsFromString() and AddSettingsFromFile().
 *
 * Each setting collection has an associated priority. The priority is used
 * to select the collection from which a setting should be retrieved when
 * more than one collection has the same definition. A setting in a collection
 * with a higher priority than another collection with the same setting
 * has precedence and will be returned by the "Get*" methods in this class.
 *
 * Settings for proxies and proxy properties are defined by specifying the
 * XML names of the proxy group, proxies, and properties in a three-level
 * hierarchy. For example, the Sphere Source settings can be defined by the
 * following:
 *
 * \pre{
 * \{
 *   "sources" : \{
 *     "SphereSource" : \{
 *       "Radius" : 2.5,
 *       "Center" : [0.0, 1.0, 0.0]
 *     \}
 *   /}
 * \}
 * }
 *
 * In this example, "sources" is the proxy group, "SphereSource" is the name of
 * a proxy, and "Radius" and "Center" are properties of the proxy.
 *
 * Vector properties with a single element can be defined as a single element
 * (e.g., 2.5) or as a single-element array (e.g., [2.5]). Multi-element vector
 * properties are specified as arrays (e.g., [0.0, 1.0, 0.0]).
 *
 * The "Set*" and "Get*" methods of this class take a character string specifying
 * the setting name. This string has the format ".level1.level2.level3[index]".
 * For example, to retrieve the y-component of the sphere center in the example
 * JSON above, one would write ".sources.SphereSource.Center[1]". Only literal
 * values (int, double, and string) are available through this interface;
 * access to non-leaf nodes in the JSON format is not provided.
 *
 * This class supports setting setting values. Settings modified through the
 * "Set*" methods modify thet setting collection that has priority
 * over all other collections. This collection can be saved to a text file in
 * JSON format using the SaveSettingsToFile() method.
 *
 * Some convenience methods for getting and setting proxy property values are
 * provided. GetProxySettings() sets the values of proxy properties that are
 * defined in the setting collections. SetProxySettings() saves the non-default
 * proxy properties in the highest-priority collection.
*/

#ifndef vtkSMSettings_h
#define vtkSMSettings_h

#include "vtkObject.h"
#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkStdString.h"                 // needed for vtkStdString.
#include <vector>                         // needed for vector.
#include <vtk_jsoncpp_fwd.h>              // for forward declarations

class vtkSMProperty;
class vtkSMProxy;
class vtkSMPropertyIterator;
class VTKPVSERVERMANAGERCORE_EXPORT vtkSMSettings : public vtkObject
{
public:
  static vtkSMSettings* New();
  vtkTypeMacro(vtkSMSettings, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get singleton instance.
   */
  static vtkSMSettings* GetInstance();

  /**
   * Add settings from a string. The string should contain valid JSON-formatted
   * text. The "priority" indicates how to treat a setting that has definitions
   * in more than one setting collections. If two setting collections
   * contain values for the same setting, then the setting from the
   * collection with higher priority will be used.
   */
  bool AddCollectionFromString(const std::string& settings, double priority);

  /**
   * The same as AddCollectionFromString, but this method reads the settings
   * string from the named file. The fileName should be a full path.
   */
  bool AddCollectionFromFile(const std::string& fileName, double priority);

  /**
   * Clear out all settings, deleting all collections.
   */
  void ClearAllSettings();

  /**
   * Distribute setting collections to all processes if in batch symmetric mode.
   */
  bool DistributeSettings();

  /**
   * Save highest priority setting collection to file.
   */
  bool SaveSettingsToFile(const std::string& filePath);

  /**
   * Check whether a setting is defined for the requested names.
   */
  bool HasSetting(const char* settingName);

  /**
   * Check whether a setting is defined for the requested names
   * at or below a maximum priority level.
   */
  bool HasSetting(const char* settingName, double maxPriority);

  /**
   * Get the number of elements in a setting.
   */
  unsigned int GetSettingNumberOfElements(const char* settingName);

  //@{
  /**
   * Get a vector setting as a scalar value.
   * Shortcut for GetSettingAs...(settingName, 0, defaultValue)
   */
  int GetSettingAsInt(const char* settingName, int defaultValue);
  double GetSettingAsDouble(const char* settingName, double defaultValue);
  std::string GetSettingAsString(const char* settingName, const std::string& defaultValue);
  //@}

  //@{
  /**
   * Get a single element of a vector setting.
   */
  int GetSettingAsInt(const char* settingName, unsigned int index, int defaultValue);
  double GetSettingAsDouble(const char* settingName, unsigned int index, double defaultValue);
  std::string GetSettingAsString(
    const char* settingName, unsigned int index, const std::string& defaultValue);
  //@}

  /**
   * Set the property value from the setting collections.
   */
  bool GetPropertySetting(vtkSMProperty* property);

  /**
   * Set the property value from the setting collections that have priority
   * at or less than the given priority.
   */
  bool GetPropertySetting(vtkSMProperty* property, double maxPriority);

  /**
   * Set the property value from the setting collections under the given prefix.
   */
  bool GetPropertySetting(const char* prefix, vtkSMProperty* property);

  /**
   * Set the property value from the setting collections under the given prefix.
   * that have priority at or less than the given priority.
   */
  bool GetPropertySetting(const char* prefix, vtkSMProperty* property, double maxPriority);

  /**
   * Set the property values in a vtkSMProxy from the setting collections.
   */
  bool GetProxySettings(vtkSMProxy* proxy);

  /**
   * Set the property values in a vtkSMProxy from the setting collections
   * that have priority at or less than the given priority.
   */
  bool GetProxySettings(vtkSMProxy* proxy, double maxPriority);

  /**
   * Set the property values in a vtkSMProxy from the settings collections
   * under the given prefix.
   */
  bool GetProxySettings(const char* prefix, vtkSMProxy* proxy);

  /**
   * Set the property values in a vtkSMProxy from the settings collections
   * under the given prefix at or less than the given priority.
   */
  bool GetProxySettings(const char* prefix, vtkSMProxy* proxy, double maxPriority);
  ;

  /**
   * Get description for a setting.
   */
  std::string GetSettingDescription(const char* settingName);

  //@{
  /**
   * Set setting of a given name in the highest priority collection.
   * Shortcut for SetSetting(settingName, 0, value). Useful for setting scalar values.
   */
  void SetSetting(const char* settingName, int value);
  void SetSetting(const char* settingName, double value);
  void SetSetting(const char* settingName, const std::string& value);
  //@}

  //@{
  /**
   * Set element of a vector setting at a location given by the setting name.
   */
  void SetSetting(const char* settingName, unsigned int index, int value);
  void SetSetting(const char* settingName, unsigned int index, double value);
  void SetSetting(const char* settingName, unsigned int index, const std::string& value);
  //@}

  /**
   * Save non-default settings in the current user settings.
   * Use the optional 'propertyIt' to limit the serialization to a subset of
   * properties (typically using vtkSMPropertyIterator subclasses like
   * vtkSMNamedPropertyIterator).
   * \c skipPropertiesWithDynamicDomains when true (default) ensures that we
   * skip serializing properties that have domains whose values change at
   * runtime.
   */
  void SetProxySettings(vtkSMProxy* proxy, vtkSMPropertyIterator* propertyIt = NULL,
    bool skipPropertiesWithDynamicDomains = true);

  /**
   * Save non-default settings in the current user settings under the given prefix.
   * Use the optional 'propertyIt' to limit the serialization to a subset of
   * properties (typically using vtkSMPropertyIterator subclasses like
   * vtkSMNamedPropertyIterator).
   * \c skipPropertiesWithDynamicDomains when true (default) ensures that we
   * skip serializing properties that have domains whose values change at
   * runtime.
   */
  void SetProxySettings(const char* prefix, vtkSMProxy* proxy,
    vtkSMPropertyIterator* propertyIt = NULL, bool skipPropertiesWithDynamicDomains = true);

  /**
   * Set the description of a setting.
   */
  void SetSettingDescription(const char* settingName, const char* description);

  /**
   * Saves the state of the proxy as JSON. The implementation simply
   * converts the state XML to JSON.
   */
  static Json::Value SerializeAsJSON(vtkSMProxy* proxy, vtkSMPropertyIterator* iter = NULL);

  /**
   * Restores a proxy state using the JSON equivalent. The implementation
   * converts JSON to XML state for the proxy and then attempts to restore the
   * XML state.
   */
  static bool DeserializeFromJSON(vtkSMProxy* proxy, const Json::Value& value);

protected:
  vtkSMSettings();
  ~vtkSMSettings() override;

private:
  vtkSMSettings(const vtkSMSettings&) = delete;
  void operator=(const vtkSMSettings&) = delete;

  class vtkSMSettingsInternal;
  vtkSMSettingsInternal* Internal;
};

#endif
