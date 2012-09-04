/*=========================================================================

  Program:   ParaView
  Module:    vtkSliceExtractorRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliceExtractorRepresentation
// .SECTION Description
// vtkSliceExtractorRepresentation select a single slice from a
// vtkMultiSliceRepresentation and make a representation out of that.

#ifndef __vtkSliceExtractorRepresentation_h
#define __vtkSliceExtractorRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"

class vtkMultiSliceRepresentation;

class vtkSliceExtractorRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkSliceExtractorRepresentation* New();
  vtkTypeMacro(vtkSliceExtractorRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Bind that representation to a given output port of a Multi-Slice representation
  void SetMultiSliceRepresentation(vtkMultiSliceRepresentation* multiSliceRep, int port);

//BTX
protected:
  vtkSliceExtractorRepresentation();
  ~vtkSliceExtractorRepresentation();

  friend class vtkCompositeMultiSliceRepresentation;

private:
  vtkSliceExtractorRepresentation(const vtkSliceExtractorRepresentation&); // Not implemented
  void operator=(const vtkSliceExtractorRepresentation&); // Not implemented
//ETX
};

#endif
