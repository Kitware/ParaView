/*=========================================================================

  Module:    vtkXMLImagePlaneWidgetWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLImagePlaneWidgetWriter - vtkImagePlaneWidget XML Writer.
// .SECTION Description
// vtkXMLImagePlaneWidgetWriter provides XML writing functionality to 
// vtkImagePlaneWidget.
// .SECTION See Also
// vtkXMLImagePlaneWidgetReader

#ifndef __vtkXMLImagePlaneWidgetWriter_h
#define __vtkXMLImagePlaneWidgetWriter_h

#include "vtkXMLPolyDataSourceWidgetWriter.h"

class VTK_EXPORT vtkXMLImagePlaneWidgetWriter : public vtkXMLPolyDataSourceWidgetWriter
{
public:
  static vtkXMLImagePlaneWidgetWriter* New();
  vtkTypeRevisionMacro(vtkXMLImagePlaneWidgetWriter,vtkXMLPolyDataSourceWidgetWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the element used inside that tree to
  // store the properties.
  static char* GetPlanePropertyElementName();
  static char* GetSelectedPlanePropertyElementName();
  static char* GetCursorPropertyElementName();
  static char* GetMarginPropertyElementName();
  static char* GetTexturePlanePropertyElementName();
  static char* GetTextPropertyElementName();

protected:
  vtkXMLImagePlaneWidgetWriter() {};
  ~vtkXMLImagePlaneWidgetWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLImagePlaneWidgetWriter(const vtkXMLImagePlaneWidgetWriter&);  
  // Not implemented.
  void operator=(const vtkXMLImagePlaneWidgetWriter&);  
  // Not implemented.
};

#endif


