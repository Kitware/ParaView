/*=========================================================================

  Module:    vtkXMLLightReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLLightReader - vtkLight XML Reader.
// .SECTION Description
// vtkXMLLightReader provides XML reading functionality to 
// vtkLight.
// .SECTION See Also
// vtkXMLLightWriter

#ifndef __vtkXMLLightReader_h
#define __vtkXMLLightReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLLightReader : public vtkXMLObjectReader
{
public:
  static vtkXMLLightReader* New();
  vtkTypeRevisionMacro(vtkXMLLightReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLLightReader() {};
  ~vtkXMLLightReader() {};

private:
  vtkXMLLightReader(const vtkXMLLightReader&); // Not implemented
  void operator=(const vtkXMLLightReader&); // Not implemented    
};

#endif


