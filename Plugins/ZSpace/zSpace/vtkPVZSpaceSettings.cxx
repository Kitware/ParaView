// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVZSpaceSettings.h"

#include <cassert>

//-----------------------------------------------------------------------------
vtkPVZSpaceSettings* vtkPVZSpaceSettings::New()
{
  vtkPVZSpaceSettings* instance = GetInstance();
  assert(instance);
  instance->Register(nullptr);
  return instance;
}

//-----------------------------------------------------------------------------
vtkPVZSpaceSettings* vtkPVZSpaceSettings::GetInstance()
{
  if (!vtkPVZSpaceSettings::Instance)
  {
    vtkPVZSpaceSettings* instance = new vtkPVZSpaceSettings();
    instance->InitializeObjectBase();
    vtkPVZSpaceSettings::Instance.TakeReference(instance);
  }
  return vtkPVZSpaceSettings::Instance;
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetLeftButtonUseCustomMacro(bool state)
{
  this->SetUseCustomMacro(vtkZSpaceSDKManager::LeftButton, state);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetMiddleButtonUseCustomMacro(bool state)
{
  this->SetUseCustomMacro(vtkZSpaceSDKManager::MiddleButton, state);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetRightButtonUseCustomMacro(bool state)
{
  this->SetUseCustomMacro(vtkZSpaceSDKManager::RightButton, state);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetLeftButtonMacro(const std::string& macroName)
{
  this->SetMacroName(vtkZSpaceSDKManager::LeftButton, macroName);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetMiddleButtonMacro(const std::string& macroName)
{
  this->SetMacroName(vtkZSpaceSDKManager::MiddleButton, macroName);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetRightButtonMacro(const std::string& macroName)
{
  this->SetMacroName(vtkZSpaceSDKManager::RightButton, macroName);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetUseCustomMacro(vtkZSpaceSDKManager::ButtonIds buttonId, bool state)
{
  assert(buttonId >= 0 && buttonId < vtkZSpaceSDKManager::NumberOfButtons);

  this->ButtonSettings[buttonId].UseDefaultEvent = state;
  // If we use a custom macro, we need to disable the default behavior for this button.
  vtkZSpaceSDKManager::GetInstance()->SetUseDefaultBehavior(buttonId, !state);
}

//-----------------------------------------------------------------------------
void vtkPVZSpaceSettings::SetMacroName(
  vtkZSpaceSDKManager::ButtonIds buttonId, const std::string& macroName)
{
  assert(buttonId >= 0 && buttonId < vtkZSpaceSDKManager::NumberOfButtons);

  this->ButtonSettings[buttonId].MacroName = macroName;
}

//-----------------------------------------------------------------------------
const std::string& vtkPVZSpaceSettings::GetMacroNameFromButton(
  vtkZSpaceSDKManager::ButtonIds buttonId)
{
  assert(buttonId >= 0 && buttonId < vtkZSpaceSDKManager::NumberOfButtons);

  return GetInstance()->ButtonSettings[buttonId].MacroName;
}

//-----------------------------------------------------------------------------
bool vtkPVZSpaceSettings::GetUseCustomMacroFromButton(vtkZSpaceSDKManager::ButtonIds buttonId)
{
  assert(buttonId >= 0 && buttonId < vtkZSpaceSDKManager::NumberOfButtons);

  return GetInstance()->ButtonSettings[buttonId].UseDefaultEvent;
}
