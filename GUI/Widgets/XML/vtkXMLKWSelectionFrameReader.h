/*=========================================================================

  Module:    vtkXMLKWSelectionFrameReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLKWSelectionFrameReader - vtkKWSelectionFrame XML Reader.
// .SECTION Description
// vtkXMLKWSelectionFrameReader provides XML reading functionality to 
// vtkKWSelectionFrame.
// .SECTION See Also
// vtkXMLKWSelectionFrameWriter

#ifndef __vtkXMLKWSelectionFrameReader_h
#define __vtkXMLKWSelectionFrameReader_h

#include "vtkXMLObjectReader.h"

class VTK_EXPORT vtkXMLKWSelectionFrameReader : public vtkXMLObjectReader
{
public:
  static vtkXMLKWSelectionFrameReader* New();
  vtkTypeRevisionMacro(vtkXMLKWSelectionFrameReader, vtkXMLObjectReader);

  // Description:
  // Parse an XML tree.
  // Return 1 on success, 0 on error.
  virtual int Parse(vtkXMLDataElement*);

  // Description:
  // Return the name of the root element of the XML tree this reader
  // is supposed to read and process.
  virtual char* GetRootElementName();

protected:  
  vtkXMLKWSelectionFrameReader() {};
  ~vtkXMLKWSelectionFrameReader() {};

private:
  vtkXMLKWSelectionFrameReader(const vtkXMLKWSelectionFrameReader&); // Not implemented
  void operator=(const vtkXMLKWSelectionFrameReader&); // Not implemented    
};

#endif

