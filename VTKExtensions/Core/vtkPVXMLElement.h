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
/**
 * @class   vtkPVXMLElement
 *
 * This is used by vtkPVXMLParser to represent an XML document starting
 * at the root element.
*/

#ifndef vtkPVXMLElement_h
#define vtkPVXMLElement_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsCoreModule.h" // needed for export macro

#include <string> // for std::string

class vtkCollection;
class vtkPVXMLParser;

struct vtkPVXMLElementInternals;

class VTKPVVTKEXTENSIONSCORE_EXPORT vtkPVXMLElement : public vtkObject
{
public:
  vtkTypeMacro(vtkPVXMLElement, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPVXMLElement* New();

  //@{
  /**
   * Set/Get the name of the element.  This is its XML tag.
   * (\c \<Name/\>).
   */
  vtkSetStringMacro(Name);
  vtkGetStringMacro(Name);
  //@}

  //@{
  /**
   * Get the id of the element. This is assigned by the XML parser
   * and can be used as an identifier to an element.
   */
  vtkGetStringMacro(Id);
  //@}

  /**
   * Get the attribute with the given name.  If it doesn't exist,
   * returns NULL.
   */
  const char* GetAttribute(const char* name) { return this->GetAttributeOrDefault(name, NULL); }

  /**
   * Get the attribute with the given name.  If it doesn't exist,
   * returns "".
   */
  const char* GetAttributeOrEmpty(const char* name)
  {
    return this->GetAttributeOrDefault(name, "");
  }

  /**
   * Get the attribute with the given name.
   * If it doesn't exist, returns the provided notFound value.
   */
  const char* GetAttributeOrDefault(const char* name, const char* notFound);

  /**
   * Get the character data for the element.
   */
  const char* GetCharacterData();

  //@{
  /**
   * Get the attribute with the given name converted to a scalar
   * value.  Returns whether value was extracted.
   */
  int GetScalarAttribute(const char* name, int* value);
  int GetScalarAttribute(const char* name, float* value);
  int GetScalarAttribute(const char* name, double* value);
#if defined(VTK_USE_64BIT_IDS)
  int GetScalarAttribute(const char* name, vtkIdType* value);
#endif
  //@}

  //@{
  /**
   * Get the attribute with the given name converted to a scalar
   * value.  Returns length of vector read.
   */
  int GetVectorAttribute(const char* name, int length, int* value);
  int GetVectorAttribute(const char* name, int length, float* value);
  int GetVectorAttribute(const char* name, int length, double* value);
#if defined(VTK_USE_64BIT_IDS)
  int GetVectorAttribute(const char* name, int length, vtkIdType* value);
#endif
  //@}

  //@{
  /**
   * Get the character data converted to a scalar
   * value.  Returns length of vector read.
   */
  int GetCharacterDataAsVector(int length, int* value);
  int GetCharacterDataAsVector(int length, float* value);
  int GetCharacterDataAsVector(int length, double* value);
#if defined(VTK_USE_64BIT_IDS)
  int GetCharacterDataAsVector(int length, vtkIdType* value);
#endif
  //@}

  /**
   * Get the parent of this element.
   */
  vtkPVXMLElement* GetParent();

  /**
   * Get the number of elements nested in this one.
   */
  unsigned int GetNumberOfNestedElements();

  /**
   * Get the element nested in this one at the given index.
   */
  vtkPVXMLElement* GetNestedElement(unsigned int index);

  /**
   * Find a nested element with the given id.
   * Not that this searches only the immediate children of this
   * vtkPVXMLElement.
   */
  vtkPVXMLElement* FindNestedElement(const char* id);

  /**
   * Locate a nested element with the given tag name.
   */
  vtkPVXMLElement* FindNestedElementByName(const char* name);

  /**
   * Locate a set of nested elements with the given tag name.
   */
  void FindNestedElementByName(const char* name, vtkCollection* elements);

  /**
   * Removes all nested elements.
   */
  void RemoveAllNestedElements();

  /**
   * Remove a particular element.
   */
  void RemoveNestedElement(vtkPVXMLElement*);

  /**
   * Replace a particular element with another
   */
  void ReplaceNestedElement(vtkPVXMLElement* elementToReplace, vtkPVXMLElement* element);

  /**
   * Lookup the element with the given id, starting at this scope.
   */
  vtkPVXMLElement* LookupElement(const char* id);

  //@{
  /**
   * Given it's name and value, add an attribute.
   */
  void AddAttribute(const char* attrName, const char* attrValue);
  void AddAttribute(const char* attrName, unsigned int attrValue);
  void AddAttribute(const char* attrName, double attrValue);
  void AddAttribute(const char* attrName, double attrValue, int precision);
  void AddAttribute(const char* attrName, int attrValue);
#if defined(VTK_USE_64BIT_IDS)
  void AddAttribute(const char* attrName, vtkIdType attrValue);
#endif
  //@}

  /**
   * Remove the attribute from the current element
   */
  void RemoveAttribute(const char* attrName);

  /**
   * Given it's name and value, set an attribute.
   * If an attribute with the given name already exists,
   * it replaces the old attribute.
   * chars that need to be XML escaped will be done so internally
   * for example " will be converted to &quot;
   */
  void SetAttribute(const char* attrName, const char* attrValue);

  //@{
  /**
   * Add a sub-element. The parent element keeps a reference to
   * sub-element. If setParent is true, the nested element's parent
   * is set as this.
   */
  void AddNestedElement(vtkPVXMLElement* element, int setPrent);
  void AddNestedElement(vtkPVXMLElement* element);
  //@}

  //@{
  /**
   * Serialize (as XML) in the given stream.
   */
  void PrintXML(ostream& os, vtkIndent indent);
  void PrintXML();
  //@}

  /**
   * Merges another element with this one, both having the same name.
   * If any attribute, character data or nested element exists in both,
   * the passed in one will override this one's.  If they don't exist,
   * they'll be added.  If nested elements have the same names, the
   * optional attributeName maybe passed in as another criteria to determine
   * what to merge in case of same names.
   */
  void Merge(vtkPVXMLElement* element, const char* attributeName);

  //@{
  /**
   * Similar to DOM specific getElementsByTagName().
   * Returns a list of vtkPVXMLElements with the given name in the order
   * in which they will be encountered in a preorder traversal
   * of the sub-tree under this node. The elements are populated
   * in the vtkCollection passed as an argument.
   */
  void GetElementsByName(const char* name, vtkCollection* elements);
  void GetElementsByName(const char* name, vtkCollection* elements, bool recursively);
  //@}

  /**
   * Encode a string.
   */
  static std::string Encode(const char* plaintext);

  /**
   * Return true if the current object has the same content as the other.
   * The comparison implementation is pretty weak in the mean that
   * we compare resulting XML string.
   */
  bool Equals(vtkPVXMLElement* other);

  /**
   * Copy the current XML element content into the provided one
   */
  void CopyTo(vtkPVXMLElement* other);

  /**
   * Copy the attributes from current XML element content into the provided one.
   */
  void CopyAttributesTo(vtkPVXMLElement* other);

protected:
  vtkPVXMLElement();
  ~vtkPVXMLElement() override;

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

  friend class vtkPVXMLParser;

private:
  vtkPVXMLElement(const vtkPVXMLElement&) = delete;
  void operator=(const vtkPVXMLElement&) = delete;
};

#endif
