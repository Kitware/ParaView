/*=========================================================================

  Program:   ParaView
  Module:    vtkMySpecialPolyDataMapper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMySpecialPolyDataMapper.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMySpecialPolyDataMapper);
//----------------------------------------------------------------------------
vtkMySpecialPolyDataMapper::vtkMySpecialPolyDataMapper()
{
}

//----------------------------------------------------------------------------
vtkMySpecialPolyDataMapper::~vtkMySpecialPolyDataMapper()
{
}

//----------------------------------------------------------------------------
void vtkMySpecialPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
