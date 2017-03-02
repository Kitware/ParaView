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

  //@{
  /**
   * Get/Set the output image resolution.
   */
  vtkSetVector3Macro(Resolution, int);
  vtkGetVector3Macro(Resolution, int);
  //@}

protected:
  vtkBoundedVolumeSource();
  ~vtkBoundedVolumeSource();

  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  virtual void ExecuteDataWithInformation(
    vtkDataObject* data, vtkInformation* outInfo) VTK_OVERRIDE;

  double Origin[3];
  double Scale[3];
  int Resolution[3];

private:
  vtkBoundedVolumeSource(const vtkBoundedVolumeSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkBoundedVolumeSource&) VTK_DELETE_FUNCTION;
};

#endif
