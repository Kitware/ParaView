/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceVectors.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSurfaceVectors
 * @brief   Constrains vectors to surface.
 *
 * This filter works on point vectors.  It does not work on cell vectors yet.
 * A normal is conputed for a point by averaging normals of surrounding
 * 2D cells.  The vector is then constrained to be perpendicular to the normal.
*/

#ifndef vtkSurfaceVectors_h
#define vtkSurfaceVectors_h

#include "vtkDataSetAlgorithm.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkFloatArray;
class vtkIdList;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkSurfaceVectors : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkSurfaceVectors, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkSurfaceVectors* New();

  enum ConstraintMode
  {
    Parallel = 0,
    Perpendicular,
    PerpendicularScale
  };

  //@{
  /**
   * This mode determines whether this filter projects vectors to be
   * perpendicular to surface or parallel to surface.
   * It defaults to parallel.
   */
  vtkSetMacro(ConstraintMode, int);
  vtkGetMacro(ConstraintMode, int);
  void SetConstraintModeToParallel() { this->SetConstraintMode(vtkSurfaceVectors::Parallel); }
  void SetConstraintModeToPerpendicular()
  {
    this->SetConstraintMode(vtkSurfaceVectors::Perpendicular);
  }
  void SetConstraintModeToPerpendicularScale()
  {
    this->SetConstraintMode(vtkSurfaceVectors::PerpendicularScale);
  }
  //@}

protected:
  vtkSurfaceVectors();
  ~vtkSurfaceVectors() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int ConstraintMode;

private:
  vtkSurfaceVectors(const vtkSurfaceVectors&) = delete;
  void operator=(const vtkSurfaceVectors&) = delete;
};

#endif
