// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "pqCustomViewpointsController.h"

#include "pqCustomViewpointsToolbar.h"

//-----------------------------------------------------------------------------
void pqCustomViewpointsController::setToolbar(pqCustomViewpointsToolbar* toolbar)
{
  Superclass::setParent(toolbar);
  this->Toolbar = toolbar;
}
