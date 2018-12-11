/*=========================================================================

  Program:   ParaView
  Module:    vtkBoundedVolumeSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBoundedVolumeSource
 * @brief   a source to generate an image volume.
 *
 * vtkBoundedVolumeSource generate an image data given the position and
 * scale factors for a unit volume.
*/

#ifndef vtkBoundedVolumeSource_h
#define vtkBoundedVolumeSource_h

#include "vtkImageAlgorithm.h"
#include "vtkPVVTKExtensionsPointsModule.h" // for export macro
#include "vtkVector.h"                      // for vtkVector

class vtkBoundingBox;

class VTKPVVTKEXTENSIONSPOINTS_EXPORT vtkBoundedVolumeSource : public vtkImageAlgorithm
{
public:
  static vtkBoundedVolumeSource* New();
  vtkTypeMacro(vtkBoundedVolumeSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the origin or translation for the unit volume.
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVector3Macro(Origin, double);
  //@}

  //@{
  /**
   * Get/Set the scale factor for a unit volume. Note that scaling is applied
   * before the translation.
   */
  vtkSetVector3Macro(Scale, double);
  vtkGetVector3Macro(Scale, double);
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
   * Get/Set the output image resolution. Used only when RefinementMode is set to
   * USE_RESOLUTION.
   */
  vtkSetVector3Macro(Resolution, int);
  vtkGetVector3Macro(Resolution, int);
  //@}

  //@{
  /**
   * Specify the cell-size of the output image. Used only when RefinementMode is set to
   * USE_CELL_SIZE.
   */
  vtkSetMacro(CellSize, double);
  vtkGetMacro(CellSize, double);
  //@}

  //@{
  /**
   * Specify the padding to use along each of the directions. This is used to
   * inflate the bounds by a fixed factor in all directions.
   */
  vtkSetClampMacro(Padding, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Padding, double);
  //@}

  //@{
  /**
   * Convenience methods that setup a image extents, origin and spacing given
   * the bounding box, and either the target image resolution or unit cell size.
   */
  static bool SetImageParameters(
    vtkImageData* image, const vtkBoundingBox& bbox, const vtkVector3i& resolution);
  static bool SetImageParameters(
    vtkImageData* image, const vtkBoundingBox& bbox, const double cellSize);
  //@}

protected:
  vtkBoundedVolumeSource();
  ~vtkBoundedVolumeSource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  void ExecuteDataWithInformation(vtkDataObject* data, vtkInformation* outInfo) override;

  double Origin[3];
  double Scale[3];
  int RefinementMode;
  int Resolution[3];
  double CellSize;
  double Padding;

private:
  vtkBoundedVolumeSource(const vtkBoundedVolumeSource&) = delete;
  void operator=(const vtkBoundedVolumeSource&) = delete;
};

#endif
