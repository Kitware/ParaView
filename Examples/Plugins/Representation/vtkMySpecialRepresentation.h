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
// .NAME vtkMySpecialRepresentation
// .SECTION Description
//

#ifndef vtkMySpecialRepresentation_h
#define vtkMySpecialRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"

class VTK_EXPORT vtkMySpecialRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkMySpecialRepresentation* New();
  vtkTypeMacro(vtkMySpecialRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkMySpecialRepresentation();
  ~vtkMySpecialRepresentation();

private:
  vtkMySpecialRepresentation(const vtkMySpecialRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMySpecialRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
