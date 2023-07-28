// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqNonOrthogonalAutoStart.h"
#include "pqModelTransformSupportBehavior.h"

//-----------------------------------------------------------------------------
pqNonOrthogonalAutoStart::pqNonOrthogonalAutoStart(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqNonOrthogonalAutoStart::~pqNonOrthogonalAutoStart() = default;

//-----------------------------------------------------------------------------
void pqNonOrthogonalAutoStart::startup()
{
  new pqModelTransformSupportBehavior(this);
}

//-----------------------------------------------------------------------------
void pqNonOrthogonalAutoStart::shutdown() {}
