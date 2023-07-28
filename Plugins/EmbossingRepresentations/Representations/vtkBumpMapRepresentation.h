// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBumpMapRepresentation
 * @brief   Representation for showing data sets with bump mapping effect.
 *
 * This representation display its input mesh using a bump mapping effect.
 * A bump mapping effect is a modification of the normals in order to add
 * embossing on a flat mesh thanks to the lights of the scene.
 *
 * @par Thanks:
 * This class was written by Joachim Pouderoux and Michael Migliore, Kitware 2017
 * This work was supported by the German Climate Computing Center (DKRZ).
 */

#ifndef vtkBumpMapRepresentation_h
#define vtkBumpMapRepresentation_h

#include "vtkEmbossingRepresentationsModule.h" // for export macro
#include "vtkGeometryRepresentationWithFaces.h"

class VTKEMBOSSINGREPRESENTATIONS_EXPORT vtkBumpMapRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkBumpMapRepresentation* New();
  vtkTypeMacro(vtkBumpMapRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Bump mapping factor information.
   */
  void SetBumpMappingFactor(double val);

  /**
   * Sets the array used by the representation.
   */
  void SetInputDataArray(int idx, int port, int connection, int fieldAssociation, const char* name);

protected:
  vtkBumpMapRepresentation();
  ~vtkBumpMapRepresentation() override = default;

private:
  vtkBumpMapRepresentation(const vtkBumpMapRepresentation&) = delete;
  void operator=(const vtkBumpMapRepresentation&) = delete;
};

#endif
