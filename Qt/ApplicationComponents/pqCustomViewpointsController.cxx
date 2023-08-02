// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCustomViewpointsController.h"

#include "pqCustomViewpointsToolbar.h"

//-----------------------------------------------------------------------------
void pqCustomViewpointsController::setToolbar(pqCustomViewpointsToolbar* toolbar)
{
  Superclass::setParent(toolbar);
  this->Toolbar = toolbar;
}
