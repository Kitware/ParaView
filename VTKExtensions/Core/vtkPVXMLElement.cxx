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

#include "vtkCollection.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkPVXMLElement);

#include <ctype.h>
#include <sstream>
#include <string>
#include <vector>
#if defined(_WIN32) && !defined(__CYGWIN__)
#define SNPRINTF _snprintf
#else
#define SNPRINTF snprintf
#endif

struct vtkPVXMLElementInternals
{
  std::vector<std::string> AttributeNames;
  std::vector<std::string> AttributeValues;
  typedef std::vector<vtkSmartPointer<vtkPVXMLElement> > VectorOfElements;
  VectorOfElements NestedElements;
  std::string CharacterData;
};

// Function to check if a string is full of whitespace characters.
static bool vtkIsSpace(const std::string& str)
{
  for (std::string::size_type cc = 0; cc < str.length(); ++cc)
  {
    if (!isspace(str[cc]))
    {
      return false;
    }
  }
  return true;
}

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
  os << indent << "Id: " << (this->Id ? this->Id : "<none>") << endl;
  os << indent << "Name: " << (this->Name ? this->Name : "<none>") << endl;
  unsigned int numNested = this->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numNested; i++)
  {
    if (this->GetNestedElement(i))
    {
      this->GetNestedElement(i)->PrintSelf(os, indent.GetNextIndent());
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, unsigned int attrValue)
{
  std::ostringstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str().c_str());
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, int attrValue)
{
  std::ostringstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str().c_str());
}

