/*=========================================================================

  Module:    vtkXMLTextPropertyWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLTextPropertyWriter - vtkTextProperty XML Writer.
// .SECTION Description
// vtkXMLTextPropertyWriter provides XML writing functionality to 
// vtkTextProperty.
// .SECTION See Also
// vtkXMLTextPropertyReader

#ifndef __vtkXMLTextPropertyWriter_h
#define __vtkXMLTextPropertyWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLTextPropertyWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLTextPropertyWriter* New();
  vtkTypeRevisionMacro(vtkXMLTextPropertyWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLTextPropertyWriter() {};
  ~vtkXMLTextPropertyWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLTextPropertyWriter(const vtkXMLTextPropertyWriter&);  // Not implemented.
  void operator=(const vtkXMLTextPropertyWriter&);  // Not implemented.
};

#endif


