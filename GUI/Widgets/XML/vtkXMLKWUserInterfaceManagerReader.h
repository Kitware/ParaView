/*=========================================================================

  Module:    vtkXMLKWUserInterfaceManagerReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWUserInterfaceManagerReader - vtkKWUserInterfaceManager XML Reader.
// .SECTION Description
// vtkXMLKWUserInterfaceManagerReader provides XML reading functionality to 
// vtkKWUserInterfaceManager.
// .SECTION See Also
// vtkXMLKWUserInterfaceManagerWriter

#ifndef __vtkXMLKWUserInterfaceManagerReader_h
#define __vtkXMLKWUserInterfaceManagerReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLKWUserInterfaceManagerReader : public vtkXMLObjectReader
{
public:
  static vtkXMLKWUserInterfaceManagerReader* New();
  vtkTypeRevisionMacro(vtkXMLKWUserInterfaceManagerReader, vtkXMLObjectReader);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLKWUserInterfaceManagerReader() {};
  ~vtkXMLKWUserInterfaceManagerReader() {};

private:
  vtkXMLKWUserInterfaceManagerReader(const vtkXMLKWUserInterfaceManagerReader&); // Not implemented
  void operator=(const vtkXMLKWUserInterfaceManagerReader&); // Not implemented    
};

#endif

