/*=========================================================================

  Module:    vtkXMLTextActorWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLTextActorWriter - vtkTextActor XML Writer.
// .SECTION Description
// vtkXMLTextActorWriter provides XML writing functionality to 
// vtkTextActor.
// .SECTION See Also
// vtkXMLTextActorReader

#ifndef __vtkXMLTextActorWriter_h
#define __vtkXMLTextActorWriter_h

#include "vtkXMLActor2DWriter.h"

class VTK_EXPORT vtkXMLTextActorWriter : public vtkXMLActor2DWriter
{
public:
  static vtkXMLTextActorWriter* New();
  vtkTypeRevisionMacro(vtkXMLTextActorWriter,vtkXMLActor2DWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the text property.
  static char* GetTextPropertyElementName();

protected:
  vtkXMLTextActorWriter() {};
  ~vtkXMLTextActorWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLTextActorWriter(const vtkXMLTextActorWriter&);  // Not implemented.
  void operator=(const vtkXMLTextActorWriter&);  // Not implemented.
};

#endif


