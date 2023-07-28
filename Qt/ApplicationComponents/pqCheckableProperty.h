// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCheckableProperty_h
#define pqCheckableProperty_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

#include <memory>

/**
 * @class pqCheckableProperty
 * @brief property widget for properties representing a checkable property.
 *
 * pqCheckableProperty is designed for a specific use-case: a property that can be enabled
 * or disabled by a checkbox. This is useful for optional properties since it restrains the
 * user to use the property unless it is really needed.
 *
 * Example proxy definition(s) that use this widget is as follows:
 *
 * @code{xml}
 *     <ServerManagerConfiguration>
 *       <ProxyGroup name="sources">
 *         <SourceProxy name="CustomSource" class="vtkSphereSource">
 *           <DoubleVectorProperty name="PropertyA"
 *                              number_of_elements="1"
 *                              default_values="0">
 *             <DoubleRangeDomain name="range" min="0" max="10" />
 *           </DoubleVectorProperty>
 *
 *           <IntVectorProperty name="EnablePropertyA"
 *                              number_of_elements="1"
 *                              default_values="0">
 *             <BooleanDomain name="bool" />
 *           </IntVectorProperty>
 *
 *           <PropertyGroup label="Property A" panel_widget="CheckableProperty">
 *             <Property name="PropertyA" function="Property" />
 *             <Property name="EnablePropertyA" function="PropertyCheckBox" />
 *           </PropertyGroup>
 *         </SourceProxy>
 *       </ProxyGroup>
 *     </ServerManagerConfiguration>
 * @endcode
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqCheckableProperty : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqCheckableProperty(vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqCheckableProperty() override;

  void apply() override;
  void reset() override;
  void select() override;
  void deselect() override;
  void updateWidget(bool showing_advanced_properties) override;
  void setPanelVisibility(const char* vis) override;
  void setView(pqView*) override;
  bool isSingleRowItem() const override;

  bool enableCheckbox() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setEnableCheckbox(bool enableCheckbox);

private:
  Q_PROPERTY(bool enableCheckbox READ enableCheckbox WRITE setEnableCheckbox)

  struct Internal;
  std::unique_ptr<Internal> internal;
};

#endif
