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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
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
  ~vtkSurfaceVectors();

  // Usual data generation method
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  int ConstraintMode;

private:
  vtkSurfaceVectors(const vtkSurfaceVectors&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSurfaceVectors&) VTK_DELETE_FUNCTION;
};

#endif
