/*=========================================================================

  Module:    vtkXMLProp3DWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLProp3DWriter - vtkProp3D XML Writer.
// .SECTION Description
// vtkXMLProp3DWriter provides XML writing functionality to 
// vtkProp3D.
// .SECTION See Also
// vtkXMLProp3DReader

#ifndef __vtkXMLProp3DWriter_h
#define __vtkXMLProp3DWriter_h

#include "vtkXMLPropWriter.h"

class VTK_EXPORT vtkXMLProp3DWriter : public vtkXMLPropWriter
{
public:
  static vtkXMLProp3DWriter* New();
  vtkTypeRevisionMacro(vtkXMLProp3DWriter,vtkXMLPropWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLProp3DWriter() {};
  ~vtkXMLProp3DWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLProp3DWriter(const vtkXMLProp3DWriter&);  // Not implemented.
  void operator=(const vtkXMLProp3DWriter&);  // Not implemented.
};

#endif


