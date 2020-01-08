/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerID.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkClientServerID
 * @brief   Identifier for a ClientServer object.
 *
 * vtkClientServerID identifies an object managed by a
 * vtkClientServerInterpreter.  Although the identifier is simply an
 * integer, this class allows vtkClientServerStream to identify the
 * integer as an object identifier.
*/

#ifndef vtkClientServerID_h
#define vtkClientServerID_h

#include "vtkIOStream.h"                         // Needed for operator <<
#include "vtkRemotingClientServerStreamModule.h" // Top-level vtkClientServer header.
#include "vtkSystemIncludes.h"                   // vtkTypeUInt32

struct VTKREMOTINGCLIENTSERVERSTREAM_EXPORT vtkClientServerID
{
  vtkClientServerID()
    : ID(0)
  {
  }
  explicit vtkClientServerID(vtkTypeUInt32 id)
    : ID(id)
  {
  }

  bool IsNull() { return this->ID == 0; }
  void SetToNull() { this->ID = 0; }

  // Convenience operators.
  bool operator<(const vtkClientServerID& i) const { return this->ID < i.ID; }
  bool operator==(const vtkClientServerID& i) const { return this->ID == i.ID; }
  bool operator!=(const vtkClientServerID& i) const { return this->ID != i.ID; }
  // The identifying integer.
  vtkTypeUInt32 ID;
};

VTKREMOTINGCLIENTSERVERSTREAM_EXPORT ostream& operator<<(ostream& os, const vtkClientServerID& id);
VTKREMOTINGCLIENTSERVERSTREAM_EXPORT vtkOStreamWrapper& operator<<(
  vtkOStreamWrapper& os, const vtkClientServerID& id);

#endif
// VTK-HeaderTest-Exclude: vtkClientServerID.h
