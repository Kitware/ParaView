/*=========================================================================

  Module:    vtkXMLScalarBarWidgetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLScalarBarWidgetReader - vtkScalarBarWidget XML Reader.
// .SECTION Description
// vtkXMLScalarBarWidgetReader provides XML reading functionality to 
// vtkScalarBarWidget.
// .SECTION See Also
// vtkXMLScalarBarWidgetWriter

#ifndef __vtkXMLScalarBarWidgetReader_h
#define __vtkXMLScalarBarWidgetReader_h

#include "vtkXMLInteractorObserverReader.h"

class VTK_EXPORT vtkXMLScalarBarWidgetReader : public vtkXMLInteractorObserverReader
{
public:
  static vtkXMLScalarBarWidgetReader* New();
  vtkTypeRevisionMacro(vtkXMLScalarBarWidgetReader, vtkXMLInteractorObserverReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLScalarBarWidgetReader() {};
  ~vtkXMLScalarBarWidgetReader() {};

private:
  vtkXMLScalarBarWidgetReader(const vtkXMLScalarBarWidgetReader&); // Not implemented
  void operator=(const vtkXMLScalarBarWidgetReader&); // Not implemented    
};

#endif


