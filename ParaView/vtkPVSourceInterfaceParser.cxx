/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceInterfaceParser.cxx
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
#include "vtkPVSourceInterfaceParser.h"
#include "vtkPVSourceInterface.h"

#include "vtkObjectFactory.h"

#include "xmlparse.h"

//----------------------------------------------------------------------------
vtkPVSourceInterfaceParser* vtkPVSourceInterfaceParser::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSourceInterfaceParser");
  if(ret)
    {
    return (vtkPVSourceInterfaceParser*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSourceInterfaceParser;
}

//----------------------------------------------------------------------------
vtkPVSourceInterfaceParser::vtkPVSourceInterfaceParser()
{  
  this->FileName = NULL;
  this->PVWindow = NULL;
  this->PVApplication = NULL;
  this->SourceInterfaces = NULL;
  
  this->PVSourceInterface = NULL;
  this->PVMethodInterface = NULL;
  
  this->Parser = XML_ParserCreate(NULL);
  XML_SetElementHandler(this->Parser,
                        &vtkPVSourceInterfaceParser::BeginElementFunction,
                        &vtkPVSourceInterfaceParser::EndElementFunction);
  XML_SetUserData(this->Parser, this);
}

//----------------------------------------------------------------------------
vtkPVSourceInterfaceParser::~vtkPVSourceInterfaceParser()
{
  this->SetFileName(NULL);
  this->SetPVWindow(NULL);
  this->SetPVApplication(NULL);
  this->SetSourceInterfaces(NULL);
  XML_ParserFree(this->Parser);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::BeginElementFunction(void* parser,
                                                      const char *name,
                                                      const char **atts)
{
  static_cast<vtkPVSourceInterfaceParser*>(parser)->BeginElement(name, atts);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::EndElementFunction(void* parser,
                                                    const char *name)
{
  static_cast<vtkPVSourceInterfaceParser*>(parser)->EndElement(name);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::BeginElement(const char *name,
                                              const char **atts)
{
  if(strcmp(name, "Source") == 0) { this->SourceElementBegin(atts); }
  else if(strcmp(name, "Filter") == 0) { this->FilterElementBegin(atts); }
  else if(strcmp(name, "Boolean") == 0) { this->BooleanElementBegin(atts); }
  else if(strcmp(name, "Scalar") == 0) { this->ScalarElementBegin(atts); }
  else if(strcmp(name, "Vector") == 0) { this->VectorElementBegin(atts); }
  else if(strcmp(name, "String") == 0) { this->StringElementBegin(atts); }
  else if(strcmp(name, "File") == 0)   { this->FileElementBegin(atts); }
  else if(strcmp(name, "Selection") == 0) { this->SelectionElementBegin(atts); }
  else if(strcmp(name, "Choice") == 0) { this->ChoiceElementBegin(atts);}
  else if(strcmp(name, "Extent") == 0) { this->ExtentElementBegin(atts); }
  else if(strcmp(name, "Interfaces") == 0) {}
  else { this->ReportUnknownElement(name); }
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::EndElement(const char *name)
{
  if(strcmp(name, "Source") == 0) { this->SourceElementEnd(); }
  else if(strcmp(name, "Filter") == 0) { this->FilterElementEnd(); }
  else if(strcmp(name, "Boolean") == 0) { this->BooleanElementEnd(); }
  else if(strcmp(name, "Scalar") == 0) { this->ScalarElementEnd(); }
  else if(strcmp(name, "Vector") == 0) { this->VectorElementEnd(); }
  else if(strcmp(name, "String") == 0) { this->StringElementEnd(); }
  else if(strcmp(name, "File") == 0)   { this->FileElementEnd(); }
  else if(strcmp(name, "Selection") == 0) { this->SelectionElementEnd(); }
  else if(strcmp(name, "Choice") == 0) { this->ChoiceElementEnd();}
  else if(strcmp(name, "Extent") == 0) { this->ExtentElementEnd(); }
  else if(strcmp(name, "Interfaces") == 0) {}
  else { /* Unknown attribute reported by begin handler. */ }
}

//----------------------------------------------------------------------------
int vtkPVSourceInterfaceParser::Parse(const char* buffer, unsigned int count)
{
  if(!XML_Parse(this->Parser, buffer, count, 0))
    {
    this->ReportXmlParseError();
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkPVSourceInterfaceParser::FinishParsing()
{
  if(!XML_Parse(this->Parser, "", 0, 1))
    {
    this->ReportXmlParseError();
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ReportStrayAttribute(const char* element,
                                                      const char* attr,
                                                      const char* value)
{
  vtkWarningMacro("Stray attribute in Interfaces file " << this->FileName
                  << ": Element " << element << " has " << attr << "=\""
                  << value << "\"");
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ReportMissingAttribute(const char* element,
                                                        const char* attr)
{
  vtkErrorMacro("Missing attribute in Interfaces file " << this->FileName
                  << ": Element " << element << " is missing " << attr);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ReportBadAttribute(const char* element,
                                                    const char* attr,
                                                    const char* value)
{
  vtkErrorMacro("Bad attribute value in Interfaces file " << this->FileName
                  << ": Element " << element << " has " << attr << "=\""
                  << value << "\"");
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ReportUnknownElement(const char* element)
{
  vtkErrorMacro("Unknown XML element in Interfaces file "
                << this->FileName << ": " << element);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ReportXmlParseError()
{
  vtkErrorMacro("Error parsing XML in Interfaces file "
                << this->FileName << " at line "
                << XML_GetCurrentLineNumber(this->Parser)
                << ": " << XML_ErrorString(XML_GetErrorCode(this->Parser)));
}

//----------------------------------------------------------------------------
int vtkPVSourceInterfaceParser::ParseFile()
{
  const int buffer_length = 4096;
  // The buffer needs one extra character so we can safely add a
  // newline when one was not stored.
  char buffer[buffer_length+1];
  ifstream fin(this->FileName, ios::in);
  if(!fin)
    {
    vtkErrorMacro("Error opening XML Interfaces file " << this->FileName);
    return 0;
    }
  
  // Read in the file and send its contents to the XML parser.
  while(fin.getline(buffer, buffer_length, '\n') || fin.gcount())
    {
    unsigned long count = fin.gcount();
    if(fin.eof())
      {
      // Final line, but with no newline.
      if(!this->Parse(buffer, count)) { return 0; }
      }
    else if(fin.fail())
      {
      // Part of a line longer than our buffer, clear the fail bit of
      // the stream so that we can continue.
      fin.clear(fin.rdstate() & ~ios::failbit);
      this->Parse(buffer, count);
      if(!this->Parse(buffer, count)) { return 0; }
      }
    else
      {
      // Line on which a newline was encountered.  It was read from
      // the stream, but not stored.  Store the newline ourself.
      buffer[count-1] = '\n';
      if(!this->Parse(buffer, count)) { return 0; }
      }
    }
  
  // Must tell the XML parser about the end-of-input.
  return this->FinishParsing();
}

//----------------------------------------------------------------------------
int vtkPVSourceInterfaceParser::ParseString(const char* str)
{
  if(!this->Parse(str, strlen(str))) { return 0; }
  return this->FinishParsing();
}
  
//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::SourceElementBegin(const char** atts)
{
  this->PVSourceInterface = vtkPVSourceInterface::New();
  this->PVSourceInterface->SetApplication(this->PVApplication);
  this->PVSourceInterface->SetPVWindow(this->PVWindow);
  
  const char* the_class=0;
  const char* root=0;
  const char* output=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "class") == 0) { the_class = atts[i+1]; }
    else if(strcmp(atts[i], "root") == 0) { root = atts[i+1]; }
    else if(strcmp(atts[i], "output") == 0) { output = atts[i+1]; }
    else { this->ReportStrayAttribute("Source", atts[i], atts[i+1]); }
    }
  
  if(the_class) { this->PVSourceInterface->SetSourceClassName(the_class); }
  else { this->ReportMissingAttribute("Source", "class"); }
  
  if(root) { this->PVSourceInterface->SetRootName(root); }
  else { this->ReportMissingAttribute("Source", "root"); }
  
  if(output) { this->PVSourceInterface->SetOutputClassName(output); }
  else { this->ReportMissingAttribute("Source", "output"); }
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::FilterElementBegin(const char** atts)
{
  this->PVSourceInterface = vtkPVSourceInterface::New();
  this->PVSourceInterface->SetApplication(this->PVApplication);
  this->PVSourceInterface->SetPVWindow(this->PVWindow);
  
  const char* the_class=0;
  const char* root=0;
  const char* input=0;
  const char* output=0;
  const char* the_default=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "class") == 0) { the_class = atts[i+1]; }
    else if(strcmp(atts[i], "root") == 0) { root = atts[i+1]; }
    else if(strcmp(atts[i], "input") == 0) { input = atts[i+1]; }
    else if(strcmp(atts[i], "output") == 0) { output = atts[i+1]; }
    else if(strcmp(atts[i], "default") == 0) { the_default = atts[i+1]; }
    else { this->ReportStrayAttribute("Source", atts[i], atts[i+1]); }
    }
  
  if(the_class) { this->PVSourceInterface->SetSourceClassName(the_class); }
  else { this->ReportMissingAttribute("Filter", "class"); }
  
  if(root) { this->PVSourceInterface->SetRootName(root); }
  else { this->ReportMissingAttribute("Filter", "root"); }
  
  if(input) { this->PVSourceInterface->SetInputClassName(input); }
  else { this->ReportMissingAttribute("Filter", "input"); }

  if(output) { this->PVSourceInterface->SetOutputClassName(output); }
  else { this->ReportMissingAttribute("Filter", "output"); }

  if(the_default)
    {
    if(strcmp(the_default, "scalars") == 0)
      { this->PVSourceInterface->DefaultScalarsOn(); }    
    else if(strcmp(the_default, "vectors") == 0)
      { this->PVSourceInterface->DefaultVectorsOn(); }
    else
      { this->ReportBadAttribute("Filter", "default", the_default); }
    }
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::BooleanElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0; 
  const char* help=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("Boolean", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("Boolean", name, set, get, help);
  this->PVMethodInterface->SetWidgetTypeToToggle();
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ScalarElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0;
  const char* type=0;
  const char* help=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "type") == 0) { type = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("Scalar", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("Scalar", name, set, get, help);

  if(type)
    {
    if(strcmp(type, "int") == 0)
      { this->PVMethodInterface->AddIntegerArgument(); }
    else if(strcmp(type, "float") == 0)
      { this->PVMethodInterface->AddFloatArgument(); }
    else
      { this->ReportBadAttribute("Scalar", "type", type); }
    }
  else
    {
    this->ReportMissingAttribute("Scalar", "type");
    }  
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::VectorElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0;
  const char* type=0;
  const char* help=0;
  const char* length_s=0;
  int length=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "type") == 0) { type = atts[i+1]; }
    else if(strcmp(atts[i], "length") == 0) { length_s = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("Vector", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("Vector", name, set, get, help);
  
  if(length_s)
    {
    length = atoi(length_s);
    switch (length)
      {
      case 2: case 3: case 4: case 6: break;
      default: this->ReportBadAttribute("Vector", "length", length_s); break;
      }
    }
  else
    {
    this->ReportMissingAttribute("Vector", "length");
    }
  
  if(type)
    {
    if(strcmp(type, "int") == 0)
      {
      for(int i=0; i < length; ++i)
        { this->PVMethodInterface->AddIntegerArgument(); }
      }
    else if(strcmp(type, "float") == 0)
      {
      for(int i=0; i < length; ++i)
        { this->PVMethodInterface->AddFloatArgument(); }
      }
    else
      { this->ReportBadAttribute("Vector", "type", type); }
    }
  else
    {
    this->ReportMissingAttribute("Vector", "type");
    }  
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::StringElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0;
  const char* help=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("String", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("String", name, set, get, help);

  this->PVMethodInterface->AddStringArgument();
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::FileElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0; 
  const char* extension=0;
  const char* help=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "extension") == 0) { extension = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("File", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("File", name, set, get, help);

  this->PVMethodInterface->SetWidgetTypeToFile();
  
  if(extension) { this->PVMethodInterface->SetFileExtension(extension); }
  else { this->ReportMissingAttribute("File", "extension"); }
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::SelectionElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0; 
  const char* help=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("Selection", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("Selection", name, set, get, help);
  this->PVMethodInterface->SetWidgetTypeToSelection();
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ChoiceElementBegin(const char** atts)
{
  const char* name=0;
  const char* value_s=0;
  int value=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "value") == 0) { value_s = atts[i+1]; }
    else { this->ReportStrayAttribute("Choice", atts[i], atts[i+1]); }
    }
  
  if(!name)
    {
    this->ReportMissingAttribute("Choice", "name");
    name = "<missing>";
    }

  if(value_s)
    {
    value = atoi(value_s);
    if(value < 0)
      {
      this->ReportBadAttribute("Choice", "value", value_s);
      }
    }
  else
    {
    this->ReportMissingAttribute("Choice", "value");
    }
  
  this->PVMethodInterface->AddSelectionEntry(value, name);
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ExtentElementBegin(const char** atts)
{
  this->PVMethodInterface = vtkPVMethodInterface::New();
  
  const char* name=0;
  const char* set=0;
  const char* get=0; 
  const char* help=0;
  
  for(int i=0; atts[i] && atts[i+1]; i+=2)
    {
    if(strcmp(atts[i], "name") == 0) { name = atts[i+1]; }
    else if(strcmp(atts[i], "set") == 0) { set = atts[i+1]; }
    else if(strcmp(atts[i], "get") == 0) { get = atts[i+1]; }
    else if(strcmp(atts[i], "help") == 0) { help = atts[i+1]; }
    else { this->ReportStrayAttribute("Extent", atts[i], atts[i+1]); }
    }

  this->SetStandardMethodInterface("Extent", name, set, get, help);
  this->PVMethodInterface->SetWidgetTypeToExtent();
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::SourceElementEnd()
{
  this->SourceInterfaces->AddItem(this->PVSourceInterface);
  this->PVSourceInterface->Delete();
  this->PVSourceInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::FilterElementEnd()
{
  this->SourceInterfaces->AddItem(this->PVSourceInterface);
  this->PVSourceInterface->Delete();
  this->PVSourceInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::BooleanElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ScalarElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::VectorElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::StringElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::FileElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::SelectionElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ChoiceElementEnd()
{
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::ExtentElementEnd()
{
  this->PVSourceInterface->AddMethodInterface(this->PVMethodInterface);
  this->PVMethodInterface->Delete();
  this->PVMethodInterface = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSourceInterfaceParser::SetStandardMethodInterface(const char* element,
                                                            const char* name,
                                                            const char* set,
                                                            const char* get,
                                                            const char* help)
{
  if(name)
    {
    this->PVMethodInterface->SetVariableName(name);
    if(!set)
      {
      char* buf = new char[strlen(name)+4];
      strcpy(buf, "Set");
      strcat(buf, name);
      this->PVMethodInterface->SetSetCommand(buf);
      delete [] buf;
      }
    if(!get)
      {
      char* buf = new char[strlen(name)+4];
      strcpy(buf, "Get");
      strcat(buf, name);
      this->PVMethodInterface->SetGetCommand(buf);
      delete [] buf;
      }
    }
  else
    {
    this->ReportMissingAttribute(element, "name");
    }
  
  if(set) { this->PVMethodInterface->SetSetCommand(set); }
  if(get) { this->PVMethodInterface->SetGetCommand(get); }
  if(help) { this->PVMethodInterface->SetBalloonHelp(help); }
}
