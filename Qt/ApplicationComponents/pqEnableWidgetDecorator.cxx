// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEnableWidgetDecorator.h"

//-----------------------------------------------------------------------------
pqEnableWidgetDecorator::pqEnableWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  QObject::connect(this, SIGNAL(boolPropertyChanged()), this, SIGNAL(enableStateChanged()));
}
