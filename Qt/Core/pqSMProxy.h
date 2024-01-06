// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqSMProxy_h
#define pqSMProxy_h

#include "pqCoreModule.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"
#include <QMetaType>

/**
 * make pqSMProxy synonymous to a smart pointer of a vtkSMProxy
 */
typedef vtkSmartPointer<vtkSMProxy> pqSMProxy;

/**
 * declare pqSMProxy for use with QVariant
 */
Q_DECLARE_METATYPE(pqSMProxy) // NOLINT(performance-no-int-to-ptr)

// use Schwartz counter idiom to correctly
// call qRegisterMetaType<> even on static builds when pqSMProxy
// header is included in any translation unit.
static class PQCORE_EXPORT pqSMProxySchwartzCounter
{
public:
  pqSMProxySchwartzCounter();
  ~pqSMProxySchwartzCounter() = default;
} pqSMProxySchwartzCounter;

#endif
