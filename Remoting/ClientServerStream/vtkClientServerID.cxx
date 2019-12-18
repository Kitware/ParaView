/*=========================================================================

  Program:   ParaView
  Module:    vtkClientServerID.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClientServerID.h"

#include "vtkIOStream.h"

ostream& operator<<(ostream& os, const vtkClientServerID& id)
{
  return os << id.ID;
}

vtkOStreamWrapper& operator<<(vtkOStreamWrapper& os, const vtkClientServerID& id)
{
  return os << id.ID;
}
