/*=========================================================================

  Module:    vtkXMLInteractorObserverReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLInteractorObserverReader - vtkInteractorObserver XML Reader.
// .SECTION Description
// vtkXMLInteractorObserverReader provides XML reading functionality to 
// vtkInteractorObserver.
// .SECTION See Also
// vtkXMLInteractorObserverWriter

#ifndef __vtkXMLInteractorObserverReader_h
#define __vtkXMLInteractorObserverReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLInteractorObserverReader : public vtkXMLObjectReader
{
public:
  static vtkXMLInteractorObserverReader* New();
  vtkTypeRevisionMacro(vtkXMLInteractorObserverReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLInteractorObserverReader() {};
  ~vtkXMLInteractorObserverReader() {};

private:
  vtkXMLInteractorObserverReader(const vtkXMLInteractorObserverReader&); // Not implemented
  void operator=(const vtkXMLInteractorObserverReader&); // Not implemented    
};

#endif


