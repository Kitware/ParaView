/*=========================================================================

  Module:    vtkXMLScalarsToColorsReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLScalarsToColorsReader - vtkScalarsToColors XML Reader.
// .SECTION Description
// vtkXMLScalarsToColorsReader provides XML reading functionality to 
// vtkScalarsToColors.
// .SECTION See Also
// vtkXMLScalarsToColorsWriter

#ifndef __vtkXMLScalarsToColorsReader_h
#define __vtkXMLScalarsToColorsReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLScalarsToColorsReader : public vtkXMLObjectReader
{
public:
  static vtkXMLScalarsToColorsReader* New();
  vtkTypeRevisionMacro(vtkXMLScalarsToColorsReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLScalarsToColorsReader() {};
  ~vtkXMLScalarsToColorsReader() {};

private:
  vtkXMLScalarsToColorsReader(const vtkXMLScalarsToColorsReader&); // Not implemented
  void operator=(const vtkXMLScalarsToColorsReader&); // Not implemented    
};

#endif


