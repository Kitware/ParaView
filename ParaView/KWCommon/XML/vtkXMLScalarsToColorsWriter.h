/*=========================================================================

  Module:    vtkXMLScalarsToColorsWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLScalarsToColorsWriter - vtkScalarsToColors XML Writer.
// .SECTION Description
// vtkXMLScalarsToColorsWriter provides XML writing functionality to 
// vtkScalarsToColors.
// .SECTION See Also
// vtkXMLScalarsToColorsReader

#ifndef __vtkXMLScalarsToColorsWriter_h
#define __vtkXMLScalarsToColorsWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLScalarsToColorsWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLScalarsToColorsWriter* New();
  vtkTypeRevisionMacro(vtkXMLScalarsToColorsWriter,vtkXMLObjectWriter);
  
  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLScalarsToColorsWriter() {};
  ~vtkXMLScalarsToColorsWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLScalarsToColorsWriter(const vtkXMLScalarsToColorsWriter&);  // Not implemented.
  void operator=(const vtkXMLScalarsToColorsWriter&);  // Not implemented.
};

#endif


