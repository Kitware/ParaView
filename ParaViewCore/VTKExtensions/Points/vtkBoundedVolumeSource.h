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

class VTKPVVTKEXTENSIONSPOINTS_EXPORT vtkBoundedVolumeSource : public vtkImageAlgorithm
{
public:
  static vtkBoundedVolumeSource* New();
  vtkTypeMacro(vtkBoundedVolumeSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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

protected:
  vtkBoundedVolumeSource();
  ~vtkBoundedVolumeSource() override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  void ExecuteDataWithInformation(vtkDataObject* data, vtkInformation* outInfo) VTK_OVERRIDE;

  double Origin[3];
  double Scale[3];
  int RefinementMode;
  int Resolution[3];
  double CellSize;

private:
  vtkBoundedVolumeSource(const vtkBoundedVolumeSource&) = delete;
  void operator=(const vtkBoundedVolumeSource&) = delete;
};

#endif
