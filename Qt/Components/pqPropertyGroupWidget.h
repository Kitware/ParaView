// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPropertyGroupWidget_h
#define pqPropertyGroupWidget_h

#include "pqPropertyWidget.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QLineEdit;
class QSpinBox;
class QToolButton;
class QWidget;
class pqColorChooserButton;
class pqDoubleSliderWidget;
class vtkSMProxy;
class vtkSMPropertyGroup;

/**
 * pqPropertyGroupWidget is a (custom) widget created for a PropertyGroup.
 *
 * It provides functions for linking standard controls with Server Manager
 * properties as well as storing the property group.
 * @see vtkSMPropertyGroup
 */
class PQCOMPONENTS_EXPORT pqPropertyGroupWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqPropertyGroupWidget(vtkSMProxy* proxy, vtkSMPropertyGroup* smGroup, QWidget* parent = nullptr);
  vtkSMPropertyGroup* propertyGroup() const { return this->PropertyGroup; }
  // make this function accessible without class prefix
  using Superclass::addPropertyLink;
  void addPropertyLink(QComboBox* cb, const char* propertyName, int smindex = -1);
  void addPropertyLink(QLineEdit* edit, const char* propertyName, int smindex = -1);
  void addPropertyLink(QCheckBox* button, const char* propertyName, int smindex = -1);
  void addPropertyLink(QToolButton* button, const char* propertyName, int smindex = -1);
  void addPropertyLink(QGroupBox* groupBox, const char* propertyName, int smindex = -1);
  void addPropertyLink(QDoubleSpinBox* spinBox, const char* propertyName, int smindex = -1);
  void addPropertyLink(QSpinBox* spinBox, const char* propertyName, int smindex = -1);
  void addPropertyLink(pqColorChooserButton* color, const char* propertyName, int smindex = -1);
  void addPropertyLink(pqDoubleSliderWidget* slider, const char* propertyName, int smindex = -1);
  // make this signal public
  using Superclass::changeFinished;

  /**
   * Overwrite pqPropertyWidget to forward calls to vtkSMPropertyGroup
   */
  char* panelVisibility() const override;
  void setPanelVisibility(const char* vis) override;

private:
  void addCheckedPropertyLink(QWidget* button, const char* propertyName, int smindex = -1);
  void addDoubleValuePropertyLink(QWidget* widget, const char* propertyName, int smindex = -1);
  void addIntValuePropertyLink(QWidget* widget, const char* propertyName, int smindex = -1);
  void addStringPropertyLink(QWidget* widget, const char* propertyName, int smindex);

  vtkSMPropertyGroup* PropertyGroup;
};

#endif
