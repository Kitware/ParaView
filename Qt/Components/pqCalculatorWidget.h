// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqCalculatorWidget_h
#define pqCalculatorWidget_h

#include "pqPropertyWidget.h"

/**
 * pqCalculatorWidget is a property-widget that can shows a calculator-like
 * UI for the property. It is designed to be used with vtkPVArrayCalculator (or
 * similar) filters. It allows users to enter expressions to compute derived
 * quantities. To determine the list of input arrays available.
 *
 * CAVEATS: Currently, this widget expects two additional properties on the
 * proxy: "Input", that provides the input and "AttributeMode", which
 * corresponds to the chosen attribute type. This code can be revised to use
 * RequiredProperties on the domain to make it reuseable, if needed.
 */
class PQCOMPONENTS_EXPORT pqCalculatorWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqCalculatorWidget(vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqCalculatorWidget() override;

protected Q_SLOTS:
  /**
   * called when the user selects a variable from the scalars/vectors menus.
   */
  void variableChosen(QAction* action);

  /**
   * called when user clicks one of the function buttons
   */
  void buttonPressed(const QString&);

  /**
   * updates the variables in the menus.
   */
  void updateVariableNames();
  void updateVariables(const QString& mode);

  /**
   * Update button labels based on which function parser is active.
   */
  void updateButtons();

private:
  Q_DISABLE_COPY(pqCalculatorWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
