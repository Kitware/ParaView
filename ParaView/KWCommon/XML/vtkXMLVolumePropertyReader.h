/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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


