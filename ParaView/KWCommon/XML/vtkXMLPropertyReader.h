/*=========================================================================

  Module:    vtkXMLPropertyReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPropertyReader - vtkProperty XML Reader.
// .SECTION Description
// vtkXMLPropertyReader provides XML reading functionality to 
// vtkProperty.
// .SECTION See Also
// vtkXMLPropertyWriter

#ifndef __vtkXMLPropertyReader_h
#define __vtkXMLPropertyReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLPropertyReader : public vtkXMLObjectReader
{
public:
  static vtkXMLPropertyReader* New();
  vtkTypeRevisionMacro(vtkXMLPropertyReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLPropertyReader() {};
  ~vtkXMLPropertyReader() {};

private:
  vtkXMLPropertyReader(const vtkXMLPropertyReader&); // Not implemented
  void operator=(const vtkXMLPropertyReader&); // Not implemented    
};

#endif


