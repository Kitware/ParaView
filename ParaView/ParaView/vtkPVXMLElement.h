/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLElement.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
// .NAME vtkPVXMLElement represents an XML element and those nested inside.
// .SECTION Description
// This is used by vtkPVXMLParser to represent an XML document starting
// at the root element.
#ifndef __vtkPVXMLElement_h
#define __vtkPVXMLElement_h

#include "vtkObject.h"

class vtkPVXMLParser;

class VTK_EXPORT vtkPVXMLElement : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkPVXMLElement,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVXMLElement* New();
  
  // Description:
  // Get the name of the element.  This is its XML tag.
  vtkGetStringMacro(Name);
  
  // Description:
  // Get the id of the element.
  vtkGetStringMacro(Id);
  
  // Description:
  // Get the attribute with the given name.  If it doesn't exist,
  // returns 0.
  const char* GetAttribute(const char* name);
  
  // Description:
  // Get the attribute with the given name and converted to a scalar
  // value.  Returns whether value was extracted.
  int GetScalarAttribute(const char* name, int* value);
  int GetScalarAttribute(const char* name, float* value);
  
  // Description:
  // Get the attribute with the given name and converted to a scalar
  // value.  Returns length of vector read.
  int GetVectorAttribute(const char* name, int length, int* value);
  int GetVectorAttribute(const char* name, int length, float* value);
  
  // Description:
  // Get the parent of this element.
  vtkPVXMLElement* GetParent();
  
  // Description:
  // Get the number of elements nested in this one.
  unsigned int GetNumberOfNestedElements();
  
  // Description:
  // Get the element nested in this one at the given index.
  vtkPVXMLElement* GetNestedElement(unsigned int index);
  
  // Description:
  // Find a nested element with the given id.
  vtkPVXMLElement* FindNestedElement(const char* id);
  
  // Description:
  // Lookup the element with the given id, starting at this scope.
  vtkPVXMLElement* LookupElement(const char* id);

protected:
  vtkPVXMLElement();
  ~vtkPVXMLElement();  
  
  char* Name;
  char* Id;
  
  // The raw property name/value pairs read from the XML attributes.
  char** AttributeNames;
  char** AttributeValues;
  unsigned int NumberOfAttributes;
  unsigned int AttributesSize;
  
  // The set of nested elements.
  unsigned int NumberOfNestedElements;
  unsigned int NestedElementsSize;
  vtkPVXMLElement** NestedElements;
  
  // The parent of this element.
  vtkPVXMLElement* Parent;
  
  // Method used by vtkPVXMLParser to setup the element.
  vtkSetStringMacro(Name);
  vtkSetStringMacro(Id);
  void ReadXMLAttributes(const char** atts);  
  void AddNestedElement(vtkPVXMLElement* element);
  
  void PrintXML(ostream& os, vtkIndent indent);

  // Internal utility methods.
  vtkPVXMLElement* LookupElementInScope(const char* id);
  vtkPVXMLElement* LookupElementUpScope(const char* id);
  void SetParent(vtkPVXMLElement* parent);
  
  //BTX
  friend class vtkPVXMLParser;
  //ETX
  
private:
  vtkPVXMLElement(const vtkPVXMLElement&);  // Not implemented.
  void operator=(const vtkPVXMLElement&);  // Not implemented.
};

#endif
