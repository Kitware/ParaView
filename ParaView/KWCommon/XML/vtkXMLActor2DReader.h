/*=========================================================================

  Module:    vtkXMLActor2DReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLActor2DReader - vtkActor2D XML Reader.
// .SECTION Description
// vtkXMLActor2DReader provides XML reading functionality to 
// vtkActor2D.
// .SECTION See Also
// vtkXMLActor2DWriter

#ifndef __vtkXMLActor2DReader_h
#define __vtkXMLActor2DReader_h

#include "vtkXMLPropReader.h"

class VTK_EXPORT vtkXMLActor2DReader : public vtkXMLPropReader
{
public:
  static vtkXMLActor2DReader* New();
  vtkTypeRevisionMacro(vtkXMLActor2DReader, vtkXMLPropReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLActor2DReader() {};
  ~vtkXMLActor2DReader() {};

private:
  vtkXMLActor2DReader(const vtkXMLActor2DReader&); // Not implemented
  void operator=(const vtkXMLActor2DReader&); // Not implemented    
};

#endif



