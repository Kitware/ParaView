// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#ifndef pqTransferFunctionWidgetPropertyDialog_h
#define pqTransferFunctionWidgetPropertyDialog_h

#include "pqApplicationComponentsModule.h" // for export macros
#include "vtkSmartPointer.h"               // For SmartPointer
#include <QDialog>
#include <QScopedPointer>

class vtkPiecewiseFunction;

class PQAPPLICATIONCOMPONENTS_EXPORT pqTransferFunctionWidgetPropertyDialog : public QDialog
{
  Q_OBJECT

public:
  pqTransferFunctionWidgetPropertyDialog(const QString& label,
    vtkPiecewiseFunction* transferFunction, QWidget* propertyWdg, QWidget* parentWdg = nullptr);
  ~pqTransferFunctionWidgetPropertyDialog() override;

private:
  vtkSmartPointer<vtkPiecewiseFunction> TransferFunction;
  QWidget* PropertyWidget;

  class pqInternals;
  const QScopedPointer<pqInternals> Internals;
};
#endif // pqTransferFunctionWidgetPropertyDialog_h
