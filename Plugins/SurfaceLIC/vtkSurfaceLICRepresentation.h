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
// .NAME vtkSurfaceLICRepresentation
// .SECTION Description
// vtkSurfaceLICRepresentation extends vtkGeometryRepresentation to use surface
// lic when rendering surfaces.

#ifndef __vtkSurfaceLICRepresentation_h
#define __vtkSurfaceLICRepresentation_h

#include "vtkGeometryRepresentation.h"

class vtkSurfaceLICPainter;

class VTK_EXPORT vtkSurfaceLICRepresentation : public vtkGeometryRepresentation
{
public:
  static vtkSurfaceLICRepresentation* New();
  vtkTypeMacro(vtkSurfaceLICRepresentation, vtkGeometryRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Indicates whether LIC should be used when doing LOD rendering.
  void SetUseLICForLOD(bool val);

  //***************************************************************************
  // Forwarded to vtkSurfaceLICPainter
  void SetEnable(bool val);
  void SetNumberOfSteps(int val);
  void SetStepSize(double val);
  void SetLICIntensity(double val);
  void SetEnhancedLIC(int val);
  void SelectInputVectors(int, int, int, int attributeMode, const char* name);

//BTX
protected:
  vtkSurfaceLICRepresentation();
  ~vtkSurfaceLICRepresentation();

  vtkSurfaceLICPainter* Painter;
  vtkSurfaceLICPainter* LODPainter;

  bool UseLICForLOD;
private:
  vtkSurfaceLICRepresentation(const vtkSurfaceLICRepresentation&); // Not implemented
  void operator=(const vtkSurfaceLICRepresentation&); // Not implemented
//ETX
};

#endif
