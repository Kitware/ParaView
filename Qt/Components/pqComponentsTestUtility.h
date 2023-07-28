// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqComponentsTestUtility_h
#define pqComponentsTestUtility_h

#include "pqComponentsModule.h"
#include "pqCoreTestUtility.h"

/**
 * pqComponentsTestUtility simply adds a pqComponents specific testing
 * capabilities to pqCoreTestUtility.
 */
class PQCOMPONENTS_EXPORT pqComponentsTestUtility : public pqCoreTestUtility
{
  Q_OBJECT
  typedef pqCoreTestUtility Superclass;

public:
  pqComponentsTestUtility(QObject* parentObj = nullptr);

  /**
   * Compares the baseline with active view for testing purposes.
   * (keeping naming-case similar to pqCoreTestUtility).
   */
  static bool CompareView(
    const QString& referenceImage, double threshold, const QString& tempDirectory);

private:
  Q_DISABLE_COPY(pqComponentsTestUtility)
};

#endif
