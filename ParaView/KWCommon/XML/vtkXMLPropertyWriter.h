/*=========================================================================

  Module:    vtkXMLPropertyWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPropertyWriter - vtkProperty XML Writer.
// .SECTION Description
// vtkXMLPropertyWriter provides XML writing functionality to 
// vtkProperty.
// .SECTION See Also
// vtkXMLPropertyReader

#ifndef __vtkXMLPropertyWriter_h
#define __vtkXMLPropertyWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLPropertyWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLPropertyWriter* New();
  vtkTypeRevisionMacro(vtkXMLPropertyWriter,vtkXMLObjectWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Output part of the object selectively
  vtkBooleanMacro(OutputShadingOnly, int);
  vtkGetMacro(OutputShadingOnly, int);
  vtkSetMacro(OutputShadingOnly, int);

protected:
  vtkXMLPropertyWriter();
  ~vtkXMLPropertyWriter() {};  
  
  int OutputShadingOnly;

  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLPropertyWriter(const vtkXMLPropertyWriter&);  // Not implemented.
  void operator=(const vtkXMLPropertyWriter&);  // Not implemented.
};

#endif


