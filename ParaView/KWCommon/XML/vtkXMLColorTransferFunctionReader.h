/*=========================================================================

  Module:    vtkXMLColorTransferFunctionReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLColorTransferFunctionReader - vtkColorTransferFunction XML Reader.
// .SECTION Description
// vtkXMLColorTransferFunctionReader provides XML reading functionality to 
// vtkColorTransferFunction.
// .SECTION See Also
// vtkXMLColorTransferFunctionWriter

#ifndef __vtkXMLColorTransferFunctionReader_h
#define __vtkXMLColorTransferFunctionReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLColorTransferFunctionReader : public vtkXMLObjectReader
{
public:
  static vtkXMLColorTransferFunctionReader* New();
  vtkTypeRevisionMacro(vtkXMLColorTransferFunctionReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLColorTransferFunctionReader() {};
  ~vtkXMLColorTransferFunctionReader() {};

private:
  vtkXMLColorTransferFunctionReader(const vtkXMLColorTransferFunctionReader&); // Not implemented
  void operator=(const vtkXMLColorTransferFunctionReader&); // Not implemented    
};

#endif


