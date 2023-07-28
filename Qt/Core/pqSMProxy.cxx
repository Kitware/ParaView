// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSMProxy.h"

namespace
{
static unsigned int counter = 0;
}

pqSMProxySchwartzCounter::pqSMProxySchwartzCounter()
{
  if (counter == 0)
  {
    // register meta type for pqSMProxy
    qRegisterMetaType<pqSMProxy>("pqSMProxy");
  }
  counter++;
}
