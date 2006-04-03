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
#include "vtkSmartPointer.h"

vtkCxxRevisionMacro(vtkPVXMLElement, "1.9");
vtkStandardNewMacro(vtkPVXMLElement);

#include <vtkstd/string>
#include <vtkstd/vector>

struct vtkPVXMLElementInternals
{
  vtkstd::vector<vtkstd::string> AttributeNames;
  vtkstd::vector<vtkstd::string> AttributeValues;
  vtkstd::vector<vtkSmartPointer<vtkPVXMLElement> > NestedElements;
  vtkstd::string CharacterData;
};

//----------------------------------------------------------------------------
vtkPVXMLElement::vtkPVXMLElement()
{
  this->Name = 0;
  this->Id = 0;
  this->Parent = 0;

  this->Internal = new vtkPVXMLElementInternals;
}

//----------------------------------------------------------------------------
vtkPVXMLElement::~vtkPVXMLElement()
{
  this->SetName(0);
  this->SetId(0);

  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Id: " << (this->Id?this->Id:"<none>") << endl;
  os << indent << "Name: " << (this->Name?this->Name:"<none>") << endl;
  unsigned int numNested = this->GetNumberOfNestedElements();
  for (unsigned int i=0; i< numNested; i++)
    {
    if (this->GetNestedElement(i))
      {
      this->GetNestedElement(i)->PrintSelf(os, indent.GetNextIndent());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, 
                                   unsigned int attrValue)
{
  ostrstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str());
  delete[] valueStr.str();
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, int attrValue)
{
  ostrstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str());
  delete[] valueStr.str();
}

#if defined(VTK_USE_64BIT_IDS)
//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, vtkIdType attrValue)
{
  ostrstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str());
  delete[] valueStr.str();
}
#endif

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, double attrValue)
{
  ostrstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str());
  delete[] valueStr.str();
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName,
                                   const char* attrValue)
{
  if (!attrName || !attrValue)
    {
    return;
    }
  
  this->Internal->AttributeNames.push_back(attrName);
  this->Internal->AttributeValues.push_back(attrValue);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::SetAttribute(const char* attrName,
  const char* attrValue)
{
  if (!attrName || !attrValue)
    {
    return;
    }
  
  // iterate over the names, and find if the attribute name exists.
  unsigned int numAttributes = this->Internal->AttributeNames.size();
  unsigned int i;
  for(i=0; i < numAttributes; ++i)
    {
    if(strcmp(this->Internal->AttributeNames[i].c_str(), attrName) == 0)
      {
      this->Internal->AttributeValues[i] = attrValue;
      return;
      }
    }
  // add the attribute.
  this->AddAttribute(attrName, attrValue);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::ReadXMLAttributes(const char** atts)
{
  this->Internal->AttributeNames.clear();
  this->Internal->AttributeValues.clear();

  if(atts)
    {
    const char** attsIter = atts;
    unsigned int count=0;
    while(*attsIter++) { ++count; }
    unsigned int numberOfAttributes = count/2;

    unsigned int i;
    for(i=0;i < numberOfAttributes; ++i)
      {
      this->AddAttribute(atts[i*2], atts[i*2+1]);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::RemoveAllNestedElements()
{
  this->Internal->NestedElements.clear();
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddNestedElement(vtkPVXMLElement* element)
{
  this->AddNestedElement(element, 1);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddNestedElement(vtkPVXMLElement* element, int setParent)
{
  if (setParent)
    {
    element->SetParent(this);
    }
  this->Internal->NestedElements.push_back(element);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddCharacterData(const char* data, int length)
{
  this->Internal->CharacterData.append(data, length);
}

//----------------------------------------------------------------------------
const char* vtkPVXMLElement::GetAttribute(const char* name)
{
  unsigned int numAttributes = this->Internal->AttributeNames.size();
  unsigned int i;
  for(i=0; i < numAttributes; ++i)
    {
    if(strcmp(this->Internal->AttributeNames[i].c_str(), name) == 0)
      {
      return this->Internal->AttributeValues[i].c_str();
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
const char* vtkPVXMLElement::GetCharacterData()
{
  return this->Internal->CharacterData.c_str();
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::PrintXML()
{
  this->PrintXML(cout, vtkIndent());
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::PrintXML(ostream& os, vtkIndent indent)
{
  os << indent << "<" << (this->Name?this->Name:"NoName");
  unsigned int numAttributes = this->Internal->AttributeNames.size();
  unsigned int i;
  for(i=0;i < numAttributes; ++i)
    {
    const char* aName = this->Internal->AttributeNames[i].c_str();
    const char* aValue = this->Internal->AttributeValues[i].c_str();
    os << " " << (aName?aName:"NoName")
       << "=\"" << (aValue?aValue:"NoValue") << "\"";
    }
  unsigned int numberOfNestedElements = this->Internal->NestedElements.size();
  if(numberOfNestedElements > 0)
    {
    os << ">\n";
    for(i=0;i < numberOfNestedElements;++i)
      {
      vtkIndent nextIndent = indent.GetNextIndent();
      this->Internal->NestedElements[i]->PrintXML(os, nextIndent);
      }
    os << indent << "</" << (this->Name?this->Name:"NoName") << ">\n";
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
  return this->Internal->NestedElements.size();
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::GetNestedElement(unsigned int index)
{
  if(index < this->Internal->NestedElements.size())
    {
    return this->Internal->NestedElements[index];
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
  unsigned int numberOfNestedElements = this->Internal->NestedElements.size();
  unsigned int i;
  for(i=0;i < numberOfNestedElements;++i)
    {
    const char* nid = this->Internal->NestedElements[i]->GetId();
    if(nid && strcmp(nid, id) == 0)
      {
      return this->Internal->NestedElements[i];
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
int vtkPVXMLElement::GetScalarAttribute(const char* name, double* value)
{
  return this->GetVectorAttribute(name, 1, value);
}

#if defined(VTK_USE_64BIT_IDS)
//----------------------------------------------------------------------------
int vtkPVXMLElement::GetScalarAttribute(const char* name, vtkIdType* value)
{
  return this->GetVectorAttribute(name, 1, value);
}
#endif

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

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length,
                                        double* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

#if defined(VTK_USE_64BIT_IDS)
//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length,
                                        vtkIdType* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}
#endif
