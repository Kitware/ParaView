/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLElement.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVXMLElement represents an XML element and those nested inside.
// .SECTION Description
// This is used by vtkPVXMLParser to represent an XML document starting
// at the root element.
#ifndef __vtkPVXMLElement_h
#define __vtkPVXMLElement_h

#include "vtkObject.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkCollection;
class vtkPVXMLParser;

//BTX
struct vtkPVXMLElementInternals;
//ETX

class VTK_EXPORT vtkPVXMLElement : public vtkObject
{
public:
  vtkTypeMacro(vtkPVXMLElement,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVXMLElement* New();

  // Description:
  // Set/Get the name of the element.  This is its XML tag.
  // (<Name />).
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);

  // Description:
  // Get the id of the element. This is assigned by the XML parser
  // and can be used as an identifier to an element.
  vtkGetStringMacro(Id);

  // Description:
  // Get the attribute with the given name.  If it doesn't exist,
  // returns 0. 
  const char* GetAttribute(const char* name);

  // Description:
  // Get the character data for the element.
  const char* GetCharacterData();

  // Description:
  // Get the attribute with the given name converted to a scalar
  // value.  Returns whether value was extracted.
  int GetScalarAttribute(const char* name, int* value);
  int GetScalarAttribute(const char* name, float* value);
  int GetScalarAttribute(const char* name, double* value);
#if defined(VTK_USE_64BIT_IDS)
  int GetScalarAttribute(const char* name, vtkIdType* value);
#endif

  // Description:
  // Get the attribute with the given name converted to a scalar
  // value.  Returns length of vector read.
  int GetVectorAttribute(const char* name, int length, int* value);
  int GetVectorAttribute(const char* name, int length, float* value);
  int GetVectorAttribute(const char* name, int length, double* value);
#if defined(VTK_USE_64BIT_IDS)
  int GetVectorAttribute(const char* name, int length, vtkIdType* value);
#endif

  // Description:
  // Get the character data converted to a scalar
  // value.  Returns length of vector read.
  int GetCharacterDataAsVector(int length, int* value);
  int GetCharacterDataAsVector(int length, float* value);
  int GetCharacterDataAsVector(int length, double* value);
#if defined(VTK_USE_64BIT_IDS)
  int GetCharacterDataAsVector(int length, vtkIdType* value);
#endif

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
  // Not that this searches only the immediate children of this
  // vtkPVXMLElement.
  vtkPVXMLElement* FindNestedElement(const char* id);

  // Description:
  // Locate a nested element with the given tag name.
  vtkPVXMLElement* FindNestedElementByName(const char* name);

  // Description:
  // Removes all nested elements.
  void RemoveAllNestedElements();

  // Description:
  // Remove a particular element.
  void RemoveNestedElement(vtkPVXMLElement*);

  // Description:
  // Lookup the element with the given id, starting at this scope.
  vtkPVXMLElement* LookupElement(const char* id);

  // Description:
  // Given it's name and value, add an attribute.
  void AddAttribute(const char* attrName, const char* attrValue);
  void AddAttribute(const char* attrName, unsigned int attrValue);
  void AddAttribute(const char* attrName, double attrValue);
  void AddAttribute(const char* attrName, double attrValue, int precision);
  void AddAttribute(const char* attrName, int attrValue);
#if defined(VTK_USE_64BIT_IDS)
  void AddAttribute(const char* attrName, vtkIdType attrValue);
#endif

  // Description:
  // Remove the attribute from the current element
  void RemoveAttribute(const char* attrName);

  // Description:
  // Given it's name and value, set an attribute.
  // If an attribute with the given name already exists,
  // it replaces the old attribute.
  // chars that need to be XML escaped will be done so internally
  // for example " will be converted to &quot;
  void SetAttribute(const char* attrName, const char* attrValue);

  // Description:
  // Add a sub-element. The parent element keeps a reference to
  // sub-element. If setParent is true, the nested element's parent
  // is set as this.
  void AddNestedElement(vtkPVXMLElement* element, int setPrent);
  void AddNestedElement(vtkPVXMLElement* element);

  // Description:
  // Serialize (as XML) in the given stream.
  void PrintXML(ostream& os, vtkIndent indent);
  void PrintXML();

  // Description:
  // Merges another element with this one, both having the same name.
  // If any attribute, character data or nested element exists in both, 
  // the passed in one will override this one's.  If they don't exist,
  // they'll be added.  If nested elements have the same names, the
  // optional attributeName maybe passed in as another criteria to determine
  // what to merge in case of same names.
  void Merge(vtkPVXMLElement* element, const char* attributeName);

  // Description:
  // Similar to DOM sepecific getElementsByTagName(). 
  // Returns a list of vtkPVXMLElements with the given name in the order
  // in which they will be encountered in a preorder traversal
  // of the sub-tree under this node. The elements are populated
  // in the vtkCollection passed as an argument.
  void GetElementsByName(const char* name, vtkCollection* elements);

  // Description:
  // Encode a string. 
  static vtkStdString Encode(const char* plaintext);

  // Description:
  // Return true if the current object has the same content as the other.
  // The comparison implementation is pretty weak in the mean that
  // we compare resulting XML string.
  bool Equals(vtkPVXMLElement* other);

  // Description:
  // Copy the current XML element content into the provided one
  void Copy(vtkPVXMLElement* other);

protected:
  vtkPVXMLElement();
  ~vtkPVXMLElement();

  vtkPVXMLElementInternals* Internal;

  char* Name;
  char* Id;

  // The parent of this element.
  vtkPVXMLElement* Parent;

  // Method used by vtkPVXMLParser to setup the element.
  vtkSetStringMacro(Id);
  void ReadXMLAttributes(const char** atts);
  void AddCharacterData(const char* data, int length);


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
