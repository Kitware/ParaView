/*=========================================================================

  Module:    vtkXMLKWRenderWidgetReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWRenderWidgetReader - vtkKWRenderWidget XML Reader.
// .SECTION Description
// vtkXMLKWRenderWidgetReader provides XML reading functionality to 
// vtkKWRenderWidget.
// .SECTION See Also
// vtkXMLKWRenderWidgetWriter

#ifndef __vtkXMLKWRenderWidgetReader_h
#define __vtkXMLKWRenderWidgetReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLKWRenderWidgetReader : public vtkXMLObjectReader
{
public:
  static vtkXMLKWRenderWidgetReader* New();
  vtkTypeRevisionMacro(vtkXMLKWRenderWidgetReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLKWRenderWidgetReader() {};
  ~vtkXMLKWRenderWidgetReader() {};

private:
  vtkXMLKWRenderWidgetReader(const vtkXMLKWRenderWidgetReader&); // Not implemented
  void operator=(const vtkXMLKWRenderWidgetReader&); // Not implemented    
};

#endif

