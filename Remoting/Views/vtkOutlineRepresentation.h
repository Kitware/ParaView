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
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkOutlineRepresentation : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkOutlineRepresentation* New();
  vtkTypeMacro(vtkOutlineRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetRepresentation(const char*) override { this->Superclass::SetRepresentation("Wireframe"); }
  void SetUseOutline(int) override { this->Superclass::SetUseOutline(1); }
  void SetSuppressLOD(bool) override { this->Superclass::SetSuppressLOD(true); }
  void SetPickable(int) override { this->Superclass::SetPickable(0); }

protected:
  vtkOutlineRepresentation();
  ~vtkOutlineRepresentation() override;

  void SetRepresentation(int) override { this->Superclass::SetRepresentation(WIREFRAME); }

private:
  vtkOutlineRepresentation(const vtkOutlineRepresentation&) = delete;
  void operator=(const vtkOutlineRepresentation&) = delete;
};

#endif
