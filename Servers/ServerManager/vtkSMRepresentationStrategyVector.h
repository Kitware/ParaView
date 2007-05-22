/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentationStrategyVector.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRepresentationStrategyVector - a std vector of
// vtkSMRepresentationStrategy pointers.
// .SECTION Description
//

#ifndef __vtkSMRepresentationStrategyVector_h
#define __vtkSMRepresentationStrategyVector_h

#include <vtkstd/vector>
#include "vtkSMRepresentationStrategy.h"
#include "vtkSmartPointer.h"

class vtkSMRepresentationStrategyVector : 
  public vtkstd::vector<vtkSmartPointer<vtkSMRepresentationStrategy> >
{

};

#endif

