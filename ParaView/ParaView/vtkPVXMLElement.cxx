/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVXMLElement.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVXMLElement, "1.6");
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
  os << "Id: " << (this->Id?this->Id:"<none>") << endl;
  os << "Name: " << (this->Name?this->Name:"<none>") << endl;
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
int vtkPVXMLVectorAttributeParse(const char* str, int length, T* data)
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
