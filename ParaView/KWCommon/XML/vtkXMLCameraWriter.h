/*=========================================================================

  Module:    vtkXMLCameraWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLCameraWriter - vtkCamera XML Writer.
// .SECTION Description
// vtkXMLCameraWriter provides XML writing functionality to 
// vtkCamera.
// .SECTION See Also
// vtkXMLCameraReader

#ifndef __vtkXMLCameraWriter_h
#define __vtkXMLCameraWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLCameraWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLCameraWriter* New();
  vtkTypeRevisionMacro(vtkXMLCameraWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLCameraWriter() {};
  ~vtkXMLCameraWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLCameraWriter(const vtkXMLCameraWriter&);  // Not implemented.
  void operator=(const vtkXMLCameraWriter&);  // Not implemented.
};

#endif


