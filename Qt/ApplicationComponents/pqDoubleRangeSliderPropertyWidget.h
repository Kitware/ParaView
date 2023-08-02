// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqDoubleRangeSliderPropertyWidget_h
#define pqDoubleRangeSliderPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

/**
 * pqDoubleRangeSliderPropertyWidget is a widget used for properties such as
 * the "ThresholdRange" property on the IsoVolume filter's panel. It provides
 * two double sliders, one for min and one for max and has logic to ensure that
 * the min <= max.
 *
 * The appearance of this widget can be modified by hints in the property XML
 * definition. If a hint element named "HideResetButton" is present, the range
 * reset button will be hidden. If a hint element named "MinimumLabel" is present with
 * a "text" attribute, that text attribute will be used as the label text instead
 * of "Minimum". Similarly, the default "Maximum" label can be replaced with a
 * "MaximumLabel" element with a "text" attribute.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqDoubleRangeSliderPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqDoubleRangeSliderPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqDoubleRangeSliderPropertyWidget() override;

  void apply() override;
  void reset() override;

protected Q_SLOTS:
  void highlightResetButton(bool highlight = true);
  void resetClicked();

private Q_SLOTS:
  /**
   * slots called when the slider(s) are moved.
   */
  void lowerChanged(double);
  void upperChanged(double);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqDoubleRangeSliderPropertyWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
