/*=========================================================================

  Module:    vtkXMLKWWindowReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWWindowReader - vtkKWWindow XML Reader.
// .SECTION Description
// vtkXMLKWWindowReader provides XML reading functionality to 
// vtkKWWindow.
// .SECTION See Also
// vtkXMLKWWindowWriter

#ifndef __vtkXMLKWWindowReader_h
#define __vtkXMLKWWindowReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLKWWindowReader : public vtkXMLObjectReader
{
public:
  static vtkXMLKWWindowReader* New();
  vtkTypeRevisionMacro(vtkXMLKWWindowReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLKWWindowReader() {};
  ~vtkXMLKWWindowReader() {};

  // Description:
  // Parse the user-interface element part.
  // Return 1 on success, 0 otherwise.
  virtual int ParseUserInterfaceElement(vtkXMLDataElement *ui_elem);

private:
  vtkXMLKWWindowReader(const vtkXMLKWWindowReader&); // Not implemented
  void operator=(const vtkXMLKWWindowReader&); // Not implemented    
};

#endif