#if defined(VTK_USE_64BIT_IDS)
//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, vtkIdType attrValue)
{
  std::ostringstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str().c_str());
}
#endif

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, double attrValue)
{
  std::ostringstream valueStr;
  valueStr << attrValue << ends;
  this->AddAttribute(attrName, valueStr.str().c_str());
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, double attrValue, int precision)
{
  if (precision <= 0)
  {
    this->AddAttribute(attrName, attrValue);
  }
  else
  {
    std::ostringstream valueStr;
    valueStr << setprecision(precision) << attrValue << ends;
    this->AddAttribute(attrName, valueStr.str().c_str());
  }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::AddAttribute(const char* attrName, const char* attrValue)
{
  if (!attrName || !attrValue)
  {
    return;
  }

  this->Internal->AttributeNames.push_back(attrName);
  this->Internal->AttributeValues.push_back(attrValue);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::SetAttribute(const char* attrName, const char* attrValue)
{
  if (!attrName || !attrValue)
  {
    return;
  }

  // iterate over the names, and find if the attribute name exists.
  size_t numAttributes = this->Internal->AttributeNames.size();
  size_t i;
  for (i = 0; i < numAttributes; ++i)
  {
    if (strcmp(this->Internal->AttributeNames[i].c_str(), attrName) == 0)
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

  if (atts)
  {
    const char** attsIter = atts;
    unsigned int count = 0;
    while (*attsIter++)
    {
      ++count;
    }
    unsigned int numberOfAttributes = count / 2;

    unsigned int i;
    for (i = 0; i < numberOfAttributes; ++i)
    {
      this->AddAttribute(atts[i * 2], atts[i * 2 + 1]);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::RemoveAllNestedElements()
{
  this->Internal->NestedElements.clear();
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::RemoveNestedElement(vtkPVXMLElement* element)
{
  std::vector<vtkSmartPointer<vtkPVXMLElement> >::iterator iter =
    this->Internal->NestedElements.begin();
  for (; iter != this->Internal->NestedElements.end(); ++iter)
  {
    if (iter->GetPointer() == element)
    {
      this->Internal->NestedElements.erase(iter);
      break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::ReplaceNestedElement(
  vtkPVXMLElement* elementToReplace, vtkPVXMLElement* element)
{
  for (auto& elem : this->Internal->NestedElements)
  {
    if (elem.GetPointer() == elementToReplace)
    {
      elem = element;
      break;
    }
  }
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
const char* vtkPVXMLElement::GetAttributeOrDefault(const char* name, const char* notFound)
{
  size_t numAttributes = this->Internal->AttributeNames.size();
  size_t i;
  for (i = 0; i < numAttributes; ++i)
  {
    if (strcmp(this->Internal->AttributeNames[i].c_str(), name) == 0)
    {
      return this->Internal->AttributeValues[i].c_str();
    }
  }
  return notFound;
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
  os << indent << "<" << (this->Name ? this->Name : "NoName");
  size_t numAttributes = this->Internal->AttributeNames.size();
  size_t i;
  for (i = 0; i < numAttributes; ++i)
  {
    const char* aName = this->Internal->AttributeNames[i].c_str();
    const char* aValue = this->Internal->AttributeValues[i].c_str();

    // we always print the encoded value. The expat parser processes encoded
    // values when reading them, hence we don't need any decoding when reading
    // the values back.
    const std::string& sanitizedValue = vtkPVXMLElement::Encode(aValue);
    os << " " << (aName ? aName : "NoName") << "=\""
       << (aValue ? sanitizedValue.c_str() : "NoValue") << "\"";
  }
  size_t numberOfNestedElements = this->Internal->NestedElements.size();
  bool hasCdata = !vtkIsSpace(this->Internal->CharacterData);

  bool childlessNode = (numberOfNestedElements == 0) && (hasCdata == false);
  if (childlessNode)
  {
    os << "/>\n";
    return;
  }
  os << ">";
  if (numberOfNestedElements > 0)
  {
    os << "\n";
    for (i = 0; i < numberOfNestedElements; ++i)
    {
      vtkIndent nextIndent = indent.GetNextIndent();
      this->Internal->NestedElements[i]->PrintXML(os, nextIndent);
    }
  }
  if (hasCdata)
  {
    const std::string& encoded = vtkPVXMLElement::Encode(this->Internal->CharacterData.c_str());
    os << encoded.c_str();
    os << "</" << (this->Name ? this->Name : "NoName") << ">\n";
  }
  else
  {
    os << indent << "</" << (this->Name ? this->Name : "NoName") << ">\n";
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
  return static_cast<unsigned int>(this->Internal->NestedElements.size());
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::GetNestedElement(unsigned int index)
{
  if (index < this->Internal->NestedElements.size())
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
  size_t numberOfNestedElements = this->Internal->NestedElements.size();
  size_t i;
  for (i = 0; i < numberOfNestedElements; ++i)
  {
    const char* nid = this->Internal->NestedElements[i]->GetId();
    if (nid && strcmp(nid, id) == 0)
    {
      return this->Internal->NestedElements[i];
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::FindNestedElementByName(const char* name)
{
  vtkPVXMLElementInternals::VectorOfElements::iterator iter =
    this->Internal->NestedElements.begin();
  for (; iter != this->Internal->NestedElements.end(); ++iter)
  {
    const char* cur_name = (*iter)->GetName();
    if (name && cur_name && strcmp(cur_name, name) == 0)
    {
      return (*iter);
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::FindNestedElementByName(const char* name, vtkCollection* elements)
{
  // No more that the current children depth
  this->GetElementsByName(name, elements, false);
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::LookupElementInScope(const char* id)
{
  // Pull off the first qualifier.
  const char* end = id;
  while (*end && (*end != '.'))
    ++end;
  unsigned int len = end - id;
  char* name = new char[len + 1];
  strncpy(name, id, len);
  name[len] = '\0';

  // Find the qualifier in this scope.
  vtkPVXMLElement* next = this->FindNestedElement(name);
  if (next && (*end == '.'))
  {
    // Lookup rest of qualifiers in nested scope.
    next = next->LookupElementInScope(end + 1);
  }

  delete[] name;
  return next;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLElement::LookupElementUpScope(const char* id)
{
  // Pull off the first qualifier.
  const char* end = id;
  while (*end && (*end != '.'))
    ++end;
  unsigned int len = end - id;
  char* name = new char[len + 1];
  strncpy(name, id, len);
  name[len] = '\0';

  // Find most closely nested occurrence of first qualifier.
  vtkPVXMLElement* curScope = this;
  vtkPVXMLElement* start = 0;
  while (curScope && !start)
  {
    start = curScope->FindNestedElement(name);
    curScope = curScope->GetParent();
  }
  if (start && (*end == '.'))
  {
    start = start->LookupElementInScope(end + 1);
  }

  delete[] name;
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
  if (!str || !length)
  {
    return 0;
  }
  std::stringstream vstr;
  vstr << str << ends;
  int i;
  for (i = 0; i < length; ++i)
  {
    vstr >> data[i];
    if (!vstr)
    {
      return i;
    }
  }
  return length;
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length, int* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length, float* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length, double* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}

#if defined(VTK_USE_64BIT_IDS)
//----------------------------------------------------------------------------
int vtkPVXMLElement::GetVectorAttribute(const char* name, int length, vtkIdType* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetAttribute(name), length, data);
}
#endif

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetCharacterDataAsVector(int length, int* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetCharacterData(), length, data);
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetCharacterDataAsVector(int length, float* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetCharacterData(), length, data);
}

//----------------------------------------------------------------------------
int vtkPVXMLElement::GetCharacterDataAsVector(int length, double* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetCharacterData(), length, data);
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::GetElementsByName(const char* name, vtkCollection* elements)
{
  this->GetElementsByName(name, elements, true); // We go as deep as possible
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::GetElementsByName(const char* name, vtkCollection* elements, bool recursively)
{
  if (!elements)
  {
    vtkErrorMacro("elements cannot be NULL.");
    return;
  }
  if (!name)
  {
    vtkErrorMacro("name cannot be NULL.");
    return;
  }

  unsigned int numChildren = this->GetNumberOfNestedElements();
  unsigned int cc;
  for (cc = 0; cc < numChildren; cc++)
  {
    vtkPVXMLElement* child = this->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), name) == 0)
    {
      elements->AddItem(child);
    }
  }

  if (recursively)
  {
    for (cc = 0; cc < numChildren; cc++)
    {
      vtkPVXMLElement* child = this->GetNestedElement(cc);
      if (child)
      {
        child->GetElementsByName(name, elements, recursively);
      }
    }
  }
}

//----------------------------------------------------------------------------
std::string vtkPVXMLElement::Encode(const char* plaintext)
{
  // escape any characters that are not allowed in XML
  std::string sanitized = "";
  if (!plaintext)
  {
    return sanitized;
  }

  const char toescape[] = { '&', '\'', '<', '>', '\"', '\r', '\n', '\t', 0 };

  size_t pt_length = strlen(plaintext);
  sanitized.reserve(pt_length);
  for (size_t cc = 0; cc < pt_length; cc++)
  {
    const char* escape_char = toescape;
    for (; *escape_char != 0; escape_char++)
    {
      if (plaintext[cc] == *escape_char)
      {
        break;
      }
    }

    if (*escape_char)
    {
      char temp[20];
      SNPRINTF(temp, 20, "&#x%x;", static_cast<int>(*escape_char));
      sanitized += temp;
    }
    else
    {
      sanitized += plaintext[cc];
    }
  }

  return sanitized;
}

#if defined(VTK_USE_64BIT_IDS)
//----------------------------------------------------------------------------
int vtkPVXMLElement::GetCharacterDataAsVector(int length, vtkIdType* data)
{
  return vtkPVXMLVectorAttributeParse(this->GetCharacterData(), length, data);
}
#endif

void vtkPVXMLElement::Merge(vtkPVXMLElement* element, const char* attributeName)
{
  if (!element || 0 != strcmp(this->GetName(), element->GetName()))
  {
    return;
  }
  if (attributeName)
  {
    const char* attr1 = this->GetAttribute(attributeName);
    const char* attr2 = element->GetAttribute(attributeName);
    if (attr1 && attr2 && 0 != strcmp(attr1, attr2))
    {
      return;
    }
  }

  // override character data if there is some
  if (!element->Internal->CharacterData.empty())
  {
    this->Internal->CharacterData = element->Internal->CharacterData;
  }

  // add attributes from element to this, or override attribute values on this
  size_t numAttributes = element->Internal->AttributeNames.size();
  size_t numAttributes2 = this->Internal->AttributeNames.size();

  for (size_t i = 0; i < numAttributes; ++i)
  {
    bool found = false;
    for (size_t j = 0; !found && j < numAttributes2; ++j)
    {
      if (element->Internal->AttributeNames[i] == this->Internal->AttributeNames[j])
      {
        this->Internal->AttributeValues[j] = element->Internal->AttributeValues[i];
        found = true;
      }
    }
    // if not found, add it
    if (!found)
    {
      this->AddAttribute(element->Internal->AttributeNames[i].c_str(),
        element->Internal->AttributeValues[i].c_str());
    }
  }

  // now recursively merge the children with the same names

  vtkPVXMLElementInternals::VectorOfElements::iterator iter;
  vtkPVXMLElementInternals::VectorOfElements::iterator iter2;

  for (iter = element->Internal->NestedElements.begin();
       iter != element->Internal->NestedElements.end(); ++iter)
  {
    bool found = false;
    for (iter2 = this->Internal->NestedElements.begin();
         iter2 != this->Internal->NestedElements.end(); ++iter2)
    {
      const char* attr1 = attributeName ? this->GetAttribute(attributeName) : NULL;
      const char* attr2 = attributeName ? element->GetAttribute(attributeName) : NULL;
      if (0 == strcmp((*iter)->Name, (*iter2)->Name) &&
        (!attributeName || (!attr1 || !attr2 || 0 == strcmp(attr1, attr2))))
      {
        (*iter2)->Merge(*iter, attributeName);
        found = true;
      }
    }
    // if not found, add it
    if (!found)
    {
      vtkSmartPointer<vtkPVXMLElement> newElement = vtkSmartPointer<vtkPVXMLElement>::New();
      newElement->SetName((*iter)->GetName());
      newElement->SetId((*iter)->GetId());
      newElement->Internal->AttributeNames = (*iter)->Internal->AttributeNames;
      newElement->Internal->AttributeValues = (*iter)->Internal->AttributeValues;
      this->AddNestedElement(newElement);
      newElement->Merge(*iter, attributeName);
    }
  }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::CopyTo(vtkPVXMLElement* other)
{
  other->SetName(GetName());
  other->SetId(GetId());
  other->Internal->AttributeNames = this->Internal->AttributeNames;
  other->Internal->AttributeValues = this->Internal->AttributeValues;
  other->AddCharacterData(
    this->Internal->CharacterData.c_str(), static_cast<int>(this->Internal->CharacterData.size()));

  // Copy recursively
  vtkPVXMLElementInternals::VectorOfElements::iterator iter;
  for (iter = this->Internal->NestedElements.begin(); iter != this->Internal->NestedElements.end();
       ++iter)
  {
    vtkSmartPointer<vtkPVXMLElement> newElement = vtkSmartPointer<vtkPVXMLElement>::New();
    (*iter)->CopyTo(newElement);
    other->AddNestedElement(newElement);
  }
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::CopyAttributesTo(vtkPVXMLElement* other)
{
  other->SetName(GetName());
  other->SetId(GetId());
  other->Internal->AttributeNames = this->Internal->AttributeNames;
  other->Internal->AttributeValues = this->Internal->AttributeValues;
  other->AddCharacterData(
    this->Internal->CharacterData.c_str(), static_cast<int>(this->Internal->CharacterData.size()));
}

//----------------------------------------------------------------------------
bool vtkPVXMLElement::Equals(vtkPVXMLElement* other)
{
  if (this == other)
  {
    return true;
  }
  if (!other)
  {
    return false;
  }
  std::ostringstream selfstream;
  std::ostringstream otherstream;
  this->PrintXML(selfstream, vtkIndent());
  other->PrintXML(otherstream, vtkIndent());
  return (selfstream.str() == otherstream.str());
}

//----------------------------------------------------------------------------
void vtkPVXMLElement::RemoveAttribute(const char* name)
{
  std::vector<std::string>::iterator nameIterator = this->Internal->AttributeNames.begin();
  std::vector<std::string>::iterator valueIterator = this->Internal->AttributeValues.begin();
  while (nameIterator != this->Internal->AttributeNames.end())
  {
    if (strcmp(nameIterator->c_str(), name) == 0)
    {
      this->Internal->AttributeNames.erase(nameIterator);
      this->Internal->AttributeValues.erase(valueIterator);
      return;
    }
    nameIterator++;
    valueIterator++;
  }
}

//----------------------------------------------------------------------------
