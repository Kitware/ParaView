/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerID.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientServerID -
// .SECTION Description
// vtkClientServerID

#ifndef __vtkClientServerID_h
#define __vtkClientServerID_h

#include "vtkType.h"

struct vtkClientServerID
{
  int operator<(const vtkClientServerID& i) const
    {
    return this->ID < i.ID;
    }
  int operator==(const vtkClientServerID& i) const
    {
    return this->ID == i.ID;
    }
  vtkTypeUInt32 ID;
};

#endif
