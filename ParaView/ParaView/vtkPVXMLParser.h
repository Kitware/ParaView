/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLParser.h
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
// .NAME vtkPVXMLParser parses ParaView XML configuration files.
// .SECTION Description
// This is a subclass of vtkXMLParser that constructs a representation
// of parsed XML using vtkPVXMLElement.
#ifndef __vtkPVXMLParser_h
#define __vtkPVXMLParser_h

#include "vtkXMLParser.h"

class vtkPVXMLElement;

class VTK_EXPORT vtkPVXMLParser : public vtkXMLParser
{
public:
  vtkTypeRevisionMacro(vtkPVXMLParser,vtkXMLParser);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVXMLParser* New();
  
  // Description:
  // Write the parsed XML into the output stream.
  void PrintXML(ostream& os);
  
  // Description:
  // Get the root element from the XML document.
  vtkPVXMLElement* GetRootElement();
  
  // Description:
  // Get/Set the file from which to read the configuration.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  // Description:
  // Parse the configuration file.  Returns 1 for success, 0 otherwise.
  int Parse();
  
  // Description:
  // Parse the given string.  Returns 1 for success, 0 otherwise.
  int Parse(const char* input);
  
protected:
  vtkPVXMLParser();
  ~vtkPVXMLParser();
  
  // Override parsing driver.
  int ParseXML();
  
  void StartElement(const char* name, const char** atts);
  void EndElement(const char* name);
  
  void AddElement(vtkPVXMLElement* element);
  void PushOpenElement(vtkPVXMLElement* element);
  vtkPVXMLElement* PopOpenElement();
  
  // The root XML element.
  vtkPVXMLElement* RootElement;
  
  // The stack of elements currently being parsed.
  vtkPVXMLElement** OpenElements;
  unsigned int NumberOfOpenElements;
  unsigned int OpenElementsSize;
  
  // Counter to assign unique element ids to those that don't have any.
  unsigned int ElementIdIndex;  
  
  // The name of the input file.
  char* FileName;
  
  // The string currently getting parsed if not from a file.
  const char* InputString;
  
private:
  vtkPVXMLParser(const vtkPVXMLParser&);  // Not implemented.
  void operator=(const vtkPVXMLParser&);  // Not implemented.
};

#endif
