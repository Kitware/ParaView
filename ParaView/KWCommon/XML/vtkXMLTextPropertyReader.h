/*=========================================================================

  Module:    vtkXMLTextPropertyReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLTextPropertyReader - vtkTextProperty XML Reader.
// .SECTION Description
// vtkXMLTextPropertyReader provides XML reading functionality to 
// vtkTextProperty.
// .SECTION See Also
// vtkXMLTextPropertyWriter

#ifndef __vtkXMLTextPropertyReader_h
#define __vtkXMLTextPropertyReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLTextPropertyReader : public vtkXMLObjectReader
{
public:
  static vtkXMLTextPropertyReader* New();
  vtkTypeRevisionMacro(vtkXMLTextPropertyReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLTextPropertyReader() {};
  ~vtkXMLTextPropertyReader() {};

private:
  vtkXMLTextPropertyReader(const vtkXMLTextPropertyReader&); // Not implemented
  void operator=(const vtkXMLTextPropertyReader&); // Not implemented    
};

#endif


