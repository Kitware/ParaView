// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkRemotingViewsModule.h" //needed for exports

class VTKREMOTINGVIEWS_EXPORT vtkFeatureEdgesRepresentation
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
