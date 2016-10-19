/*=========================================================================

  Program:   ParaView
  Module:    vtkOutlineRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOutlineRepresentation
 * @brief   representation for outline.
 *
 * vtkOutlineRepresentation is merely a vtkGeometryRepresentationWithFaces that forces
 * the geometry filter to produce outlines.
*/

#ifndef vtkOutlineRepresentation_h
#define vtkOutlineRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkOutlineRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkOutlineRepresentation* New();
  vtkTypeMacro(vtkOutlineRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void SetRepresentation(const char*) { this->Superclass::SetRepresentation("Wireframe"); }
  virtual void SetUseOutline(int) { this->Superclass::SetUseOutline(1); }
  virtual void SetSuppressLOD(bool) { this->Superclass::SetSuppressLOD(true); }
  virtual void SetPickable(int) { this->Superclass::SetPickable(0); }

protected:
  vtkOutlineRepresentation();
  ~vtkOutlineRepresentation();

  virtual void SetRepresentation(int) { this->Superclass::SetRepresentation(WIREFRAME); }

private:
  vtkOutlineRepresentation(const vtkOutlineRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOutlineRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
