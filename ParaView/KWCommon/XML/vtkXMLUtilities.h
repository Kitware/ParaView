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
// .NAME vtkXMLUtilities - XML utilities.
// .SECTION Description
// vtkXMLUtilities provides XML-related convenience functions.
// .SECTION See Also
// vtkXMLDataElement

#ifndef __vtkXMLUtilities_h
#define __vtkXMLUtilities_h

#include "vtkObject.h"

class vtkXMLDataElement;

//BTX
template<class DataType> class vtkVector;
template<class DataType> class vtkVectorIterator;
//ETX

class VTK_EXPORT vtkXMLUtilities : public vtkObject
{
public:
  static vtkXMLUtilities* New();
  vtkTypeRevisionMacro(vtkXMLUtilities, vtkObject);

  // Description:
  // Output a string to a stream, replace special chars with the corresponding
  // character entities.
  static void ConvertSpecialChars(const char*, ostream&);

  // Description:
  // Collate a vtkXMLDataElement's attributes to a stream as a series of
  // name="value" pairs (the separator between each pair can be specified,
  // if not, it defaults to a space).
  static void CollateAttributes(vtkXMLDataElement*, 
                                ostream&, 
                                const char *sep = 0);

  // Description:
  // Flatten a vtkXMLDataElement to a stream, i.e. output an XML stream
  // of characters corresponding to that element, its attributes and its
  // nested elements.
  // If 'indent' is not NULL, it is used to indent the whole tree.
  // If 'indent' is not NULL and 'indent_attributes' is true, attributes will 
  // be indented as well.
  static void FlattenElement(vtkXMLDataElement*, 
                             ostream&, 
                             vtkIndent *indent = 0,
                             int indent_attributes = 1);

  // Description:
  // Write a vtkXMLDataElement to a file
  // Return 1 on success, 0 otherwise.
  static int WriteElement(vtkXMLDataElement*, 
                          const char *filename, 
                          vtkIndent *indent = 0);

  // Description:
  // Read a vtkXMLDataElement from a file or stream
  // Return the root element on success, NULL otherwise.
  // Note that you have to call Delete() on the element returned by that
  // function to ensure it is freed properly.
  //BTX
  static vtkXMLDataElement* ReadElementFromStream(istream&);
  static vtkXMLDataElement* ReadElementFromString(const char *str);
  static vtkXMLDataElement* ReadElementFromFile(const char *filename);
  //ETX

  // Description:
  // Find all elements in 'tree' that are similar to 'elem' (using the
  // vtkXMLDataElement::IsEqualTo() predicate). 
  // Return the number of elements found and store those elements in
  // 'results' (automatically allocated).
  // Warning: the results do not include 'elem' if it was found in the tree ;
  // do not forget to deallocate 'results' if something was found.
  //BTX
  static int FindSimilarElements(vtkXMLDataElement *elem, 
                                 vtkXMLDataElement *tree, 
                                 vtkXMLDataElement ***results);
  //ETX

  // Description:
  // Factor and unfactor a tree. This operation looks for duplicate elements
  // in the tree, and replace them with references to a pool of elements.
  // Unfactoring a non-factored element is harmless.
  static void FactorElements(vtkXMLDataElement *tree);
  static void UnFactorElements(vtkXMLDataElement *tree);

protected:  
  vtkXMLUtilities() {};
  ~vtkXMLUtilities() {};

  //BTX
  typedef vtkVector<vtkXMLDataElement*> DataElementContainer;
  typedef vtkVectorIterator<vtkXMLDataElement*> DataElementContainerIterator;

  static void FindSimilarElementsInternal(vtkXMLDataElement *elem, 
                                          vtkXMLDataElement *tree, 
                                          DataElementContainer *results);

  static int FactorElementsInternal(vtkXMLDataElement *tree, 
                                    vtkXMLDataElement *root, 
                                    vtkXMLDataElement *pool);
  static int UnFactorElementsInternal(vtkXMLDataElement *tree, 
                                      vtkXMLDataElement *pool);
  //ETX

private:
  vtkXMLUtilities(const vtkXMLUtilities&); // Not implemented
  void operator=(const vtkXMLUtilities&); // Not implemented    
};

#endif


