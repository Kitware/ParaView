/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSimpleSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageSimpleSource
 * @brief   Create an image with simple-to-compute pixel values.
 *
 * vtkImageSimpleSource produces images with pixel values that can
 * be obtained with relatively low computational load.
*/

#ifndef vtkImageSimpleSource_h
#define vtkImageSimpleSource_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkThreadedImageAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkImageSimpleSource : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageSimpleSource* New();
  vtkTypeMacro(vtkImageSimpleSource, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax, int zMin, int zMax);

protected:
  vtkImageSimpleSource();
  ~vtkImageSimpleSource() override {}

  int WholeExtent[6];
  int FirstIndex[3];

  void PrepareImageData(vtkInformationVector** inputVector, vtkInformationVector* outputVector,
    vtkImageData*** inDataObjects = nullptr, vtkImageData** outDataObjects = nullptr) override;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int extent[6], int threadId) override;

private:
  vtkImageSimpleSource(const vtkImageSimpleSource&) = delete;
  void operator=(const vtkImageSimpleSource&) = delete;
};

#endif
