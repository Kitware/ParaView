// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqIntegrationModelSeedHelperWidget_h
#define pqIntegrationModelSeedHelperWidget_h

#include "pqIntegrationModelHelperWidget.h"

class vtkSMInputProperty;

/// Class to represent ArraysToGenerate property
/// with the Lagrangian Seed Helper widget,
/// It manually creates the widgets and updates the properties
/// accordingly
class pqIntegrationModelSeedHelperWidget : public pqIntegrationModelHelperWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> arrayToGenerate READ arrayToGenerate WRITE setArrayToGenerate)
  typedef pqIntegrationModelHelperWidget Superclass;

public:
  pqIntegrationModelSeedHelperWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = nullptr);
  ~pqIntegrationModelSeedHelperWidget() override = default;

  QList<QVariant> arrayToGenerate() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setArrayToGenerate(const QList<QVariant>&);

Q_SIGNALS:
  void arrayToGenerateChanged();

protected Q_SLOTS:
  /// Create/Reset the widget
  void resetWidget() override;
  void forceResetSeedWidget();

  /// Update the enable state of certain widgets in this widget
  void updateEnabledState();

private:
  Q_DISABLE_COPY(pqIntegrationModelSeedHelperWidget);

  /// Non-virtual resetWidget method
  void resetSeedWidget(bool force);

  vtkSMInputProperty* FlowInputProperty;
};

#endif
