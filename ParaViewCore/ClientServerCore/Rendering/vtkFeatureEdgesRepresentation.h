/*=========================================================================

  Program:   ParaView
  Module:    vtkFeatureEdgesRepresentation.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFeatureEdgesRepresentation
 * @brief   representation for feature edges.
 *
 * vtkFeatureEdgesRepresentation is merely a vtkGeometryRepresentationWithFaces that forces
 * the geometry filter to produce feature edges.
*/

#ifndef vtkFeatureEdgesRepresentation_h
#define vtkFeatureEdgesRepresentation_h

#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkPVClientServerCoreRenderingModule.h" //needed for exports

class VTKPVCLIENTSERVERCORERENDERING_EXPORT vtkFeatureEdgesRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkFeatureEdgesRepresentation* New();
  vtkTypeMacro(vtkFeatureEdgesRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetRepresentation(const char*) override { this->Superclass::SetRepresentation("Wireframe"); }
  void SetUseOutline(int) override { this->Superclass::SetUseOutline(0); }
  void SetSuppressLOD(bool) override { this->Superclass::SetSuppressLOD(true); }
  void SetPickable(int) override { this->Superclass::SetPickable(0); }
  void SetGenerateFeatureEdges(bool) override { this->Superclass::SetGenerateFeatureEdges(true); }

protected:
  vtkFeatureEdgesRepresentation();
  ~vtkFeatureEdgesRepresentation() override;

  void SetRepresentation(int) override { this->Superclass::SetRepresentation(WIREFRAME); }

private:
  vtkFeatureEdgesRepresentation(const vtkFeatureEdgesRepresentation&) = delete;
  void operator=(const vtkFeatureEdgesRepresentation&) = delete;
};

#endif
