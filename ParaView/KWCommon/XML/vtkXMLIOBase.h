/*=========================================================================

  Module:    vtkXMLIOBase.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLIOBase - Base XML I/O functionalitues.
// .SECTION Description
// vtkXMLIOBase provides base functionalities for all XML writers/readers.
// .SECTION See Also
// vtkXMLObjectReader vtkXMLObjectWriter

#ifndef __vtkXMLIOBase_h
#define __vtkXMLIOBase_h

#include "vtkObject.h"

class vtkXMLDataElement;

class VTK_EXPORT vtkXMLIOBase : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkXMLIOBase,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the object to serialize/deserialize.
  virtual void SetObject(vtkObject*);
  vtkGetObjectMacro(Object, vtkObject);

  // Description:
  // Set the default global character encoding that will be used to store
  // textual values internally.
  // Note that each vtkXMLDataElement created through NewDataElement()
  // will use this encoding to set its attribute encoding.
  static void SetDefaultCharacterEncoding(int val);
  static int GetDefaultCharacterEncoding();

  // Description:
  // Create and return a new instance of vtkXMLDataElement.
  // Use this function in favor of vtkXMLDataElement::New() so that the
  // data element created through NewDataElement() will automatically
  // be tuned according to the current Ivars (example: the attribute
  // encoding of the new data element will be set to the value of out global 
  // DefaultCharacterEncoding).
  // It is up to the caller to invoke Delete() on the returned element.
  virtual vtkXMLDataElement* NewDataElement();

  // Description:
  // Return the name of the root element of the XML tree this I/O
  // object will read (expect) or write (create).
  virtual char* GetRootElementName() = 0;

  // Description:
  // Set/Get the error log.
  vtkSetStringMacro(ErrorLog);
  vtkGetStringMacro(ErrorLog);
  virtual void AppendToErrorLog(const char *);

protected:
  vtkXMLIOBase();
  ~vtkXMLIOBase();  

  vtkObject *Object;

  char *ErrorLog;

private:
  vtkXMLIOBase(const vtkXMLIOBase&);  // Not implemented.
  void operator=(const vtkXMLIOBase&);  // Not implemented.
};

#endif


