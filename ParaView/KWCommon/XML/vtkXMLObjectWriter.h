/*=========================================================================

  Module:    vtkXMLObjectWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLObjectWriter - XML Object Writer.
// .SECTION Description
// vtkXMLObjectWriter provides base functionalities for all XML writers.
// .SECTION See Also
// vtkXMLObjectReader

#ifndef __vtkXMLObjectWriter_h
#define __vtkXMLObjectWriter_h

#include "vtkXMLIOBase.h"

class vtkXMLDataElement;

class VTK_EXPORT vtkXMLObjectWriter : public vtkXMLIOBase
{
public:
  vtkTypeRevisionMacro(vtkXMLObjectWriter,vtkXMLIOBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create an XML representation of the object (in-place) by setting
  // the name, attributes and nested element of 'elem' according to the
  // current 'Object'.
  // Return 1 on success, 0 otherwise.
  virtual int Create(vtkXMLDataElement *elem);

  // Description:
  // Write an XML serialized representation of the object
  // Return 1 on success, 0 otherwise.
  virtual int WriteToStream(ostream &os, vtkIndent *indent = 0);
  virtual int WriteToFile(const char *filename);

  // Description:
  // Enable/Disable factorization of the XML tree on write.
  vtkSetClampMacro(WriteFactored, int, 0, 1);
  vtkGetMacro(WriteFactored, int);
  vtkBooleanMacro(WriteFactored, int);

  // Description:
  // Enable/Disable indentation of the XML tree on write.
  vtkSetClampMacro(WriteIndented, int, 0, 1);
  vtkGetMacro(WriteIndented, int);
  vtkBooleanMacro(WriteIndented, int);

  // Description:
  // Convenience method to create an XML representation of the object
  // (see Create()) and insert that representation (XML data element) 
  // inside 'parent'.
  // Return 1 on success, 0 otherwise.
  virtual int CreateInElement(vtkXMLDataElement *parent);

  // Description:
  // Convenience method to create a simple XML parent element with 
  // name 'name', insert it inside 'grandparent', then create an XML 
  // representation of the objet inside that parent (see CreateInElement()), 
  // thus creating a 2nd nested element.
  // Return 1 on success, 0 otherwise.
  virtual int CreateInNestedElement(vtkXMLDataElement *grandparent, 
                                    const char *name);

protected:
  vtkXMLObjectWriter();
  ~vtkXMLObjectWriter() {};  
  
  int WriteFactored;
  int WriteIndented;

  // Description:
  // Add the root element attributes.
  // Return 1 on success, 0 otherwise.
  virtual int AddAttributes(vtkXMLDataElement *elem);

  // Description:
  // Add the root element internal/nested elements
  // Return 1 on success, 0 otherwise.
  virtual int AddNestedElements(vtkXMLDataElement *elem);

private:
  vtkXMLObjectWriter(const vtkXMLObjectWriter&);  // Not implemented.
  void operator=(const vtkXMLObjectWriter&);  // Not implemented.
};

#endif


