/*=========================================================================

  Module:    vtkXMLProperty2DWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLProperty2DWriter - vtkProperty2D XML Writer.
// .SECTION Description
// vtkXMLProperty2DWriter provides XML writing functionality to 
// vtkProperty2D.
// .SECTION See Also
// vtkXMLProperty2DReader

#ifndef __vtkXMLProperty2DWriter_h
#define __vtkXMLProperty2DWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLProperty2DWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLProperty2DWriter* New();
  vtkTypeRevisionMacro(vtkXMLProperty2DWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLProperty2DWriter() {};
  ~vtkXMLProperty2DWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLProperty2DWriter(const vtkXMLProperty2DWriter&);  // Not implemented.
  void operator=(const vtkXMLProperty2DWriter&);  // Not implemented.
};

#endif


