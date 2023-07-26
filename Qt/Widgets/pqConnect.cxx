// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "pqConnect.h"

pqConnect::pqConnect(const char* signal, const QObject* receiver, const char* method)
  : Signal(signal)
  , Receiver(receiver)
  , Method(method)
{
}
