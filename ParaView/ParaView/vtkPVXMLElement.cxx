/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLElement.cxx
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
#include "vtkPVXMLElement.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVXMLElement, "1.3");
vtkStandardNewMacro(vtkPVXMLElement);

//----------------------------------------------------------------------------
vtkPVXMLElement::vtkPVXMLElement()
{
  this->Name = 0;
  this->Id = 0;
  this->Parent = 0;
  
  this->NumberOfAttributes = 0;
  this->AttributesSize = 5;
  this->AttributeNames = new char*[this->AttributesSize];
  this->AttributeValues = new char*[this->AttributesSize];
  
  this->NumberOfNestedElements = 0;
  this->NestedElementsSize = 10;
  this->NestedElements = new vtkPVXMLElement*[this->NestedElementsSize];
}

//----------------------------------------------------------------------------
vtkPVXMLElement::~vtkPVXMLElement()
{
  this->SetName(0);
  this->SetId(0);
  unsigned int i;
  for(i=0;i < this->NumberOfAttributes;++i)
    {
    delete [] this->AttributeNames[i];
    delete [] this->AttributeValues[i];
    }
  delete [] this->AttributeNames;
  delete [] this->AttributeValues;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    this->NestedElements[i]->UnRegister(this);
    }
  delete [] this->NestedElements;
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::ReadXMLAttributes(const char** atts)
{
  if(this->NumberOfAttributes > 0)
    {
    unsigned int i;
    for(i=0;i < this->NumberOfAttributes;++i)
      {
      delete [] this->AttributeNames[i];
      delete [] this->AttributeValues[i];
      }
    this->NumberOfAttributes = 0;
    }
  if(atts)
    {
    const char** attsIter = atts;
    unsigned int count=0;
    while(*attsIter++) { ++count; }
    this->NumberOfAttributes = count/2;
    this->AttributesSize = this->NumberOfAttributes;
    
    delete [] this->AttributeNames;
    delete [] this->AttributeValues;
    this->AttributeNames = new char* [this->AttributesSize];
    this->AttributeValues = new char* [this->AttributesSize];
    
    unsigned int i;
    for(i=0;i < this->NumberOfAttributes; ++i)
      {
      this->AttributeNames[i] = new char[strlen(atts[i*2])+1];
      strcpy(this->AttributeNames[i], atts[i*2]);
      
      this->AttributeValues[i] = new char[strlen(atts[i*2+1])+1];
      strcpy(this->AttributeValues[i], atts[i*2+1]);
      }
    }  
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddNestedElement(vtkPVXMLElement* element)
{
  if(this->NumberOfNestedElements == this->NestedElementsSize)
    {
    unsigned int i;
    unsigned int newSize = this->NestedElementsSize*2;
    vtkPVXMLElement** newNestedElements = new vtkPVXMLElement*[newSize];
    for(i=0;i < this->NumberOfNestedElements;++i)
      {
      newNestedElements[i] = this->NestedElements[i];
      }
    delete [] this->NestedElements;
    this->NestedElements = newNestedElements;
    this->NestedElementsSize = newSize;
    }
  
  unsigned int index = this->NumberOfNestedElements++;
  this->NestedElements[index] = element;
  element->Register(this);
  element->SetParent(this);
}

//----------------------------------------------------------------------------
const char* vtkPVXMLElement::GetAttribute(const char* name)
{
  unsigned int i;
  for(i=0; i < this->NumberOfAttributes;++i)
    {
    if(strcmp(this->AttributeNames[i], name) == 0)
      {
      return this->AttributeValues[i];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::PrintXML(ostream& os, vtkIndent indent)
{
  os << indent << "<" << this->Name;
  unsigned int i;
  for(i=0;i < this->NumberOfAttributes;++i)
    {
    os << " " << this->AttributeNames[i]
       << "=\"" << this->AttributeValues[i] << "\"";
    }
  if(this->NumberOfNestedElements > 0)
    {
    os << ">\n";
    for(i=0;i < this->NumberOfNestedElements;++i)
      {
      vtkIndent nextIndent = indent.GetNextIndent();
      this->NestedElements[i]->PrintXML(os, nextIndent);
      }
    os << indent << "</" << this->Name << ">\n";
    }
  else
    {
    os << "/>\n";
    }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::SetParent(vtkPVXMLElement* parent)
{
  this->Parent = parent;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::GetParent()
{
  return this->Parent;
}

//----------------------------------------------------------------------------
unsigned int vtkPVXMLElement::GetNumberOfNestedElements()
{
  return this->NumberOfNestedElements;
}
  
//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::GetNestedElement(unsigned int index)
{
  if(index < this->NumberOfNestedElements)
    {
    return this->NestedElements[index];
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::LookupElement(const char* id)
{
  return this->LookupElementUpScope(id);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::FindNestedElement(const char* id)
{
  unsigned int i;
  for(i=0;i < this->NumberOfNestedElements;++i)
    {
    const char* nid = this->NestedElements[i]->GetId();
    if(strcmp(nid, id) == 0)
      {
      return this->NestedElements[i];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::LookupElementInScope(const char* id)
{
  // Pull off the first qualifier.
  const char* end = id;
  while(*end && (*end != '.')) ++end;
  unsigned int len = end - id;
  char* name = new char[len+1];
  strncpy(name, id, len);
  name[len] = '\0';
  
  // Find the qualifier in this scope.
  vtkPVXMLElement* next = this->FindNestedElement(name);  
  if(next && (*end == '.'))
    {
    // Lookup rest of qualifiers in nested scope.
    next = next->LookupElementInScope(end+1);
    }
  
  delete [] name;
  return next;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::LookupElementUpScope(const char* id)
{
  // Pull off the first qualifier.
  const char* end = id;
  while(*end && (*end != '.')) ++end;
  unsigned int len = end - id;
  char* name = new char[len+1];
  strncpy(name, id, len);
  name[len] = '\0';
  
  // Find most closely nested occurrence of first qualifier.
  vtkPVXMLElement* curScope = this;
  vtkPVXMLElement* start = 0;
  while(curScope && !start)
    {
    start = curScope->FindNestedElement(name);
    curScope = curScope->GetParent();
    }
  if(start && (*end == '.'))
    {
    start = start->LookupElementInScope(end+1);
    }
  
  delete [] name;
  return start;
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetScalarAttribute(const char* name, int* value)
{
  return this->GetVectorAttribute(name, 1, value);
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetScalarAttribute(const char* name, float* value)
{
  return this->GetVectorAttribute(name, 1, value);
}

//----------------------------------------------------------------------------
template <class T>
static int vtkPVXMLVectorAttributeParse(const char* str, int length, T* data)
{
  if(!str || !length) { return 0; }
  strstream vstr;
  vstr << str << ends;  
  int i;
  for(i=0;i < length;++i)
    {
    vstr >> data[i];
    if(!vstr) { return i; }
    }
  return length;
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length,
                                        int* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length,
                                        float* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}
