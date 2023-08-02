// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqIntegrationModelHelperWidget.h"

#include "vtkSMProxyProperty.h"
#include "vtkSMStringVectorProperty.h"

//-----------------------------------------------------------------------------
pqIntegrationModelHelperWidget::pqIntegrationModelHelperWidget(
  vtkSMProxy* smproxy, vtkSMProperty* vtkNotUsed(smproperty), QWidget* parentObject)
  : Superclass(smproxy, parentObject)
{
  this->setShowLabel(false);
  this->setChangeAvailableAsChangeFinished(true);

  // Connect the IntegrationModel property to the reset slot
  // so the widget is reset when the model is changed.
  this->ModelProperty =
    vtkSMProxyProperty::SafeDownCast(this->proxy()->GetProperty("IntegrationModel"));
  this->ModelPropertyValue = this->ModelProperty->GetProxy(0);
  this->VTKConnector->Connect(
    this->ModelProperty, vtkCommand::UncheckedPropertyModifiedEvent, this, SLOT(resetWidget()));
}
