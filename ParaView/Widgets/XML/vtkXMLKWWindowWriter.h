/*=========================================================================

  Module:    vtkXMLKWWindowWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWWindowWriter - vtkKWWindow XML Writer.
// .SECTION Description
// vtkXMLKWWindowWriter provides XML writing functionality to 
// vtkKWWindow.
// .SECTION See Also
// vtkXMLKWWindowReader

#ifndef __vtkXMLKWWindowWriter_h
#define __vtkXMLKWWindowWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLKWWindowWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLKWWindowWriter* New();
  vtkTypeRevisionMacro(vtkXMLKWWindowWriter,vtkXMLObjectWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the user interface settings.
  static char* GetUserInterfaceElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the interface manager.
  static char* GetUserInterfaceManagerElementName();

  // Description:
  // Output UI elements only
  vtkBooleanMacro(OutputUserInterfaceElementOnly, int);
  vtkGetMacro(OutputUserInterfaceElementOnly, int);
  vtkSetMacro(OutputUserInterfaceElementOnly, int);

protected:
  vtkXMLKWWindowWriter();
  ~vtkXMLKWWindowWriter() {};  
  
  int OutputUserInterfaceElementOnly;

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

  // Description:
  // Create  the user-interface internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int CreateUserInterfaceElement(vtkXMLDataElement *ui_elem);

private:
  vtkXMLKWWindowWriter(const vtkXMLKWWindowWriter&);  // Not implemented.
  void operator=(const vtkXMLKWWindowWriter&);  // Not implemented.
};

#endif

