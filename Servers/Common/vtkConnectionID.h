/*=========================================================================

  Program:   ParaView
  Module:    vtkConnectionID.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConnectionID - Identifier for a ProcessModuleConnection.
// .SECTION Description
// vtkConnectionID identifies a connection for the vtkProcessModule.

#ifndef __vtkConnectionID_h
#define __vtkConnectionID_h

#include "vtkSystemIncludes.h" // vtkTypeUInt32

struct VTK_EXPORT vtkConnectionID
{
  // Convenience operators.
  // Need to use vtkstd_bool type because std: :less requires bool return
  vtkstd_bool operator<(const vtkConnectionID& i) const
    {
    return this->ID < i.ID;
    }

  int operator==(const vtkConnectionID& i) const
    {
    return this->ID == i.ID;
    }

  int operator!=(const vtkConnectionID& i) const
    {
    return this->ID != i.ID;
    }
  // The identifying integer.
  vtkTypeUInt32 ID;
};

inline ostream& operator<<(ostream& os, const vtkConnectionID& id)
{
  return os << id.ID;
}

#endif

