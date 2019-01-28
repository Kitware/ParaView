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

#include "GeometryRepresentationsModule.h" // for export macro
#include "vtkGeometryRepresentationWithFaces.h"

class GEOMETRYREPRESENTATIONS_EXPORT vtkMySpecialRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkMySpecialRepresentation* New();
  vtkTypeMacro(vtkMySpecialRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMySpecialRepresentation();
  ~vtkMySpecialRepresentation();

private:
  vtkMySpecialRepresentation(const vtkMySpecialRepresentation&) = delete;
  void operator=(const vtkMySpecialRepresentation&) = delete;
};

#endif
