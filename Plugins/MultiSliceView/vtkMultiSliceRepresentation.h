/*=========================================================================

  Program:   ParaView
  Module:    vtkMultiSliceRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiSliceRepresentation
// .SECTION Description
// vtkMultiSliceRepresentation extends vtkGeometryRepresentation to add
// support for slicing data along the 3 axes.

#ifndef __vtkMultiSliceRepresentation_h
#define __vtkMultiSliceRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"

class vtkOrthogonalSliceFilter;

class vtkMultiSliceRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkMultiSliceRepresentation* New();
  vtkTypeMacro(vtkMultiSliceRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Manage slices normal to X
  void SetSliceX(int index, double sliceValue);
  void SetNumberOfSliceX(int size);

  // Description:
  // Manage slices normal to Y
  void SetSliceY(int index, double sliceValue);
  void SetNumberOfSliceY(int size);

  // Description:
  // Manage slices normal to Z
  void SetSliceZ(int index, double sliceValue);
  void SetNumberOfSliceZ(int size);

  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port, int conn);

//BTX
protected:
  vtkMultiSliceRepresentation();
  ~vtkMultiSliceRepresentation();

  vtkOrthogonalSliceFilter* InternalSliceFilter;
private:
  vtkMultiSliceRepresentation(const vtkMultiSliceRepresentation&); // Not implemented
  void operator=(const vtkMultiSliceRepresentation&); // Not implemented
//ETX
};

#endif
