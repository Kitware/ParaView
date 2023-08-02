// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTransferFunctionWidgetPropertyWidget_h
#define pqTransferFunctionWidgetPropertyWidget_h

#include "pqApplicationComponentsModule.h" // for export macros
#include "pqPropertyWidget.h"

class vtkSMTransferFunctionProxy;
class vtkEventQtSlotConnect;
class vtkSMRangedTransferFunctionDomain;
class pqTransferFunctionWidgetPropertyDialog;

/**
 * A property widget for editing a transfer function.
 *
 * To use this widget for a property add the
 * 'panel_widget="transfer_function_editor"' to the property's XML.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTransferFunctionWidgetPropertyWidget
  : public pqPropertyWidget
{
  Q_OBJECT

public:
  explicit pqTransferFunctionWidgetPropertyWidget(
    vtkSMProxy* proxy, vtkSMProperty* property, QWidget* parent = nullptr);
  ~pqTransferFunctionWidgetPropertyWidget() override;

  friend pqTransferFunctionWidgetPropertyDialog;

Q_SIGNALS:
  void domainChanged();

protected:
  void UpdateProperty();

protected Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void editButtonClicked();
  void propagateProxyPointsProperty();
  void resetRangeToDomainDefault();

  /**
   * Update widget with transfer function proxy value.
   */
  void updateRange();

  /**
   * Update transfer function proxy from widgets.
   */
  void onRangeEdited();

private:
  vtkEventQtSlotConnect* Connection;
  vtkSMTransferFunctionProxy* TFProxy;
  QDialog* Dialog;
  vtkSMRangedTransferFunctionDomain* Domain;

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};

#endif // pqTransferFunctionWidgetPropertyWidget_h
