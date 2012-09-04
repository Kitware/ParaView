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
// vtkMultiSliceRepresentation extends vtkGeometryRepresentationWithFaces to add
// support for slicing data along the 3 axes.

#ifndef __vtkMultiSliceRepresentation_h
#define __vtkMultiSliceRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"

class vtkThreeSliceFilter;

class vtkMultiSliceRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkMultiSliceRepresentation* New();
  vtkTypeMacro(vtkMultiSliceRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ====== Methods usefull as proxy proeprty =======

  // Description:
  // Manage slices normal to X
  void SetSliceXNormal(double normal[3]);
  void SetSliceXOrigin(double origin[3]);
  void SetSliceXNormal(double normalX, double normalY, double normalZ);
  void SetSliceXOrigin(double originX, double originY, double originZ);
  void SetSliceX(int index, double sliceValue);
  void SetNumberOfSliceX(int size);

  // Description:
  // Manage slices normal to Y
  void SetSliceYNormal(double normal[3]);
  void SetSliceYOrigin(double origin[3]);
  void SetSliceYNormal(double normalX, double normalY, double normalZ);
  void SetSliceYOrigin(double originX, double originY, double originZ);
  void SetSliceY(int index, double sliceValue);
  void SetNumberOfSliceY(int size);

  // Description:
  // Manage slices normal to Z
  void SetSliceZNormal(double normal[3]);
  void SetSliceZOrigin(double origin[3]);
  void SetSliceZNormal(double normalX, double normalY, double normalZ);
  void SetSliceZOrigin(double originX, double originY, double originZ);
  void SetSliceZ(int index, double sliceValue);
  void SetNumberOfSliceZ(int size);

  // ====== More generic methods =======

  // Description:
  // Set a Slice Normal for a given cutter
  void SetCutNormal(int cutIndex, double normal[3]);

  // Description:
  // Set a slice Origin for a given cutter
  void SetCutOrigin(int cutIndex, double origin[3]);

  // Description:
  // Set a slice value for a given cutter
  void SetCutValue(int cutIndex, int index, double value);

  // Description:
  // Set number of slices for a given cutter
  void SetNumberOfSlice(int cutIndex, int size);

  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port, int conn);
  virtual vtkAlgorithmOutput* GetInternalOutputPort(int port);

  // Description:
  // Access the internal filter that is used to slice the input dataset
  vtkThreeSliceFilter* GetInternalVTKFilter();

  // Description:
  // Allow user to set which output port of the vtkThreeSliceFilter should be
  // used for that representation (Mapper)
  // 0: All | 1: SliceX | 2: SliceY | 3: SliceZ
  vtkSetMacro(PortToUse, int);
  vtkGetMacro(PortToUse, int);

//BTX
protected:
  vtkMultiSliceRepresentation();
  ~vtkMultiSliceRepresentation();

  vtkThreeSliceFilter* InternalSliceFilter;
  int PortToUse;
private:
  vtkMultiSliceRepresentation(const vtkMultiSliceRepresentation&); // Not implemented
  void operator=(const vtkMultiSliceRepresentation&); // Not implemented
//ETX
};

#endif
