/*=========================================================================

  Module:    vtkXMLPiecewiseFunctionReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPiecewiseFunctionReader - vtkPiecewiseFunction XML Reader.
// .SECTION Description
// vtkXMLPiecewiseFunctionReader provides XML reading functionality to 
// vtkPiecewiseFunction.
// .SECTION See Also
// vtkXMLPiecewiseFunctionWriter

#ifndef __vtkXMLPiecewiseFunctionReader_h
#define __vtkXMLPiecewiseFunctionReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLPiecewiseFunctionReader : public vtkXMLObjectReader
{
public:
  static vtkXMLPiecewiseFunctionReader* New();
  vtkTypeRevisionMacro(vtkXMLPiecewiseFunctionReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLPiecewiseFunctionReader() {};
  ~vtkXMLPiecewiseFunctionReader() {};

private:
  vtkXMLPiecewiseFunctionReader(const vtkXMLPiecewiseFunctionReader&); // Not implemented
  void operator=(const vtkXMLPiecewiseFunctionReader&); // Not implemented    
};

#endif


