/*=========================================================================

  Module:    vtkXMLCornerAnnotationWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLCornerAnnotationWriter - vtkCornerAnnotation XML Writer.
// .SECTION Description
// vtkXMLCornerAnnotationWriter provides XML writing functionality to 
// vtkCornerAnnotation.
// .SECTION See Also
// vtkXMLCornerAnnotationReader

#ifndef __vtkXMLCornerAnnotationWriter_h
#define __vtkXMLCornerAnnotationWriter_h

#include "vtkXMLActor2DWriter.h"

class VTK_EXPORT vtkXMLCornerAnnotationWriter : public vtkXMLActor2DWriter
{
public:
  static vtkXMLCornerAnnotationWriter* New();
  vtkTypeRevisionMacro(vtkXMLCornerAnnotationWriter,vtkXMLActor2DWriter);

  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the text property.
  static char* GetTextPropertyElementName();

protected:
  vtkXMLCornerAnnotationWriter() {};
  ~vtkXMLCornerAnnotationWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLCornerAnnotationWriter(const vtkXMLCornerAnnotationWriter&);  // Not implemented.
  void operator=(const vtkXMLCornerAnnotationWriter&);  // Not implemented.
};

#endif

