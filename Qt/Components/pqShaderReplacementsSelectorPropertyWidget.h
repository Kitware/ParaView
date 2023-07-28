// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqShaderReplacementsSelectorPropertyWidget_h
#define pqShaderReplacementsSelectorPropertyWidget_h

#include "pqComponentsModule.h"

#include "pqPropertyWidget.h"

/**
* Property widget for selecting the ShaderReplacements to apply to a geometry.
*
* To use this widget for a property add 'panel_widget="shader_replacements_selector"'
* to the property's XML.

*/
class PQCOMPONENTS_EXPORT pqShaderReplacementsSelectorPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
public:
  pqShaderReplacementsSelectorPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqShaderReplacementsSelectorPropertyWidget() override;

protected Q_SLOTS:
  void updateShaderReplacements();
  void textChangedAndEditingFinished();
  void onLoad();
  void onDelete();
  void onPresetChanged(int index);

protected: // NOLINT(readability-redundant-access-specifiers)
  bool loadShaderReplacements(const QString& filename);
  void refreshView();

private:
  class pqInternal;
  pqInternal* Internal;
};

#endif
