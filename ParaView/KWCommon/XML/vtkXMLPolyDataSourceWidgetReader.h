/*=========================================================================

  Module:    vtkXMLPolyDataSourceWidgetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPolyDataSourceWidgetReader - vtkPolyDataSourceWidget XML Reader.
// .SECTION Description
// vtkXMLPolyDataSourceWidgetReader provides XML reading functionality to 
// vtkPolyDataSourceWidget.
// .SECTION See Also
// vtkXMLPolyDataSourceWidgetWriter

#ifndef __vtkXMLPolyDataSourceWidgetReader_h
#define __vtkXMLPolyDataSourceWidgetReader_h

#include "vtkXML3DWidgetReader.h"

class VTK_EXPORT vtkXMLPolyDataSourceWidgetReader : public vtkXML3DWidgetReader
{
public:
  static vtkXMLPolyDataSourceWidgetReader* New();
  vtkTypeRevisionMacro(vtkXMLPolyDataSourceWidgetReader, vtkXML3DWidgetReader);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLPolyDataSourceWidgetReader() {};
  ~vtkXMLPolyDataSourceWidgetReader() {};

private:
  vtkXMLPolyDataSourceWidgetReader(const vtkXMLPolyDataSourceWidgetReader&); // Not implemented
  void operator=(const vtkXMLPolyDataSourceWidgetReader&); // Not implemented    
};

#endif


