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
// .NAME vtkClientServerID - Identifier for a ClientServer object.
// .SECTION Description
// vtkClientServerID identifies an object managed by a
// vtkClientServerInterpreter.  Although the identifier is simply an
// integer, this class allows vtkClientServerStream to identify the
// integer as an object identifier.

#ifndef __vtkClientServerID_h
#define __vtkClientServerID_h

#include "vtkClientServerConfigure.h" // Top-level vtkClientServer header.
#include "vtkSystemIncludes.h" // vtkTypeUInt32

struct VTK_CLIENT_SERVER_EXPORT vtkClientServerID
{
  // Convenience operators.
  int operator<(const vtkClientServerID& i) const
    {
    return this->ID < i.ID;
    }
  int operator==(const vtkClientServerID& i) const
    {
    return this->ID == i.ID;
    }
  int operator!=(const vtkClientServerID& i) const
    {
    return this->ID != i.ID;
    }
  // The identifying integer.
  vtkTypeUInt32 ID;
};

inline ostream& operator<<(ostream& os, const vtkClientServerID& id)
{
  return os << id.ID;
}

inline vtkOStreamWrapper& operator<<(vtkOStreamWrapper& os, const vtkClientServerID& id)
{
  return os << id.ID;
}

                      
#endif
