/*=========================================================================

  Module:    vtkXMLKWRenderWidgetWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWRenderWidgetWriter - vtkKWRenderWidget XML Writer.
// .SECTION Description
// vtkXMLKWRenderWidgetWriter provides XML writing functionality to 
// vtkKWRenderWidget.
// .SECTION See Also
// vtkXMLKWRenderWidgetReader

#ifndef __vtkXMLKWRenderWidgetWriter_h
#define __vtkXMLKWRenderWidgetWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLKWRenderWidgetWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLKWRenderWidgetWriter* New();
  vtkTypeRevisionMacro(vtkXMLKWRenderWidgetWriter,vtkXMLObjectWriter);

  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the camera element used inside that tree to
  // store the current camera parameters.
  static char* GetCurrentCameraElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the corner annotation parameters.
  static char* GetCornerAnnotationElementName();

  // Description:
  // Return the name of the header annotation element used inside that tree to
  // store the data.
  static char* GetHeaderAnnotationElementName();

protected:
  vtkXMLKWRenderWidgetWriter() {};
  ~vtkXMLKWRenderWidgetWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLKWRenderWidgetWriter(const vtkXMLKWRenderWidgetWriter&);  // Not implemented.
  void operator=(const vtkXMLKWRenderWidgetWriter&);  // Not implemented.
};

#endif

