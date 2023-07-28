// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMyPropertyWidgetDecorator.h"

#include "pqCoreUtilities.h"
#include "pqPropertyWidget.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqMyPropertyWidgetDecorator::pqMyPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy ? proxy->GetProperty("ShrinkFactor") : nullptr;
  if (!prop)
  {
    qDebug("Could not locate property named 'ShrinkFactor'. "
           "pqMyPropertyWidgetDecorator will have no effect.");
    return;
  }

  this->ObservedObject = prop;
  this->ObserverId = pqCoreUtilities::connect(
    prop, vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(visibilityChanged()));
}

//-----------------------------------------------------------------------------
pqMyPropertyWidgetDecorator::~pqMyPropertyWidgetDecorator()
{
  if (this->ObservedObject && this->ObserverId)
  {
    this->ObservedObject->RemoveObserver(this->ObserverId);
  }
}

//-----------------------------------------------------------------------------
bool pqMyPropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  pqPropertyWidget* parentObject = this->parentWidget();
  vtkSMProxy* proxy = parentObject->proxy();
  vtkSMProperty* prop = proxy ? proxy->GetProperty("ShrinkFactor") : nullptr;
  if (prop)
  {
    double value = vtkSMUncheckedPropertyHelper(prop).GetAsDouble();
    if (value < 0.1)
    {
      return false;
    }
  }

  return this->Superclass::canShowWidget(show_advanced);
}
