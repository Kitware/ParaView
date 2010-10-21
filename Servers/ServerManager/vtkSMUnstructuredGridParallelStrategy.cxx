/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridParallelStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMUnstructuredGridParallelStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMUnstructuredGridParallelStrategy);
//----------------------------------------------------------------------------
vtkSMUnstructuredGridParallelStrategy::vtkSMUnstructuredGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMUnstructuredGridParallelStrategy::~vtkSMUnstructuredGridParallelStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMUnstructuredGridParallelStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


