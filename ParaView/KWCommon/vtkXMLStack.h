/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLStack.h
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
// .NAME vtkXMLStack - am XML stack.
// .SECTION Description
// vtkXMLStack provides a simple stack to keep track of elements.
// .SECTION See Also
// vtkXMLParser

#ifndef __vtkXMLStack_h
#define __vtkXMLStack_h

#include "vtkObject.h"

//BTX
template<class DataType> class vtkVector;
template<class DataType> class vtkVectorIterator;
//ETX

class VTK_EXPORT vtkXMLStack : public vtkObject
{
public:
  static vtkXMLStack* New();
  vtkTypeRevisionMacro(vtkXMLStack, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Push/Pop element.
  // Return VTK_OK on success.
  virtual int Push(const char *name);
  virtual int Pop();

  // Description:
  // Get the name of the current (and previous) element on top of stack
  // Return NULL on error.
  virtual char* GetCurrentElementName();
  virtual char* GetPreviousElementName();

  // Description:
  // Get/Set the attributes of the current element on top of stack
  // If passed a char**, it assumes a NULL terminated list of 
  // attribute/value pairs (and return a collated string).
  // Return NULL on error.
  virtual char* GetCurrentElementAttributes();
  virtual char* SetCurrentElementAttributes(const char **args);

  // Description:
  // Get the complete current element (i.e. its XML form constructed from
  // its name and attributes).
  virtual char* GetCompleteCurrentElement();

  // Description:
  // Remove all elements.
  virtual void RemoveAllElements();

  // Description:
  // Convenience method to collate a list of attributes into a single string
  static void CollateAttributes(const char **args, ostream &os);

protected:  
  vtkXMLStack();
  ~vtkXMLStack();

  //BTX

  class ElementSlot
  {
  public:

    ElementSlot();
    ~ElementSlot();

    char* GetName() { return this->Name; }
    void SetName(const char *);
    char* GetAttributes() { return this->Attributes; }
    void SetAttributes(const char *);

  protected:

    char *Name;
    char *Attributes;
  };

  typedef vtkVector<ElementSlot*> ElementsContainer;
  typedef vtkVectorIterator<ElementSlot*> ElementsContainerIterator;
  ElementsContainer *Elements;

  ElementSlot* GetElement(int id);
  ElementSlot* GetCurrentElement();

  //ETX

  // A buffer zone to store the complete current element

  vtkSetStringMacro(CompleteCurrentElement);
  char *CompleteCurrentElement;

private:
  vtkXMLStack(const vtkXMLStack&); // Not implemented
  void operator=(const vtkXMLStack&); // Not implemented    
};

#endif
