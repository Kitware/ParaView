/*=========================================================================

  Module:    vtkXMLActorReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLActorReader - vtkActor XML Reader.
// .SECTION Description
// vtkXMLActorReader provides XML reading functionality to 
// vtkActor.
// .SECTION See Also
// vtkXMLActorWriter

#ifndef __vtkXMLActorReader_h
#define __vtkXMLActorReader_h

#include "vtkXMLProp3DReader.h"

class VTK_EXPORT vtkXMLActorReader : public vtkXMLProp3DReader
{
public:
  static vtkXMLActorReader* New();
  vtkTypeRevisionMacro(vtkXMLActorReader, vtkXMLProp3DReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLActorReader() {};
  ~vtkXMLActorReader() {};

private:
  vtkXMLActorReader(const vtkXMLActorReader&); // Not implemented
  void operator=(const vtkXMLActorReader&); // Not implemented    
};

#endif



