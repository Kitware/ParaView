// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqIntegrationModelSurfaceHelperWidget_h
#define pqIntegrationModelSurfaceHelperWidget_h

#include "pqIntegrationModelHelperWidget.h"

class vtkStringArray;
class vtkPVDataInformation;

/// Class to represent ArraysToGenerate property
/// with the Lagrangian Surface Helper widget,
/// It manually creates the widgets and updates the properties
/// accordingly
class pqIntegrationModelSurfaceHelperWidget : public pqIntegrationModelHelperWidget
{
  Q_OBJECT
  Q_PROPERTY(QList<QVariant> arrayToGenerate READ arrayToGenerate WRITE setArrayToGenerate)
  typedef pqIntegrationModelHelperWidget Superclass;

public:
  pqIntegrationModelSurfaceHelperWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = nullptr);
  ~pqIntegrationModelSurfaceHelperWidget() override = default;

  QList<QVariant> arrayToGenerate() const;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  void setArrayToGenerate(const QList<QVariant>&);

Q_SIGNALS:
  void arrayToGenerateChanged();

protected Q_SLOTS:
  /// Create/Reset the widget
  void resetWidget() override;

protected: // NOLINT(readability-redundant-access-specifiers)
  /// Recursive method to fill leaf names from a potentially composite data information
  /// to a flat string array
  static vtkStringArray* fillLeafNames(
    vtkPVDataInformation* info, QString baseName, vtkStringArray* names);

private:
  Q_DISABLE_COPY(pqIntegrationModelSurfaceHelperWidget);

  /// Non virtual resetWidget method
  void resetSurfaceWidget(bool force);
};

#endif
