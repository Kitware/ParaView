/*=========================================================================

  Module:    vtkXMLKWSelectionFrameWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWSelectionFrameWriter - vtkKWSelectionFrame XML Writer.
// .SECTION Description
// vtkXMLKWSelectionFrameWriter provides XML writing functionality to 
// vtkKWSelectionFrame.
// .SECTION See Also
// vtkXMLKWSelectionFrameReader

#ifndef __vtkXMLKWSelectionFrameWriter_h
#define __vtkXMLKWSelectionFrameWriter_h

#include "vtkXMLObjectWriter.h"

class VTK_EXPORT vtkXMLKWSelectionFrameWriter : public vtkXMLObjectWriter
{
public:
  static vtkXMLKWSelectionFrameWriter* New();
  vtkTypeRevisionMacro(vtkXMLKWSelectionFrameWriter,vtkXMLObjectWriter);

  // Description:
  // Return the name of the root element of the XML tree this writer
  // is supposed to write.
  virtual char* GetRootElementName();

protected:
  vtkXMLKWSelectionFrameWriter() {};
  ~vtkXMLKWSelectionFrameWriter() {};  
  
  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

private:
  vtkXMLKWSelectionFrameWriter(const vtkXMLKWSelectionFrameWriter&);  // Not implemented.
  void operator=(const vtkXMLKWSelectionFrameWriter&);  // Not implemented.
};

#endif

