/*=========================================================================

  Module:    vtkXMLVolumePropertyReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLVolumePropertyReader - vtkVolumeProperty XML Reader.
// .SECTION Description
// vtkXMLVolumePropertyReader provides XML reading functionality to 
// vtkVolumeProperty.
// .SECTION See Also
// vtkXMLVolumePropertyWriter

#ifndef __vtkXMLVolumePropertyReader_h
#define __vtkXMLVolumePropertyReader_h

#include "vtkXMLObjectReader.h"

class vtkImageData;

class VTK_EXPORT vtkXMLVolumePropertyReader : public vtkXMLObjectReader
{
public:
  static vtkXMLVolumePropertyReader* New();
  vtkTypeRevisionMacro(vtkXMLVolumePropertyReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

  // Description:
  // Set the image data the volume property that is going to be read will
  // be used on. This will be used for CheckScalarOpacityUnitDistance.
  virtual void SetImageData(vtkImageData *data);
  vtkGetObjectMacro(ImageData, vtkImageData);

  // Description:
  // Check if the scalar opacity unit distance attribute fits within
  // the data spacing (see SetImageData). If not, ignore it.
  vtkSetMacro(CheckScalarOpacityUnitDistance, int);
  vtkGetMacro(CheckScalarOpacityUnitDistance, int);
  vtkBooleanMacro(CheckScalarOpacityUnitDistance, int);

  // Description:
  // Keep points range. The transfer function points will be adjusted so that
  // the transfer function range remain the same. Points out of the
  // range will be discarded.
  vtkSetMacro(KeepTransferFunctionPointsRange, int);
  vtkGetMacro(KeepTransferFunctionPointsRange, int);
  vtkBooleanMacro(KeepTransferFunctionPointsRange, int);

protected:  
  vtkXMLVolumePropertyReader();
  ~vtkXMLVolumePropertyReader();

  vtkImageData *ImageData;

  int CheckScalarOpacityUnitDistance;
  int KeepTransferFunctionPointsRange;

private:
  vtkXMLVolumePropertyReader(const vtkXMLVolumePropertyReader&); // Not implemented
  void operator=(const vtkXMLVolumePropertyReader&); // Not implemented    
};

#endif


