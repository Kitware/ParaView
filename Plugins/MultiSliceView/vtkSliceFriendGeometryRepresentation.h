/*=========================================================================

  Program:   ParaView
  Module:    vtkSliceFriendGeometryRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSliceFriendGeometryRepresentation
// .SECTION Description
// vtkSliceFriendGeometryRepresentation is a vtkGeometryRepresentationWithFaces
// which make our vtkCompositeSliceRepresentation a friend so it could access
// the protected method of it.

#ifndef __vtkSliceFriendGeometryRepresentation_h
#define __vtkSliceFriendGeometryRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"

class vtkSliceFriendGeometryRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkSliceFriendGeometryRepresentation* New();
  vtkTypeMacro(vtkSliceFriendGeometryRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkSetMacro(AllowInputConnectionSetting, bool);
  vtkGetMacro(AllowInputConnectionSetting, bool);
  vtkBooleanMacro(AllowInputConnectionSetting, bool);

  // Description:
  // Overridden to be able to disable them when we don't want to change the input
  virtual void SetInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void SetInputConnection(vtkAlgorithmOutput* input);
  virtual void AddInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void AddInputConnection(vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, vtkAlgorithmOutput* input);
  virtual void RemoveInputConnection(int port, int idx);

//BTX
protected:
  vtkSliceFriendGeometryRepresentation();
  ~vtkSliceFriendGeometryRepresentation();

  friend class vtkCompositeSliceRepresentation;

  bool AllowInputConnectionSetting;

private:
  vtkSliceFriendGeometryRepresentation(const vtkSliceFriendGeometryRepresentation&); // Not implemented
  void operator=(const vtkSliceFriendGeometryRepresentation&); // Not implemented
//ETX
};

#endif
