/*=========================================================================

  Program:   ParaView
  Module:    vtkNetworkImageSource.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNetworkImageSource
 * @brief   an image source that can read an image file on
 * one process and ensure that it's available on some other group of processes.
 *
 * vtkNetworkImageSource is a vtkImageAlgorithm that can read an image file on
 * on the client process and produce the output image data on a client and
 * render-server processes.
*/

#ifndef vtkNetworkImageSource_h
#define vtkNetworkImageSource_h

#include "vtkImageAlgorithm.h"
#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports

class vtkImageData;
class vtkClientServerStream;

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkNetworkImageSource : public vtkImageAlgorithm
{
public:
  static vtkNetworkImageSource* New();
  vtkTypeMacro(vtkNetworkImageSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the filename.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Needs to be called to perform the actual image migration.
   */
  void UpdateImage();

protected:
  vtkNetworkImageSource();
  ~vtkNetworkImageSource();

  vtkTimeStamp UpdateImageTime;

  char* FileName;

  vtkImageData* Buffer;
  int ReadImageFromFile(const char* filename);
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

private:
  vtkNetworkImageSource(const vtkNetworkImageSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkNetworkImageSource&) VTK_DELETE_FUNCTION;
};

#endif
