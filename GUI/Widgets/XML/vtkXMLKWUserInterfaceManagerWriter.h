/*=========================================================================

  Module:    vtkXMLKWUserInterfaceManagerWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWUserInterfaceManagerWriter - vtkKWUserInterfaceManager XML Writer.
// .SECTION Description
// vtkXMLKWUserInterfaceManagerWriter provides XML writing functionality to 
// vtkKWUserInterfaceManager.
// .SECTION See Also
// vtkXMLKWUserInterfaceManagerReader

#ifndef __vtkXMLKWUserInterfaceManagerWriter_h
#define __vtkXMLKWUserInterfaceManagerWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLKWUserInterfaceManagerWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLKWUserInterfaceManagerWriter* New();
  vtkTypeRevisionMacro(vtkXMLKWUserInterfaceManagerWriter,vtkXMLObjectWriter);

  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLKWUserInterfaceManagerWriter() {};
  ~vtkXMLKWUserInterfaceManagerWriter() {};  

private:
  vtkXMLKWUserInterfaceManagerWriter(const vtkXMLKWUserInterfaceManagerWriter&);  // Not implemented.
  void operator=(const vtkXMLKWUserInterfaceManagerWriter&);  // Not implemented.
};

#endif

