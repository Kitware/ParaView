/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceInterfaceParser.h
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
// .NAME vtkPVSourceInterfaceParser
// .SECTION Description
// This class encapsulates the parsing of the XML description of the
// interfaces.

#ifndef __vtkPVSourceInterfaceParser_h
#define __vtkPVSourceInterfaceParser_h

#include "vtkObject.h"
#include "vtkPVWindow.h"
#include "vtkPVApplication.h"
#include "vtkCollection.h"

class vtkPVSourceInterface;
class vtkPVMethodInterface;

class VTK_EXPORT vtkPVSourceInterfaceParser : public vtkObject
{
public:
  static vtkPVSourceInterfaceParser* New();
  vtkTypeMacro(vtkPVSourceInterfaceParser,vtkObject);
  
  // Description:
  // Parse the XML from the given input file.  Returns success or
  // failure.
  int ParseFile();

  // Description:
  // Parse the XML from the given input string.  The input must be
  // complete XML code.  Parse the XML from the given input file.
  // Returns success or failure.
  int ParseString(const char*);
  
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  
  vtkSetObjectMacro(PVWindow, vtkPVWindow);
  vtkGetObjectMacro(PVWindow, vtkPVWindow);
  
  vtkSetObjectMacro(PVApplication, vtkPVApplication);
  vtkGetObjectMacro(PVApplication, vtkPVApplication);
  
  vtkSetObjectMacro(SourceInterfaces, vtkCollection);
  vtkGetObjectMacro(SourceInterfaces, vtkCollection);
protected:
  vtkPVSourceInterfaceParser();
  ~vtkPVSourceInterfaceParser();
  vtkPVSourceInterfaceParser(const vtkPVSourceInterfaceParser&);
  void operator=(const vtkPVSourceInterfaceParser&);

  // Begin element handler that is registered with the XML_Parser.
  static void BeginElementFunction(void*, const char *, const char **);
  
  // End element handler that is registered with the XML_Parser.
  static void EndElementFunction(void*, const char *);

  // Called when a new element is opened in the XML source.  Checks the
  // tag name, and calls the appropriate handler with an Attributes
  // container.
  void BeginElement(const char *, const char **);
  
  // Called at the end of an element in the XML source opened when
  // BeginElement was called.
  void EndElement(const char *);

  // Send the given buffer to the XML parser.
  int Parse(const char*, unsigned int);
  
  // Inform the XML parser of end-of-input.
  int FinishParsing();
  
  // Called by begin handlers to report any stray attribute values.
  void ReportStrayAttribute(const char*, const char*, const char*);
  
  // Called by begin handlers to report any missing attribute values.
  void ReportMissingAttribute(const char*, const char*);
  
  // Called by begin handlers to report bad attribute values.
  void ReportBadAttribute(const char*, const char*, const char*);
  
  // Called by BeginElement to report unknown element type.
  void ReportUnknownElement(const char*);
  
  // Called by BeginElement to report stray element type.
  void ReportStrayElement(const char*);

  // Called by Parse to report an XML syntax error.
  void ReportXmlParseError();
  
  // Begin handlers for each element type.  
  void SourceElementBegin(const char**);
  void FilterElementBegin(const char**);
  void BooleanElementBegin(const char**);
  void ScalarElementBegin(const char**);
  void VectorElementBegin(const char**);
  void StringElementBegin(const char**);
  void FileElementBegin(const char**);
  void SelectionElementBegin(const char**);
  void ChoiceElementBegin(const char**);
  void ExtentElementBegin(const char**);

  // End handlers for each element type.  
  void SourceElementEnd();
  void FilterElementEnd();
  void BooleanElementEnd();
  void ScalarElementEnd();
  void VectorElementEnd();
  void StringElementEnd();
  void FileElementEnd();
  void SelectionElementEnd();
  void ChoiceElementEnd();
  void ExtentElementEnd();

  // Called by begin handlers to setup "name", "set", "get", and "help".
  void SetStandardMethodInterface(const char*, const char*, const char*,
                                  const char*, const char*);
  
  // The actual XML_Parser.
  void* Parser;
  
  // The name of the file to parse.
  char* FileName;
  
  // The vtkPVWindow using this parser.
  vtkPVWindow* PVWindow;
  
  // The vtkPVApplication containing the window.
  vtkPVApplication* PVApplication;
  
  // The vtkCollection of source interfaces to fill while parsing.
  vtkCollection* SourceInterfaces;
  
  // The source or filter currently being parsed.
  vtkPVSourceInterface *PVSourceInterface;
  
  // The variable currently being parsed.
  vtkPVMethodInterface *PVMethodInterface;
};


#endif
