/*=========================================================================

  Module:    vtkXMLProp3DReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLProp3DReader - vtkProp3D XML Reader.
// .SECTION Description
// vtkXMLProp3DReader provides XML reading functionality to 
// vtkProp3D.
// .SECTION See Also
// vtkXMLProp3DWriter

#ifndef __vtkXMLProp3DReader_h
#define __vtkXMLProp3DReader_h

#include "vtkXMLPropReader.h"

class VTK_EXPORT vtkXMLProp3DReader : public vtkXMLPropReader
{
public:
  static vtkXMLProp3DReader* New();
  vtkTypeRevisionMacro(vtkXMLProp3DReader, vtkXMLPropReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLProp3DReader() {};
  ~vtkXMLProp3DReader() {};

private:
  vtkXMLProp3DReader(const vtkXMLProp3DReader&); // Not implemented
  void operator=(const vtkXMLProp3DReader&); // Not implemented    
};

#endif


