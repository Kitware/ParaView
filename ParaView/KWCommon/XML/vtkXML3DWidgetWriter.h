/*=========================================================================

  Module:    vtkXML3DWidgetWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXML3DWidgetWriter - vtk3DWidget XML Writer.
// .SECTION Description
// vtkXML3DWidgetWriter provides XML writing functionality to 
// vtk3DWidget.
// .SECTION See Also
// vtkXML3DWidgetReader

#ifndef __vtkXML3DWidgetWriter_h
#define __vtkXML3DWidgetWriter_h

#include "vtkXMLInteractorObserverWriter.h"

class VTK_EXPORT vtkXML3DWidgetWriter : public vtkXMLInteractorObserverWriter
{
public:
  static vtkXML3DWidgetWriter* New();
  vtkTypeRevisionMacro(vtkXML3DWidgetWriter,vtkXMLInteractorObserverWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXML3DWidgetWriter() {};
  ~vtkXML3DWidgetWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXML3DWidgetWriter(const vtkXML3DWidgetWriter&);  // Not implemented.
  void operator=(const vtkXML3DWidgetWriter&);  // Not implemented.
};

#endif



