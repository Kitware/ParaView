// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSetData_h
#define pqSetData_h

#include "pqWidgetsModule.h"
#include <QVariant>

/**
  Using pqSetData, you can create and initialize Qt objects without having to create a bunch of
  temporaries:

  \code
  menu->addAction("Open") << pqSetData("My Private Data");
  \endcode

  \sa pqSetName, pqConnect
*/

struct PQWIDGETS_EXPORT pqSetData
{
  pqSetData(const QVariant& Data);
  const QVariant Data;

private:
  void operator=(const pqSetData&);
};

/**
 * Sets custom data for a Qt object
 */
template <typename T>
T* operator<<(T* LHS, const pqSetData& RHS)
{
  LHS->setData(RHS.Data);
  return LHS;
}

#endif // !pqSetData_h
