/*=========================================================================

  Module:    vtkXMLPropWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPropWriter - vtkProp XML Writer.
// .SECTION Description
// vtkXMLPropWriter provides XML writing functionality to 
// vtkProp.
// .SECTION See Also
// vtkXMLPropReader

#ifndef __vtkXMLPropWriter_h
#define __vtkXMLPropWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLPropWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLPropWriter* New();
  vtkTypeRevisionMacro(vtkXMLPropWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLPropWriter() {};
  ~vtkXMLPropWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLPropWriter(const vtkXMLPropWriter&);  // Not implemented.
  void operator=(const vtkXMLPropWriter&);  // Not implemented.
};

#endif


