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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual void SetRepresentation(const char*) VTK_OVERRIDE
  {
    this->Superclass::SetRepresentation("Wireframe");
  }
  virtual void SetUseOutline(int) VTK_OVERRIDE { this->Superclass::SetUseOutline(1); }
  virtual void SetSuppressLOD(bool) VTK_OVERRIDE { this->Superclass::SetSuppressLOD(true); }
  virtual void SetPickable(int) VTK_OVERRIDE { this->Superclass::SetPickable(0); }

protected:
  vtkOutlineRepresentation();
  ~vtkOutlineRepresentation();

  virtual void SetRepresentation(int) VTK_OVERRIDE
  {
    this->Superclass::SetRepresentation(WIREFRAME);
  }

private:
  vtkOutlineRepresentation(const vtkOutlineRepresentation&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOutlineRepresentation&) VTK_DELETE_FUNCTION;
};

#endif
