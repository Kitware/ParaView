// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqOMETransferFunctionsPropertyWidget_h
#define pqOMETransferFunctionsPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"
#include <QScopedPointer> // for QScopedPointer

class pqTransferFunctionWidget;

class PQAPPLICATIONCOMPONENTS_EXPORT pqOMETransferFunctionsPropertyWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqOMETransferFunctionsPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent = nullptr);
  ~pqOMETransferFunctionsPropertyWidget() override;

  bool event(QEvent* evt) override;

Q_SIGNALS:
  void xrgbPointsChanged();
  void xvmsPointsChanged();

private Q_SLOTS:
  void channelVisibilitiesChanged();
  void stcChanged(pqTransferFunctionWidget* src = nullptr);
  void pwfChanged(pqTransferFunctionWidget* src = nullptr);

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqOMETransferFunctionsPropertyWidget);
  class pqInternals;
  QScopedPointer<pqInternals> Internals;
  bool UpdatingProperty = false;

  void setXvmsPoints(int index, const QList<QVariant>& xvms);
  void setXrgbPoints(int index, const QList<QVariant>& xrgb);
};

#endif
