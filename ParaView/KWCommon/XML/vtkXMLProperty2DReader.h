/*=========================================================================

  Module:    vtkXMLProperty2DReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLProperty2DReader - vtkProperty2D XML Reader.
// .SECTION Description
// vtkXMLProperty2DReader provides XML reading functionality to 
// vtkProperty2D.
// .SECTION See Also
// vtkXMLProperty2DWriter

#ifndef __vtkXMLProperty2DReader_h
#define __vtkXMLProperty2DReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLProperty2DReader : public vtkXMLObjectReader
{
public:
  static vtkXMLProperty2DReader* New();
  vtkTypeRevisionMacro(vtkXMLProperty2DReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLProperty2DReader() {};
  ~vtkXMLProperty2DReader() {};

private:
  vtkXMLProperty2DReader(const vtkXMLProperty2DReader&); // Not implemented
  void operator=(const vtkXMLProperty2DReader&); // Not implemented    
};

#endif


