/*=========================================================================

  Module:    vtkXMLScalarBarActorWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLScalarBarActorWriter - vtkScalarBarActor XML Writer.
// .SECTION Description
// vtkXMLScalarBarActorWriter provides XML writing functionality to 
// vtkScalarBarActor.
// .SECTION See Also
// vtkXMLScalarBarActorReader

#ifndef __vtkXMLScalarBarActorWriter_h
#define __vtkXMLScalarBarActorWriter_h

#include "vtkXMLActor2DWriter.h"

class VTK_EXPORT vtkXMLScalarBarActorWriter : public vtkXMLActor2DWriter
{
public:
  static vtkXMLScalarBarActorWriter* New();
  vtkTypeRevisionMacro(vtkXMLScalarBarActorWriter,vtkXMLActor2DWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the properties.
  static char* GetTitleTextPropertyElementName();
  static char* GetLabelTextPropertyElementName();

protected:
  vtkXMLScalarBarActorWriter() {};
  ~vtkXMLScalarBarActorWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLScalarBarActorWriter(const vtkXMLScalarBarActorWriter&);  // Not implemented.
  void operator=(const vtkXMLScalarBarActorWriter&);  // Not implemented.
};

#endif


