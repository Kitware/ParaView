/*=========================================================================

  Program:   ParaView
  Module:    vtkReservedRemoteObjectIds.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReservedRemoteObjectIds - Enum listing custom remote object
// with dedicated GlobalID
// .SECTION Description
// Enum listing custom remote object with dedicated GlobalID. Those IDs are 
// listed inside this enum to prevent any possible conflict.

#ifndef __vtkReservedRemoteObjectIds_h
#define __vtkReservedRemoteObjectIds_h

struct VTK_EXPORT vtkReservedRemoteObjectIds
{
  // This Enum allow the user to list a set of the reserved GlobalIds
  enum ReservedGlobalIds
    {
    RESERVED_PROXY_MANAGER_ID = 1,
    RESERVED_PROXY_DEFINITION_MANAGER_ID = 2,
    RESERVED_MAX_IDS = 255
    };
};
#endif
