// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqRecentlyUsedResourceLoaderInterface.h"

//-----------------------------------------------------------------------------
pqRecentlyUsedResourceLoaderInterface::pqRecentlyUsedResourceLoaderInterface() = default;

//-----------------------------------------------------------------------------
pqRecentlyUsedResourceLoaderInterface::~pqRecentlyUsedResourceLoaderInterface() = default;

//-----------------------------------------------------------------------------
QIcon pqRecentlyUsedResourceLoaderInterface::icon(const pqServerResource&)
{
  return QIcon();
}
