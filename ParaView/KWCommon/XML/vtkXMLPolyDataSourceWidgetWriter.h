/*=========================================================================

  Module:    vtkXMLPolyDataSourceWidgetWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPolyDataSourceWidgetWriter - vtkPolyDataSourceWidget XML Writer.
// .SECTION Description
// vtkXMLPolyDataSourceWidgetWriter provides XML writing functionality to 
// vtkPolyDataSourceWidget.
// .SECTION See Also
// vtkXMLPolyDataSourceWidgetReader

#ifndef __vtkXMLPolyDataSourceWidgetWriter_h
#define __vtkXMLPolyDataSourceWidgetWriter_h

#include "vtkXML3DWidgetWriter.h"

class VTK_EXPORT vtkXMLPolyDataSourceWidgetWriter : public vtkXML3DWidgetWriter
{
public:
  static vtkXMLPolyDataSourceWidgetWriter* New();
  vtkTypeRevisionMacro(vtkXMLPolyDataSourceWidgetWriter,vtkXML3DWidgetWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLPolyDataSourceWidgetWriter() {};
  ~vtkXMLPolyDataSourceWidgetWriter() {};  
  
private:
  vtkXMLPolyDataSourceWidgetWriter(const vtkXMLPolyDataSourceWidgetWriter&);  // Not implemented.
  void operator=(const vtkXMLPolyDataSourceWidgetWriter&);  // Not implemented.
};

#endif


