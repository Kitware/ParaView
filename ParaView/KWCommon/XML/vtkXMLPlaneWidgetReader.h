/*=========================================================================

  Module:    vtkXMLPlaneWidgetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPlaneWidgetReader - vtkPlaneWidget XML Reader.
// .SECTION Description
// vtkXMLPlaneWidgetReader provides XML reading functionality to 
// vtkPlaneWidget.
// .SECTION See Also
// vtkXMLPlaneWidgetWriter

#ifndef __vtkXMLPlaneWidgetReader_h
#define __vtkXMLPlaneWidgetReader_h

#include "vtkXMLPolyDataSourceWidgetReader.h"

class VTK_EXPORT vtkXMLPlaneWidgetReader : public vtkXMLPolyDataSourceWidgetReader
{
public:
  static vtkXMLPlaneWidgetReader* New();
  vtkTypeRevisionMacro(vtkXMLPlaneWidgetReader, vtkXMLPolyDataSourceWidgetReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLPlaneWidgetReader() {};
  ~vtkXMLPlaneWidgetReader() {};

private:
  vtkXMLPlaneWidgetReader(const vtkXMLPlaneWidgetReader&); // Not implemented
  void operator=(const vtkXMLPlaneWidgetReader&); // Not implemented    
};

#endif


