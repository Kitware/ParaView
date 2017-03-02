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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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

  //@{
  /**
   * Specify the resolution of the plane.
   */
  vtkSetClampMacro(Resolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(Resolution, int);
  //@}

protected:
  vtkBoundedPlaneSource();
  ~vtkBoundedPlaneSource();

  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  double Center[3];
  double Normal[3];
  double BoundingBox[6];
  int Resolution;

private:
  vtkBoundedPlaneSource(const vtkBoundedPlaneSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoundedPlaneSource&) VTK_DELETE_FUNCTION;
};

#endif
