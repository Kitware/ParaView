// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqShaderReplacementsComboBox_h
#define pqShaderReplacementsComboBox_h

#include "pqComponentsModule.h"
#include <QComboBox>

/**
 * This is a ComboBox that is used on the display tab to select available
 * ShaderReplacements presets paths saved in vtkSMSettings.
 */
class PQCOMPONENTS_EXPORT pqShaderReplacementsComboBox : public QComboBox
{
  Q_OBJECT
  typedef QComboBox Superclass;

public:
  pqShaderReplacementsComboBox(QWidget* parent = nullptr);
  ~pqShaderReplacementsComboBox() override = default;

  /**
   * Return the combobox entry index corresponding to the provided preset path.
   * Returned value is 0 if not found.
   */
  int getPathIndex(const QString& presetPath) const;

  /**
   * Select the combobox entry corresponding to the provided path.
   */
  void setPath(const QString& presetPath);

  /**
   * Clear current combobox content and repopulate it with the shader
   * replacement presets using the files declared in vtkSMSettings.
   */
  void populate();

  /**
   * Setting name of the variable that stores the paths of the presets.
   */
  static const char* ShaderReplacementPathsSettings;

protected:
  void showPopup() override;

private:
  Q_DISABLE_COPY(pqShaderReplacementsComboBox)
};

#endif
