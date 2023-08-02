// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqConnect_h
#define pqConnect_h

#include "pqWidgetsModule.h"

class QObject;

/**
  Using pqConnect, you can create and initialize Qt objects without having to create a bunch of
  temporaries:

  \code
  menu->addAction("Open") << pqConnect(SIGNAL(triggered()), this, SLOT(onTriggered()));
  \endcode

  \sa pqSetName, pqSetData
*/

struct PQWIDGETS_EXPORT pqConnect
{
  pqConnect(const char* Signal, const QObject* Receiver, const char* Method);

  const char* Signal;
  const QObject* Receiver;
  const char* Method;
};

/**
 * Makes a Qt connection
 */
template <typename T>
T* operator<<(T* LHS, const pqConnect& RHS)
{
  LHS->connect(LHS, RHS.Signal, RHS.Receiver, RHS.Method);
  return LHS;
}

#endif // !pqConnect_h
