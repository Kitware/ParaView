/*=========================================================================

  Module:    vtkXMLInteractorObserverWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLInteractorObserverWriter - vtkInteractorObserver XML Writer.
// .SECTION Description
// vtkXMLInteractorObserverWriter provides XML writing functionality to 
// vtkInteractorObserver.
// .SECTION See Also
// vtkXMLInteractorObserverReader

#ifndef __vtkXMLInteractorObserverWriter_h
#define __vtkXMLInteractorObserverWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLInteractorObserverWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLInteractorObserverWriter* New();
  vtkTypeRevisionMacro(vtkXMLInteractorObserverWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLInteractorObserverWriter() {};
  ~vtkXMLInteractorObserverWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLInteractorObserverWriter(const vtkXMLInteractorObserverWriter&);  // Not implemented.
  void operator=(const vtkXMLInteractorObserverWriter&);  // Not implemented.
};

#endif


