/*=========================================================================

  Module:    vtkXMLCameraReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLCameraReader - vtkCamera XML Reader.
// .SECTION Description
// vtkXMLCameraReader provides XML reading functionality to 
// vtkCamera.
// .SECTION See Also
// vtkXMLCameraWriter

#ifndef __vtkXMLCameraReader_h
#define __vtkXMLCameraReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLCameraReader : public vtkXMLObjectReader
{
public:
  static vtkXMLCameraReader* New();
  vtkTypeRevisionMacro(vtkXMLCameraReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLCameraReader() {};
  ~vtkXMLCameraReader() {};

private:
  vtkXMLCameraReader(const vtkXMLCameraReader&); // Not implemented
  void operator=(const vtkXMLCameraReader&); // Not implemented    
};

#endif


