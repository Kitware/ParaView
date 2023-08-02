// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqIntegrationModelHelperWidget_h
#define pqIntegrationModelHelperWidget_h

#include "pqPropertyWidget.h"
#include "vtkEventQtSlotConnect.h" // For the connector
#include "vtkNew.h"                // For the connector

#include <QVariant>

class vtkSMProxyProperty;

/// Base class to represent the "ArraysToGenerate" Property.
/// pqIntegrationModelHelperWidget is a convenience class inherited
/// by pqIntegrationModelSeedHelperWidget and pqIntegrationModelSurfaceHelperWidget
/// It connects an "IntegrationModel" properties changes to a call
/// to updateWidget method.
class pqIntegrationModelHelperWidget : public pqPropertyWidget
{
  Q_OBJECT
  typedef pqPropertyWidget Superclass;

public:
  pqIntegrationModelHelperWidget(
    vtkSMProxy* smproxy, vtkSMProperty* smproperty, QWidget* parentObject = nullptr);
  ~pqIntegrationModelHelperWidget() override = default;

protected Q_SLOTS:
  virtual void resetWidget() = 0;

protected: // NOLINT(readability-redundant-access-specifiers)
  vtkNew<vtkEventQtSlotConnect> VTKConnector;
  vtkSMProxyProperty* ModelProperty;
  vtkSMProxy* ModelPropertyValue;

private:
  Q_DISABLE_COPY(pqIntegrationModelHelperWidget)
};

#endif
