/*=========================================================================

  Module:    vtkXMLActorWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLActorWriter - vtkActor XML Writer.
// .SECTION Description
// vtkXMLActorWriter provides XML writing functionality to 
// vtkActor.
// .SECTION See Also
// vtkXMLActorReader

#ifndef __vtkXMLActorWriter_h
#define __vtkXMLActorWriter_h

#include "vtkXMLProp3DWriter.h"

class VTK_EXPORT vtkXMLActorWriter : public vtkXMLProp3DWriter
{
public:
  static vtkXMLActorWriter* New();
  vtkTypeRevisionMacro(vtkXMLActorWriter,vtkXMLProp3DWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the property element used inside that tree to
  // store properties.
  static char* GetPropertyElementName();
  static char* GetBackfacePropertyElementName();

protected:
  vtkXMLActorWriter() {};
  ~vtkXMLActorWriter() {};  
  
  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLActorWriter(const vtkXMLActorWriter&);  // Not implemented.
  void operator=(const vtkXMLActorWriter&);  // Not implemented.
};

#endif


