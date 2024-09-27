// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkBivariateTextureRepresentation
 * @brief Representation to visualize bi-variate data with a texture.
 *
 * vtkBivariateTextureRepresentation allows to vizualize bi-variate data with a texture.
 * The representation takes two single-component point data arrays from the input
 * data (see the SetInputArrayToProcess method) and generates textures coordinates
 * from them. Values of each array are lineary mapped from 0 (min value) to 1 (max
 * value) to generate the 2D texture coordinates.
 */

#ifndef vtkBivariateTextureRepresentation_h
#define vtkBivariateTextureRepresentation_h

#include "vtkBivariateRepresentationsModule.h" // for export macro
#include "vtkDoubleArray.h"                    // for vtkDoubleArray
#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkNew.h" // for vtkNew

class vtkView;

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkBivariateTextureRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkBivariateTextureRepresentation* New();
  vtkTypeMacro(vtkBivariateTextureRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Force overload lookup on superclass
   */
  using Superclass::SetInputArrayToProcess;

  /**
   * Overriden to pass 1st and 2nd arrays to the mapper (if idx == 1 or 2).
   * These arrays are used to generate texture coordinates.
   */
  void SetInputArrayToProcess(int idx, int port, int connection, int fieldAssociation,
    const char* attributeTypeorName) override;

  ///@{
  /**
   * Get the range of the first / second array.
   */
  vtkGetVectorMacro(FirstArrayRange, double, 2);
  vtkGetVectorMacro(SecondArrayRange, double, 2);
  ///@}

  ///@{
  /**
   * Get the name of the first / second array.
   */
  vtkGetMacro(FirstArrayName, std::string);
  vtkGetMacro(SecondArrayName, std::string);
  ///@}

  /**
   * Overriden to force new rendering when setting a new texture.
   */
  void SetTexture(vtkTexture*) override;

protected:
  vtkBivariateTextureRepresentation();
  ~vtkBivariateTextureRepresentation() override;

  /**
   * Overriden to compute the texture coordinates from input point data arrays.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Overriden to prevent coloring with color array, i.e. when we only select
   * the "first array".
   */
  void UpdateColoringParameters() override{};

private:
  vtkBivariateTextureRepresentation(const vtkBivariateTextureRepresentation&) = delete;
  void operator=(const vtkBivariateTextureRepresentation&) = delete;

  vtkNew<vtkDoubleArray> TCoordsArray;

  double FirstArrayRange[2] = { 0, 0 };
  double SecondArrayRange[2] = { 0, 0 };

  std::string FirstArrayName;
  std::string SecondArrayName;
};

#endif

// VTK-HeaderTest-Exclude: vtkBivariateTextureRepresentation.h
