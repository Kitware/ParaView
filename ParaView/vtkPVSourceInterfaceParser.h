/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceInterfaceParser.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

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
