// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqConnect.h"

pqConnect::pqConnect(const char* signal, const QObject* receiver, const char* method)
  : Signal(signal)
  , Receiver(receiver)
  , Method(method)
{
}
