/*=========================================================================

  Module:    vtkXMLLightWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLLightWriter - vtkLight XML Writer.
// .SECTION Description
// vtkXMLLightWriter provides XML writing functionality to 
// vtkLight.
// .SECTION See Also
// vtkXMLLightReader

#ifndef __vtkXMLLightWriter_h
#define __vtkXMLLightWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLLightWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLLightWriter* New();
  vtkTypeRevisionMacro(vtkXMLLightWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLLightWriter() {};
  ~vtkXMLLightWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLLightWriter(const vtkXMLLightWriter&);  // Not implemented.
  void operator=(const vtkXMLLightWriter&);  // Not implemented.
};

#endif


