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


