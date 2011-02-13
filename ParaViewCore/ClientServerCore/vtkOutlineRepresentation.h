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
// .NAME vtkOutlineRepresentation - representation for outline.
// .SECTION Description
// vtkOutlineRepresentation is merely a vtkGeometryRepresentation that forces
// the geometry filter to produce outlines. It also

#ifndef __vtkOutlineRepresentation_h
#define __vtkOutlineRepresentation_h

#include "vtkGeometryRepresentation.h"

class VTK_EXPORT vtkOutlineRepresentation : public vtkGeometryRepresentation
{
public:
  static vtkOutlineRepresentation* New();
  vtkTypeMacro(vtkOutlineRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkOutlineRepresentation();
  ~vtkOutlineRepresentation();

private:
  vtkOutlineRepresentation(const vtkOutlineRepresentation&); // Not implemented
  void operator=(const vtkOutlineRepresentation&); // Not implemented
//ETX
};

#endif
