/*=========================================================================

  Module:    vtkXMLImagePlaneWidgetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLImagePlaneWidgetReader - vtkImagePlaneWidget XML Reader.
// .SECTION Description
// vtkXMLImagePlaneWidgetReader provides XML reading functionality to 
// vtkImagePlaneWidget.
// .SECTION See Also
// vtkXMLImagePlaneWidgetWriter

#ifndef __vtkXMLImagePlaneWidgetReader_h
#define __vtkXMLImagePlaneWidgetReader_h

#include "vtkXMLPolyDataSourceWidgetReader.h"

class VTK_EXPORT vtkXMLImagePlaneWidgetReader : public vtkXMLPolyDataSourceWidgetReader
{
public:
  static vtkXMLImagePlaneWidgetReader* New();
  vtkTypeRevisionMacro(vtkXMLImagePlaneWidgetReader, vtkXMLPolyDataSourceWidgetReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLImagePlaneWidgetReader() {};
  ~vtkXMLImagePlaneWidgetReader() {};

private:
  vtkXMLImagePlaneWidgetReader(const vtkXMLImagePlaneWidgetReader&); // Not implemented
  void operator=(const vtkXMLImagePlaneWidgetReader&); // Not implemented    
};

#endif


