// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkBivariateNoiseRepresentation
 * @brief Representation to visualize bivariate data with noise
 *
 * The vtkBivariateNoiseRepresentation allows to visualize bivariate data with Perlin noise.
 * Please see the vtkBivariateNoiseMapper documentation for more information.
 *
 * @sa vtkBivariateNoiseMapper
 */
#ifndef vtkBivariateNoiseRepresentation_h
#define vtkBivariateNoiseRepresentation_h

#include "vtkBivariateRepresentationsModule.h" // for export macro
#include "vtkGeometryRepresentationWithFaces.h"

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkBivariateNoiseRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkBivariateNoiseRepresentation* New();
  vtkTypeMacro(vtkBivariateNoiseRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overriden to pass the noise array to the mapper (if idx == 1).
   */
  using Superclass::SetInputArrayToProcess; // Force overload lookup on superclass
  void SetInputArrayToProcess(int idx, int port, int connection, int fieldAssociation,
    const char* attributeTypeorName) override;

  ///@{
  /**
   * Noise parameters.
   * Forwarded to the mapper.
   */
  void SetFrequency(double fMod);
  void SetAmplitude(double aMod);
  void SetSpeed(double sMod);
  void SetNbOfOctaves(int nbOctaves);
  ///@}

protected:
  vtkBivariateNoiseRepresentation();
  ~vtkBivariateNoiseRepresentation() override;

private:
  vtkBivariateNoiseRepresentation(const vtkBivariateNoiseRepresentation&) = delete;
  void operator=(const vtkBivariateNoiseRepresentation&) = delete;
};

#endif

// VTK-HeaderTest-Exclude: vtkBivariateNoiseRepresentation.h
