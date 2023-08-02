// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMoleculePropertyWidget_h
#define pqMoleculePropertyWidget_h

#include "pqPropertyGroupWidget.h"

struct MapperParameters;
class vtkSMPropertyGroup;
class QWidget;

/**
 * @class pqMoleculePropertyWidget
 * @brief Expose molecule mapper parameters to the user.
 *
 * Some presets are defined for an easiest configuration.
 */
class pqMoleculePropertyWidget : public pqPropertyGroupWidget
{
  Q_OBJECT
  typedef pqPropertyGroupWidget Superclass;

public:
  pqMoleculePropertyWidget(
    vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject = nullptr);
  ~pqMoleculePropertyWidget() override = default;

  /**
   * Override to show/hide relevant widgets depending on the advanced properties status.
   */
  void updateWidget(bool showing_advanced_properties) override;

protected Q_SLOTS:
  ///@{
  /**
   * Show/hide widgets depending on the states of other widgets.
   * Handle advanced proprerties visibility.
   */
  void updateBondWidgetsVisibility();
  void updateAtomWidgetsVisibility();
  void updateAtomicRadiusWidgetsVisibility();
  void updateBondColorWidgetVisibility();
  ///@}

  /**
   * Apply selected preset to the proxy
   */
  void onPresetChanged(int);

  /**
   * Reset the preset widget.
   * If a property has changed, preset widget should display '(no preset)'.
   */
  void resetPreset();

  ///@{
  /**
   * Update the atom/bond radius sliders bounds.
   */
  void onScaleAtomFactorChanged(double scale);
  void onResetAtomFactorToggled();
  void onScaleBondRadiusChanged(double scale);
  void onResetBondRadiusToggled();
  ///@}

protected: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Set property documentation as widget tooltip.
   */
  void setDocumentationAsTooltip(vtkSMProperty* prop, QWidget* widget);

private:
  Q_DISABLE_COPY(pqMoleculePropertyWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
