/*=========================================================================

  Module:    vtkXMLActor2DWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLActor2DWriter - vtkActor2D XML Writer.
// .SECTION Description
// vtkXMLActor2DWriter provides XML writing functionality to 
// vtkActor2D.
// .SECTION See Also
// vtkXMLActor2DReader

#ifndef __vtkXMLActor2DWriter_h
#define __vtkXMLActor2DWriter_h

#include "vtkXMLPropWriter.h"

class VTK_EXPORT vtkXMLActor2DWriter : public vtkXMLPropWriter
{
public:
  static vtkXMLActor2DWriter* New();
  vtkTypeRevisionMacro(vtkXMLActor2DWriter,vtkXMLPropWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the property element used inside that tree to
  // store a property.
  static char* GetPropertyElementName();

protected:
  vtkXMLActor2DWriter() {};
  ~vtkXMLActor2DWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLActor2DWriter(const vtkXMLActor2DWriter&);  // Not implemented.
  void operator=(const vtkXMLActor2DWriter&);  // Not implemented.
};

#endif



