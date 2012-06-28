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
// .NAME vtkNetworkImageSource - an image source that can read an image file on
// one process and ensure that it's available on some other group of processes.
// .SECTION Description
// vtkNetworkImageSource is a vtkImageAlgorithm that can read an image file on
// on the client process and produce the output image data on a client and
// render-server processes.

#ifndef __vtkNetworkImageSource_h
#define __vtkNetworkImageSource_h

#include "vtkImageAlgorithm.h"

class vtkImageData;
class vtkClientServerStream;

class VTK_EXPORT vtkNetworkImageSource : public vtkImageAlgorithm
{
public:
  static vtkNetworkImageSource* New();
  vtkTypeMacro(vtkNetworkImageSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the filename.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Needs to be called to perform the actual image migration.
  void UpdateImage();

protected:
  vtkNetworkImageSource();
  ~vtkNetworkImageSource();

  vtkTimeStamp UpdateImageTime;

  char* FileName;

  vtkImageData* Buffer;
  int ReadImageFromFile(const char* filename);
  int RequestData(vtkInformation *request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);
  int RequestInformation(vtkInformation *request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector);

private:
  vtkNetworkImageSource(const vtkNetworkImageSource&); // Not implemented.
  void operator=(const vtkNetworkImageSource&); // Not implemented.
};

#endif
