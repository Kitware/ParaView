/*=========================================================================

  Module:    vtkXMLTextActorReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLTextActorReader - vtkTextActor XML Reader.
// .SECTION Description
// vtkXMLTextActorReader provides XML reading functionality to 
// vtkTextActor.
// .SECTION See Also
// vtkXMLTextActorWriter

#ifndef __vtkXMLTextActorReader_h
#define __vtkXMLTextActorReader_h

#include "vtkXMLActor2DReader.h"

class VTK_EXPORT vtkXMLTextActorReader : public vtkXMLActor2DReader
{
public:
  static vtkXMLTextActorReader* New();
  vtkTypeRevisionMacro(vtkXMLTextActorReader, vtkXMLActor2DReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLTextActorReader() {};
  ~vtkXMLTextActorReader() {};

private:
  vtkXMLTextActorReader(const vtkXMLTextActorReader&); // Not implemented
  void operator=(const vtkXMLTextActorReader&); // Not implemented    
};

#endif


