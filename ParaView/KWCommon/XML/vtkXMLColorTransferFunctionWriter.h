/*=========================================================================

  Module:    vtkXMLColorTransferFunctionWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLColorTransferFunctionWriter - vtkColorTransferFunction XML Writer.
// .SECTION Description
// vtkXMLColorTransferFunctionWriter provides XML writing functionality to 
// vtkColorTransferFunction.
// .SECTION See Also
// vtkXMLColorTransferFunctionReader

#ifndef __vtkXMLColorTransferFunctionWriter_h
#define __vtkXMLColorTransferFunctionWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLColorTransferFunctionWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLColorTransferFunctionWriter* New();
  vtkTypeRevisionMacro(vtkXMLColorTransferFunctionWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the point element used inside that tree to
  // store a point.
  static char* GetPointElementName();

protected:
  vtkXMLColorTransferFunctionWriter() {};
  ~vtkXMLColorTransferFunctionWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLColorTransferFunctionWriter(const vtkXMLColorTransferFunctionWriter&);  // Not implemented.
  void operator=(const vtkXMLColorTransferFunctionWriter&);  // Not implemented.
};

#endif


