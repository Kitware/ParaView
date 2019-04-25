/*=========================================================================

  Program:   ParaView
  Module:    vtkNetworkImageData.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNetworkImageData
 * @brief   an image source that can take an image data on
 * one process and ensure that it's available on some other group of processes.
 *
 * @todo : make vtkNetworkImageSource a subclass of vtkNetworkImageData
 * and better naming (vtkNetworkImageSourceReader ?)
*/

#ifndef vtkNetworkImageData_h
#define vtkNetworkImageData_h

#include "vtkImageAlgorithm.h"
#include "vtkPVVTKExtensionsFiltersRenderingModule.h" //needed for exports
#include "vtkSetGet.h"

class vtkImageData;
class vtkClientServerStream;

class VTKPVVTKEXTENSIONSFILTERSRENDERING_EXPORT vtkNetworkImageData : public vtkImageAlgorithm
{
public:
  static vtkNetworkImageData* New();
  vtkTypeMacro(vtkNetworkImageData, vtkImageAlgorithm);

  /**
   * Needs to be called to perform the actual buffer migration.
   */
  void UpdateBuffer();

protected:
  vtkNetworkImageData() = default;
  ~vtkNetworkImageData() override;

  vtkTimeStamp UpdateImageTime;
  vtkImageData* Buffer = nullptr;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkNetworkImageData(const vtkNetworkImageData&) = delete;
  void operator=(const vtkNetworkImageData&) = delete;
};

#endif
