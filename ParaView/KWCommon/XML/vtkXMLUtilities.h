/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUtilities.h
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
// .NAME vtkXMLUtilities - XML utilities.
// .SECTION Description
// vtkXMLUtilities provides XML-related convenience functions.
// .SECTION See Also
// vtkXMLDataElement

#ifndef __vtkXMLUtilities_h
#define __vtkXMLUtilities_h

#include "vtkObject.h"

class vtkXMLDataElement;

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
  // it defaults to a space).
  static void CollateAttributes(vtkXMLDataElement*, 
                                ostream&, 
                                const char *sep = 0);

  // Description:
  // Flatten a vtkXMLDataElement to a stream, i.e. output an XML stream
  // of characters corresponding to that element, its attributes and its
  // nested elements.
  static void FlattenElement(vtkXMLDataElement*, 
                             ostream&, 
                             vtkIndent *indent = 0);

  // Description:
  // Write a vtkXMLDataElement to a file
  // Return 1 on success, 0 otherwise.
  static void WriteElement(vtkXMLDataElement*, 
                           const char *filename);

  // Description:
  // Read a vtkXMLDataElement from a file
  // Return the root element on success, NULL otherwise.
  // Note that you have to call Delete() on the element returned by that
  // function to ensure it is freed properly.
  //BTX
  static vtkXMLDataElement* ReadElement(const char *filename);
  //ETX

protected:  
  vtkXMLUtilities() {};
  ~vtkXMLUtilities() {};

private:
  vtkXMLUtilities(const vtkXMLUtilities&); // Not implemented
  void operator=(const vtkXMLUtilities&); // Not implemented    
};

#endif
