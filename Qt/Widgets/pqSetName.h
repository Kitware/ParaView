// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSetName_h
#define pqSetName_h

#include "pqWidgetsModule.h"
#include <QString>

/**
  Using pqSetName, you can create and initialize Qt objects without having to create a bunch of
  temporaries:

  \code
  menu->addAction("Open") << pqSetName("FileOpenMenu");
  \endcode

  \sa pqSetData, pqConnect
*/

struct PQWIDGETS_EXPORT pqSetName
{
  pqSetName(const QString& Name);
  const QString Name;

private:
  void operator=(const pqSetName&);
};

/**
 * Sets a Qt object's name
 */
template <typename T>
T* operator<<(T* LHS, const pqSetName& RHS)
{
  LHS->setObjectName(RHS.Name);
  return LHS;
}

#endif // !pqSetName_h
