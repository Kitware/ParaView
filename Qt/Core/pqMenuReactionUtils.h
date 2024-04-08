// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMenuReactionUtils_h
#define pqMenuReactionUtils_h

#include "pqCoreModule.h" // for exports
#include <QString>

class vtkSMDomain;
class vtkSMInputProperty;
class vtkSMProxy;

/**
 * Groups helper functions used by multiple pq*MenuReaction classes.
 *
 * @see pqExtractorsMenuReaction
 * @see pqFiltersMenuReaction
 */
namespace pqMenuReactionUtils
{
/**
 * Generates a QString with an error helper message for a given vtkSMDomain
 */
PQCORE_EXPORT QString getDomainDisplayText(vtkSMDomain* domain);

/**
 * Gets the vtkSMInputProperty for a given vtkSMProxy.
 *
 * @return nullptr_t if a proxy has no vtkSMInputProperty
 */
PQCORE_EXPORT vtkSMInputProperty* getInputProperty(vtkSMProxy* proxy);
}

#endif
