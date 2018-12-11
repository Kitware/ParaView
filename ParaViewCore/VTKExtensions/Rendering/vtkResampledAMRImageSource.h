/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkResampledAMRImageSource
 * @brief   image data source that resamples an AMR
 * dataset to produce the image data.
 *
 *
 * vtkResampledAMRImageSource is a image data source that resamples a
 * vtkOverlappingAMR dataset to produce an image data. The output AMR will have
 * both the point data and cell data from the input AMR grids passed along as point
 * data for the output image data. This filter assumes that all blocks in the
 * input AMR have exactly the same point/cell arrays in same order. If they are
 * different we will end up with weird runtime issues that may be hard to debug.
 *
 * @attention
 * We subclass vtkTrivialProducer since it deals with all the meta-data that
 * needs to be passed down the pipeline for image data, keeping the code here
 * simple.
*/

#ifndef vtkResampledAMRImageSource_h
#define vtkResampledAMRImageSource_h

#include "vtkPVVTKExtensionsRenderingModule.h" // needed for export macro
#include "vtkSmartPointer.h"                   // needed for vtkSmartPointer
#include "vtkTrivialProducer.h"

class vtkAMRBox;
class vtkImageData;
class vtkIntArray;
class vtkOverlappingAMR;
class vtkPointData;

class VTKPVVTKEXTENSIONSRENDERING_EXPORT vtkResampledAMRImageSource : public vtkTrivialProducer
{
public:
  static vtkResampledAMRImageSource* New();
  vtkTypeMacro(vtkResampledAMRImageSource, vtkTrivialProducer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the maximum number of samples along each axis.
   */
  vtkSetVector3Macro(MaxDimensions, int);
  vtkGetVector3Macro(MaxDimensions, int);
  //@}

  //@{
  /**
   * When provided, the resampled image is set up to cover these bounds. If not
   * provided, data bounds are used. If provided, these bounds *MUST* fit
   * within the data bounds. This is essential to ensure valid resampled volume
   * is generated.
   */
  vtkSetVector6Macro(SpatialBounds, double);
  vtkGetVector6Macro(SpatialBounds, double);
  //@}

  /**
   * To restart the incremental resample process, call this method. The output
   * image data is setup in the first call to Update().
   */
  void Reset();

  /**
   * Updates the volume. Any non-empty pieces provided by the amr are added to
   * the resampled volume if it adds refinement to the volume.
   */
  void UpdateResampledVolume(vtkOverlappingAMR*);

  /**
   * Returns true if the resampler will reinitialize the volume in the next call
   * to UpdateResampledVolume().
   */
  bool NeedsInitialization() const { return (this->MTime > this->InitializationTime); }

protected:
  vtkResampledAMRImageSource();
  ~vtkResampledAMRImageSource() override;

  bool Initialize(vtkOverlappingAMR* amr);
  bool UpdateResampledVolume(
    const unsigned int& level, const unsigned& index, const vtkAMRBox& box, vtkImageData* data);

  int MaxDimensions[3];
  double SpatialBounds[6];

  vtkSmartPointer<vtkImageData> ResampledAMR;
  vtkSmartPointer<vtkPointData> ResampledAMRPointData;

  vtkSmartPointer<vtkIntArray> DonorLevel;

private:
  vtkResampledAMRImageSource(const vtkResampledAMRImageSource&) = delete;
  void operator=(const vtkResampledAMRImageSource&) = delete;

  vtkTimeStamp InitializationTime;
};

#endif
