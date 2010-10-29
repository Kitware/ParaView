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
// .NAME vtkGeometryRepresentationWithHiddenLinesRemoval
// .SECTION Description
//

#ifndef __vtkGeometryRepresentationWithHiddenLinesRemoval_h
#define __vtkGeometryRepresentationWithHiddenLinesRemoval_h

#include "vtkGeometryRepresentation.h"

class VTK_EXPORT vtkGeometryRepresentationWithHiddenLinesRemoval :
  public vtkGeometryRepresentation
{
public:
  static vtkGeometryRepresentationWithHiddenLinesRemoval* New();
  vtkTypeMacro(vtkGeometryRepresentationWithHiddenLinesRemoval, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkGeometryRepresentationWithHiddenLinesRemoval();
  ~vtkGeometryRepresentationWithHiddenLinesRemoval();

private:
  vtkGeometryRepresentationWithHiddenLinesRemoval(const vtkGeometryRepresentationWithHiddenLinesRemoval&); // Not implemented
  void operator=(const vtkGeometryRepresentationWithHiddenLinesRemoval&); // Not implemented
//ETX
};

#endif
