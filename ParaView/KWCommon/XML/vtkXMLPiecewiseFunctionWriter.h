/*=========================================================================

  Module:    vtkXMLPiecewiseFunctionWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPiecewiseFunctionWriter - vtkPiecewiseFunction XML Writer.
// .SECTION Description
// vtkXMLPiecewiseFunctionWriter provides XML writing functionality to 
// vtkPiecewiseFunction.
// .SECTION See Also
// vtkXMLPiecewiseFunctionReader

#ifndef __vtkXMLPiecewiseFunctionWriter_h
#define __vtkXMLPiecewiseFunctionWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLPiecewiseFunctionWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLPiecewiseFunctionWriter* New();
  vtkTypeRevisionMacro(vtkXMLPiecewiseFunctionWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

  // Description:
  // Return the name of the point element used inside that tree to
  // store a point.
  static char* GetPointElementName();

protected:
  vtkXMLPiecewiseFunctionWriter() {};
  ~vtkXMLPiecewiseFunctionWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLPiecewiseFunctionWriter(const vtkXMLPiecewiseFunctionWriter&);  // Not implemented.
  void operator=(const vtkXMLPiecewiseFunctionWriter&);  // Not implemented.
};

#endif


