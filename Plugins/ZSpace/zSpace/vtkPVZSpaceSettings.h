// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkPVZSpaceSettings_h
#define vtkPVZSpaceSettings_h

#include "vtkObject.h"
#include "vtkSetGet.h"
#include "vtkSmartPointer.h"
#include "vtkZSpaceSDKManager.h"
#include "vtkZSpaceViewModule.h"

#include <string>

/**
 * Class containning all settings for the zSpace plugin.
 * - If "Use custom macro" is set to true for Left/Middle/Right, then the zSpace stylus will trigger
 * a user defined macro for its assigned button.
 * - The Left/Middle/Right button macro setting sets the macro name for the middle, left or right
 * button respectively.
 */
class VTKZSPACEVIEW_EXPORT vtkPVZSpaceSettings : public vtkObject
{
public:
  static vtkPVZSpaceSettings* New();
  vtkTypeMacro(vtkPVZSpaceSettings, vtkObject);

  /**
   * Return the instance of the zSpace settings singleton.
   */
  static vtkPVZSpaceSettings* GetInstance();

  ///@{
  /**
   * Set the whether or not to use a custom macro per button.
   */
  void SetLeftButtonUseCustomMacro(bool state);
  void SetMiddleButtonUseCustomMacro(bool state);
  void SetRightButtonUseCustomMacro(bool state);
  ///@}

  ///@{
  /**
   * Set the macro name per button.
   */
  void SetLeftButtonMacro(const std::string& macroName);
  void SetMiddleButtonMacro(const std::string& macroName);
  void SetRightButtonMacro(const std::string& macroName);
  ///@}

  ///@{
  /**
   * Return the macro name associated to the given buttonId. If there is no macro attached, it
   * returns an empty string.
   */
  static const std::string& GetMacroNameFromButton(vtkZSpaceSDKManager::ButtonIds buttonId);
  static bool GetUseCustomMacroFromButton(vtkZSpaceSDKManager::ButtonIds buttonId);
  ///@}

protected:
  vtkPVZSpaceSettings() = default;
  ~vtkPVZSpaceSettings() override = default;

private:
  vtkPVZSpaceSettings(const vtkPVZSpaceSettings&) = delete;
  void operator=(const vtkPVZSpaceSettings&) = delete;

  ///@{
  /**
   * Set the button settings for a given macro.
   */
  void SetUseCustomMacro(vtkZSpaceSDKManager::ButtonIds buttonId, bool state);
  void SetMacroName(vtkZSpaceSDKManager::ButtonIds buttonId, const std::string& macroName);
  ///@}

  struct ButtonSettings
  {
    ButtonSettings() = default;

    bool UseDefaultEvent = true;
    std::string MacroName;
  };
  ButtonSettings ButtonSettings[vtkZSpaceSDKManager::NumberOfButtons];

  static inline vtkSmartPointer<vtkPVZSpaceSettings> Instance = nullptr;
};

#endif
