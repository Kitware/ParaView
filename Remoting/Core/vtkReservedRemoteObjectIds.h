// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkReservedRemoteObjectIds
 * @brief   Enum listing custom remote object
 * with dedicated GlobalID
 *
 * Enum listing custom remote object with dedicated GlobalID. Those IDs are
 * listed inside this enum to prevent any possible conflict.
 * ReservedId should be used if:
 *  - You have a vtkRemoteObject or a Proxy that is client only.
 *  - Only one instance in a session
 *  - It has to be shared across clients
 */

#ifndef vtkReservedRemoteObjectIds_h
#define vtkReservedRemoteObjectIds_h

#include "vtkRemotingCoreModule.h" // for export macros

struct VTKREMOTINGCORE_EXPORT vtkReservedRemoteObjectIds
{
  // This Enum allow the user to list a set of the reserved GlobalIds
  enum ReservedGlobalIds
  {
    RESERVED_PROXY_MANAGER_ID = 1,
    RESERVED_PROXY_DEFINITION_MANAGER_ID = 2,
    RESERVED_COLLABORATION_COMMUNICATOR_ID = 3,
    RESERVED_MAX_IDS = 255
  };
};
#endif

// VTK-HeaderTest-Exclude: vtkReservedRemoteObjectIds.h
