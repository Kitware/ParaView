/*=========================================================================

  Module:    vtkXMLScalarBarWidgetWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLScalarBarWidgetWriter - vtkScalarBarWidget XML Writer.
// .SECTION Description
// vtkXMLScalarBarWidgetWriter provides XML writing functionality to 
// vtkScalarBarWidget.
// .SECTION See Also
// vtkXMLScalarBarWidgetReader

#ifndef __vtkXMLScalarBarWidgetWriter_h
#define __vtkXMLScalarBarWidgetWriter_h

#include "vtkXMLInteractorObserverWriter.h"

class VTK_EXPORT vtkXMLScalarBarWidgetWriter : public vtkXMLInteractorObserverWriter
{
public:
  static vtkXMLScalarBarWidgetWriter* New();
  vtkTypeRevisionMacro(vtkXMLScalarBarWidgetWriter,vtkXMLInteractorObserverWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLScalarBarWidgetWriter() {};
  ~vtkXMLScalarBarWidgetWriter() {};  
  
  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLScalarBarWidgetWriter(const vtkXMLScalarBarWidgetWriter&);  // Not implemented.
  void operator=(const vtkXMLScalarBarWidgetWriter&);  // Not implemented.
};

#endif


