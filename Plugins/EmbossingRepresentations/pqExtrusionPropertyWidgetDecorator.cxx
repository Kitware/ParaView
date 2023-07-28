// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqExtrusionPropertyWidgetDecorator.h"

#include "pqCoreUtilities.h"
#include "pqPropertyWidget.h"
#include "vtkCommand.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

//-----------------------------------------------------------------------------
pqExtrusionPropertyWidgetDecorator::pqExtrusionPropertyWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  vtkSMProxy* proxy = parentObject->proxy();
  this->ObservedObject1 = proxy ? proxy->GetProperty("ExtrusionNormalizeData") : nullptr;
  this->ObservedObject2 = proxy ? proxy->GetProperty("ExtrusionAutoScaling") : nullptr;

  if (!this->ObservedObject1)
  {
    qDebug("Could not locate property named 'ExtrusionNormalizeData'. "
           "pqExtrusionPropertyWidgetDecorator will have no effect.");
    return;
  }

  if (!this->ObservedObject2)
  {
    qDebug("Could not locate property named 'ExtrusionAutoScaling'. "
           "pqExtrusionPropertyWidgetDecorator will have no effect.");
    return;
  }

  this->ObserverId1 = pqCoreUtilities::connect(this->ObservedObject1,
    vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(visibilityChanged()));

  this->ObserverId2 = pqCoreUtilities::connect(this->ObservedObject2,
    vtkCommand::UncheckedPropertyModifiedEvent, this, SIGNAL(visibilityChanged()));
}

//-----------------------------------------------------------------------------
pqExtrusionPropertyWidgetDecorator::~pqExtrusionPropertyWidgetDecorator()
{
  if (this->ObservedObject1 && this->ObserverId1)
  {
    this->ObservedObject1->RemoveObserver(this->ObserverId1);
  }
  if (this->ObservedObject2 && this->ObserverId2)
  {
    this->ObservedObject2->RemoveObserver(this->ObserverId2);
  }
}

//-----------------------------------------------------------------------------
bool pqExtrusionPropertyWidgetDecorator::canShowWidget(bool show_advanced) const
{
  if (this->ObservedObject1 && this->ObservedObject2)
  {
    bool normalizeData = vtkSMUncheckedPropertyHelper(this->ObservedObject1).GetAsInt() == 1;
    bool autoScaling = vtkSMUncheckedPropertyHelper(this->ObservedObject2).GetAsInt() == 1;
    return normalizeData && !autoScaling;
  }

  return this->Superclass::canShowWidget(show_advanced);
}
