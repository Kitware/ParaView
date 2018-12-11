/*=========================================================================

  Program:   ParaView
  Module:    vtkBoundedPlaneSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBoundedPlaneSource
 * @brief   a plane source bounded by a bounding box.
 *
 * vtkBoundedPlaneSource is a simple planar polydata generator that produces a
 * plane by intersecting a bounding box by a plane (specified by center and
 * normal).
*/

#ifndef vtkBoundedPlaneSource_h
#define vtkBoundedPlaneSource_h

#include "vtkPVVTKExtensionsPointsModule.h" // for export macro
#include "vtkPolyDataAlgorithm.h"
class VTKPVVTKEXTENSIONSPOINTS_EXPORT vtkBoundedPlaneSource : public vtkPolyDataAlgorithm
{
public:
  static vtkBoundedPlaneSource* New();
  vtkTypeMacro(vtkBoundedPlaneSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the center for the plane. Note that if the center is outside the
   * specified bounds, this source will produce empty poly data.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  //@}

  //@{
  /**
   * Get/Set the normal for the plane.
   */
  vtkSetVector3Macro(Normal, double);
  vtkGetVector3Macro(Normal, double);
  //@}

  //@{
  /**
   * Get/Set the bounding box for the plane.
   */
  vtkSetVector6Macro(BoundingBox, double);
  vtkGetVector6Macro(BoundingBox, double);
  //@}

  enum RefinementModes
  {
    USE_RESOLUTION,
    USE_CELL_SIZE
  };

  /**
   * Get/Set how the output refinement is to be determined.
   */
  vtkSetClampMacro(RefinementMode, int, USE_RESOLUTION, USE_CELL_SIZE);
  vtkGetMacro(RefinementMode, int);

  //@{
  /**
   * Specify the resolution of the plane. Used only when RefinementMode is set to
   * USE_RESOLUTION.
   */
  vtkSetClampMacro(Resolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Specify the cell-size of the plane. Used only when RefinementMode is set to
   * USE_CELL_SIZE.
   */
  vtkSetMacro(CellSize, double);
  vtkGetMacro(CellSize, double);
  //@}

  //@{
  /**
   * Specify the padding to use along each of the directions. This is used to
   * inflate the bounds by a fixed factor in all directions before generating
   * the plane.
   */
  vtkSetClampMacro(Padding, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Padding, double);
  //@}

protected:
  vtkBoundedPlaneSource();
  ~vtkBoundedPlaneSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Center[3];
  double Normal[3];
  double BoundingBox[6];
  int RefinementMode;
  int Resolution;
  double CellSize;
  double Padding;

private:
  vtkBoundedPlaneSource(const vtkBoundedPlaneSource&) = delete;
  void operator=(const vtkBoundedPlaneSource&) = delete;
};

#endif
