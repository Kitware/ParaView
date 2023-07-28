// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
// .NAME vtkMySpecialRepresentation
// .SECTION Description
//

#ifndef vtkMySpecialRepresentation_h
#define vtkMySpecialRepresentation_h

#include "GeometryRepresentationsModule.h" // for export macro
#include "vtkGeometryRepresentationWithFaces.h"

class GEOMETRYREPRESENTATIONS_EXPORT vtkMySpecialRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkMySpecialRepresentation* New();
  vtkTypeMacro(vtkMySpecialRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMySpecialRepresentation();
  ~vtkMySpecialRepresentation();

private:
  vtkMySpecialRepresentation(const vtkMySpecialRepresentation&) = delete;
  void operator=(const vtkMySpecialRepresentation&) = delete;
};

#endif
