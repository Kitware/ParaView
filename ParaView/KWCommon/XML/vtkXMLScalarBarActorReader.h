/*=========================================================================

  Module:    vtkXMLScalarBarActorReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLScalarBarActorReader - vtkScalarBarActor XML Reader.
// .SECTION Description
// vtkXMLScalarBarActorReader provides XML reading functionality to 
// vtkScalarBarActor.
// .SECTION See Also
// vtkXMLScalarBarActorWriter

#ifndef __vtkXMLScalarBarActorReader_h
#define __vtkXMLScalarBarActorReader_h

#include "vtkXMLActor2DReader.h"

class VTK_EXPORT vtkXMLScalarBarActorReader : public vtkXMLActor2DReader
{
public:
  static vtkXMLScalarBarActorReader* New();
  vtkTypeRevisionMacro(vtkXMLScalarBarActorReader, vtkXMLActor2DReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLScalarBarActorReader() {};
  ~vtkXMLScalarBarActorReader() {};

private:
  vtkXMLScalarBarActorReader(const vtkXMLScalarBarActorReader&); // Not implemented
  void operator=(const vtkXMLScalarBarActorReader&); // Not implemented    
};

#endif


