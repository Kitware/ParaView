/*=========================================================================

  Module:    vtkXML3DWidgetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXML3DWidgetReader - vtk3DWidget XML Reader.
// .SECTION Description
// vtkXML3DWidgetReader provides XML reading functionality to 
// vtk3DWidget.
// .SECTION See Also
// vtkXML3DWidgetWriter

#ifndef __vtkXML3DWidgetReader_h
#define __vtkXML3DWidgetReader_h

#include "vtkXMLInteractorObserverReader.h"

class VTK_EXPORT vtkXML3DWidgetReader : public vtkXMLInteractorObserverReader
{
public:
  static vtkXML3DWidgetReader* New();
  vtkTypeRevisionMacro(vtkXML3DWidgetReader, vtkXMLInteractorObserverReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXML3DWidgetReader() {};
  ~vtkXML3DWidgetReader() {};

private:
  vtkXML3DWidgetReader(const vtkXML3DWidgetReader&); // Not implemented
  void operator=(const vtkXML3DWidgetReader&); // Not implemented    
};

#endif



