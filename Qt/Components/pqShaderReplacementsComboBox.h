/*=========================================================================

   Program: ParaView
   Module:    pqShaderReplacementsComboBox.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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
