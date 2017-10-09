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

  void SetRepresentation(const char*) VTK_OVERRIDE
  {
    this->Superclass::SetRepresentation("Wireframe");
  }
  void SetUseOutline(int) VTK_OVERRIDE { this->Superclass::SetUseOutline(1); }
  void SetSuppressLOD(bool) VTK_OVERRIDE { this->Superclass::SetSuppressLOD(true); }
  void SetPickable(int) VTK_OVERRIDE { this->Superclass::SetPickable(0); }

protected:
  vtkOutlineRepresentation();
  ~vtkOutlineRepresentation() override;

  void SetRepresentation(int) VTK_OVERRIDE { this->Superclass::SetRepresentation(WIREFRAME); }

private:
  vtkOutlineRepresentation(const vtkOutlineRepresentation&) = delete;
  void operator=(const vtkOutlineRepresentation&) = delete;
};

#endif
