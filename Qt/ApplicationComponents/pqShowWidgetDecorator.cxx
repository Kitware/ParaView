// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqShowWidgetDecorator.h"

//-----------------------------------------------------------------------------
pqShowWidgetDecorator::pqShowWidgetDecorator(
  vtkPVXMLElement* config, pqPropertyWidget* parentObject)
  : Superclass(config, parentObject)
{
  QObject::connect(this, SIGNAL(boolPropertyChanged()), this, SIGNAL(visibilityChanged()));
}
