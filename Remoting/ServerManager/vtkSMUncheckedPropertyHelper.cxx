// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMUncheckedPropertyHelper.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkSMUncheckedPropertyHelper::vtkSMUncheckedPropertyHelper(
  vtkSMProxy* proxy, const char* name, bool quiet /* = false*/)
  : vtkSMPropertyHelper(proxy, name, quiet)
{
  this->setUseUnchecked(true);
}

//----------------------------------------------------------------------------
vtkSMUncheckedPropertyHelper::vtkSMUncheckedPropertyHelper(
  vtkSMProperty* property, bool quiet /* = false*/)
  : vtkSMPropertyHelper(property, quiet)
{
  this->setUseUnchecked(true);
}
