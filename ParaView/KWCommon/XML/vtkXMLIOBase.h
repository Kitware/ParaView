/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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


