/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLLookmarkElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// .NAME vtkXMLLookmarkElement - Represents an XML element and those nested inside.
// .SECTION Description
// vtkXMLLookmarkElement is used by vtkXMLDataParser to represent an XML
// element.  It provides methods to access the element's attributes
// and nested elements in a convenient manner.  This allows easy
// traversal of an input XML file by vtkXMLReader and its subclasses.

// .SECTION See Also
// vtkXMLDataParser

#ifndef __vtkXMLLookmarkElement_h
#define __vtkXMLLookmarkElement_h

#include "vtkXMLDataElement.h"

class vtkXMLDataParser;

class VTK_EXPORT vtkXMLLookmarkElement : public vtkXMLDataElement
{
public:

  vtkTypeRevisionMacro(vtkXMLLookmarkElement, vtkXMLDataElement)
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLLookmarkElement* New();

  void PrintXML(ostream& os, vtkIndent indent);
  
protected:
  vtkXMLLookmarkElement();
  ~vtkXMLLookmarkElement();  

private:
  vtkXMLLookmarkElement(const vtkXMLLookmarkElement&);  // Not implemented.
  void operator=(const vtkXMLLookmarkElement&);  // Not implemented.
};

#endif
