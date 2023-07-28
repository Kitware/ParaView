// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqInputSelectorWidget_h
#define pqInputSelectorWidget_h

#include "pqComponentsModule.h" // for exports
#include "pqPropertyWidget.h"
#include "pqSMProxy.h"       // for pqSMProxy.
#include "pqTimer.h"         // for pqTimer
#include "vtkSmartPointer.h" // for vtkSmartPointer

class QComboBox;
class vtkSMProxy;

/**
 * @class pqInputSelectorWidget
 * @brief widget for input property to choose a pipeline input.
 *
 * Generally, pqProxyWidget does not show widget for input properties. This is
 * by design since changing input to filters is generally not a thing we let the
 * user treat at same level as changing other properties. In some cases,
 * however, we may want to show a combo-box to let user pick the input from the
 * available pipeline proxies. pqInputSelectorWidget is designed for such
 * exceptional cases. To use this widget for a vtkSMInputProperty, set
 * `panel_widget` attribute to `input_selector`.
 *
 */
class PQCOMPONENTS_EXPORT pqInputSelectorWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;
  Q_PROPERTY(pqSMProxy selectedInput READ selectedInput WRITE setSelectedInput)

public:
  pqInputSelectorWidget(vtkSMProxy* proxy, vtkSMProperty* smproperty, QWidget* parent = nullptr);
  ~pqInputSelectorWidget() override;

  /**
   * Chosen port. This returns `vtkSmartPointer<vtkSMOutputPort>`
   * for the chosen port. if any.
   */
  pqSMProxy selectedInput() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setSelectedInput(pqSMProxy);

Q_SIGNALS:
  void selectedInputChanged();

private Q_SLOTS:
  void updateComboBox();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqInputSelectorWidget);
  QComboBox* ComboBox;
  vtkWeakPointer<vtkSMProxy> ChosenPort;
  pqTimer UpdateComboBoxTimer;
};

#endif
