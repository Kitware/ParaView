/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPrismCubeAxesRepresentation - representation for a cube-axes.
// .SECTION Description
// vtkPrismCubeAxesRepresentation is a representation for the Cube-Axes
//customized to show the pre SESAME conversion table data ranges

#ifndef __vtkPrismCubeAxesRepresentation_h
#define __vtkPrismCubeAxesRepresentation_h

#include "vtkCubeAxesRepresentation.h"

class VTK_EXPORT vtkPrismCubeAxesRepresentation : public vtkCubeAxesRepresentation
{
public:
  static vtkPrismCubeAxesRepresentation* New();
  vtkTypeMacro(vtkPrismCubeAxesRepresentation, vtkCubeAxesRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetLabelRanges(
    double xmin, double xmax, double ymin, double ymax, double zmin, double zmax);

//BTX
protected:
  vtkPrismCubeAxesRepresentation();
  ~vtkPrismCubeAxesRepresentation();
private:
  vtkPrismCubeAxesRepresentation(const vtkPrismCubeAxesRepresentation&); // Not implemented
  void operator=(const vtkPrismCubeAxesRepresentation&); // Not implemented
//ETX
};

#endif
