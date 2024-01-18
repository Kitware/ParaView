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
 *
 * The representation adds the array "BivariateTCoords" as TCoords array to the
 * input dataset and can then be used to apply textures.
 *
 * The representation also holds a vtkLogoSourceRepresentation that is used to
 * display the texture as a logo in the current render view.
 */

#ifndef vtkBivariateTextureRepresentation_h
#define vtkBivariateTextureRepresentation_h

#include "vtkBivariateRepresentationsModule.h" // for export macro
#include "vtkDoubleArray.h"                    // for vtkDoubleArray
#include "vtkGeometryRepresentationWithFaces.h"
#include "vtkLogoSourceRepresentation.h" // for vtkLogoSourceRepresentation
#include "vtkNew.h"                      // for vtkNew
#include "vtkPVLogoSource.h"             // for vtkPVLogoSource
#include "vtkSmartPointer.h"             // for vtkSmartPointer

class vtkView;

class VTKBIVARIATEREPRESENTATIONS_EXPORT vtkBivariateTextureRepresentation
  : public vtkGeometryRepresentationWithFaces
{
public:
  static vtkBivariateTextureRepresentation* New();
  vtkTypeMacro(vtkBivariateTextureRepresentation, vtkGeometryRepresentationWithFaces);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Force overload lookup on superclass
  using Superclass::SetInputArrayToProcess;

  /**
   * Overriden to pass 1st and 2nd arrays to the mapper (if idx == 1 or 2).
   * These arrays are used to generate texture coordinates.
   */
  void SetInputArrayToProcess(int idx, int port, int connection, int fieldAssociation,
    const char* attributeTypeorName) override;

  //@{
  /**
   * Set/get the Logo source representation.
   * This representation is used to reprent the current 2D texture as a logo
   * in the current render view.
   */
  vtkSetSmartPointerMacro(LogoSourceRepresentation, vtkLogoSourceRepresentation);
  vtkGetSmartPointerMacro(LogoSourceRepresentation, vtkLogoSourceRepresentation);
  //@}

  /**
   * Overriden to initialize the internal vtkLogoSourceRepresentation as well.
   */
  unsigned int Initialize(unsigned int minIdAvailable, unsigned int maxIdAvailable) override;

  /**
   * Overridden to set the internal vtkLogoSourceRepresentation visibility as well.
   */
  void SetVisibility(bool visible) override;

  /**
   * Overriden to forward the texture to the internal vtkPVLogoSource.
   */
  void SetTexture(vtkTexture*) override;

protected:
  vtkBivariateTextureRepresentation();
  ~vtkBivariateTextureRepresentation() override;

  /**
   * Adds the representation to the view.
   * This is called from vtkView::AddRepresentation().
   * Returns true if the addition succeeds.
   *
   * Overriden to add the internal vtkLogoSourceRepresentation as well.
   */
  bool AddToView(vtkView* view) override;

  /**
   * Removes the representation to the view.
   * This is called from vtkView::RemoveRepresentation().
   * Returns true if the removal succeeds.
   *
   * Overriden to remove the internal vtkLogoSourceRepresentation as well.
   */
  bool RemoveFromView(vtkView* view) override;

  /**
   * Overriden to compute the texture coordinates from input point data arrays.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkBivariateTextureRepresentation(const vtkBivariateTextureRepresentation&) = delete;
  void operator=(const vtkBivariateTextureRepresentation&) = delete;

  vtkSmartPointer<vtkLogoSourceRepresentation> LogoSourceRepresentation;

  vtkNew<vtkDoubleArray> TCoordsArray;
  vtkNew<vtkPVLogoSource> LogoSource;
};

#endif

// VTK-HeaderTest-Exclude: vtkBivariateTextureRepresentation.h
