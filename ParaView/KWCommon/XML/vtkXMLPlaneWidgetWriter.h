/*=========================================================================

  Module:    vtkXMLPlaneWidgetWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPlaneWidgetWriter - vtkPlaneWidget XML Writer.
// .SECTION Description
// vtkXMLPlaneWidgetWriter provides XML writing functionality to 
// vtkPlaneWidget.
// .SECTION See Also
// vtkXMLPlaneWidgetReader

#ifndef __vtkXMLPlaneWidgetWriter_h
#define __vtkXMLPlaneWidgetWriter_h

#include "vtkXMLPolyDataSourceWidgetWriter.h"

class VTK_EXPORT vtkXMLPlaneWidgetWriter : public vtkXMLPolyDataSourceWidgetWriter
{
public:
  static vtkXMLPlaneWidgetWriter* New();
  vtkTypeRevisionMacro(vtkXMLPlaneWidgetWriter,vtkXMLPolyDataSourceWidgetWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the properties.
  static char* GetHandlePropertyElementName();
  static char* GetSelectedHandlePropertyElementName();
  static char* GetPlanePropertyElementName();
  static char* GetSelectedPlanePropertyElementName();

protected:
  vtkXMLPlaneWidgetWriter() {};
  ~vtkXMLPlaneWidgetWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLPlaneWidgetWriter(const vtkXMLPlaneWidgetWriter&);  // Not implemented.
  void operator=(const vtkXMLPlaneWidgetWriter&);  // Not implemented.
};

#endif


