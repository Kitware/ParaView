/*=========================================================================

  Module:    vtkXMLCornerAnnotationReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLCornerAnnotationReader - vtkCornerAnnotation XML Reader.
// .SECTION Description
// vtkXMLCornerAnnotationReader provides XML reading functionality to 
// vtkCornerAnnotation.
// .SECTION See Also
// vtkXMLCornerAnnotationWriter

#ifndef __vtkXMLCornerAnnotationReader_h
#define __vtkXMLCornerAnnotationReader_h

#include "vtkXMLActor2DReader.h"

class VTK_EXPORT vtkXMLCornerAnnotationReader : public vtkXMLActor2DReader
{
public:
  static vtkXMLCornerAnnotationReader* New();
  vtkTypeRevisionMacro(vtkXMLCornerAnnotationReader, vtkXMLActor2DReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLCornerAnnotationReader() {};
  ~vtkXMLCornerAnnotationReader() {};

private:
  vtkXMLCornerAnnotationReader(const vtkXMLCornerAnnotationReader&); // Not implemented
  void operator=(const vtkXMLCornerAnnotationReader&); // Not implemented    
};

#endif

