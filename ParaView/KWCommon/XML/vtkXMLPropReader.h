/*=========================================================================

  Module:    vtkXMLPropReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPropReader - vtkProp XML Reader.
// .SECTION Description
// vtkXMLPropReader provides XML reading functionality to 
// vtkProp.
// .SECTION See Also
// vtkXMLPropWriter

#ifndef __vtkXMLPropReader_h
#define __vtkXMLPropReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLPropReader : public vtkXMLObjectReader
{
public:
  static vtkXMLPropReader* New();
  vtkTypeRevisionMacro(vtkXMLPropReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLPropReader() {};
  ~vtkXMLPropReader() {};

private:
  vtkXMLPropReader(const vtkXMLPropReader&); // Not implemented
  void operator=(const vtkXMLPropReader&); // Not implemented    
};

#endif


