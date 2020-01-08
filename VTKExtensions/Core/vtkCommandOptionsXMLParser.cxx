/*=========================================================================

  Program:   ParaView
  Module:    vtkCommandOptionsXMLParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCommandOptionsXMLParser.h"
#include "vtkCommandOptions.h"
#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include <map>

//----------------------------------------------------------------------------
struct vtkCommandOptionsXMLParserArgumentStructure
{
  enum Type
  {
    INT_TYPE,
    BOOL_TYPE,
    CHAR_TYPE
  };
  void* Variable;
  int VariableType;
  int ProcessType;
};

//----------------------------------------------------------------------------
//****************************************************************************
class vtkCommandOptionsXMLParserInternal
{
public:
  vtkCommandOptionsXMLParserInternal() { this->ProcessType = 0; }

  void AddArgument(
    const char* arg, vtkCommandOptionsXMLParserArgumentStructure::Type type, void* var, int ptype);
  int SetArgument(const char* arg, const char* value);
  int GetArgumentProcessType(const char* arg)
  {
    if (this->ArgumentToVariableMap.count(arg) == 0)
    {
      return 0;
    }
    return this->ArgumentToVariableMap[arg].ProcessType;
  }
  std::map<std::string, vtkCommandOptionsXMLParserArgumentStructure> ArgumentToVariableMap;
  int ProcessType;
};

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::SetProcessTypeInt(int ptype)
{
  this->Internals->ProcessType = ptype;
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::SetProcessType(const char* ptype)
{
  if (!ptype)
  {
    this->SetProcessTypeInt(vtkCommandOptions::EVERYBODY);
    return;
  }
}

//----------------------------------------------------------------------------
int vtkCommandOptionsXMLParserInternal::SetArgument(const char* arg, const char* value)
{
  if (this->ArgumentToVariableMap.count(arg))
  {
    vtkCommandOptionsXMLParserArgumentStructure tmp = this->ArgumentToVariableMap[arg];
    if (!(tmp.ProcessType & this->ProcessType || tmp.ProcessType == vtkCommandOptions::EVERYBODY ||
          this->ProcessType == vtkCommandOptions::EVERYBODY))
    {
      // Silently skip argument in xml because the process type does not match
      return 1;
    }
    switch (tmp.VariableType)
    {
      case vtkCommandOptionsXMLParserArgumentStructure::BOOL_TYPE:
      {
        int* variable = (int*)tmp.Variable;
        *variable = 1;
      }
      break;
      case vtkCommandOptionsXMLParserArgumentStructure::INT_TYPE:
      {
        if (!value)
        {
          vtkGenericWarningMacro("Bad XML Format missing Value for Name=\"" << arg << "\"");
          return 0;
        }
        int* variable = (int*)tmp.Variable;
        *variable = atoi(value);
      }
      break;
      case vtkCommandOptionsXMLParserArgumentStructure::CHAR_TYPE:
      {
        if (!value)
        {
          vtkGenericWarningMacro("Bad XML Format missing Value for Name=\"" << arg << "\"");
          return 0;
        }
        char** variable = static_cast<char**>(tmp.Variable);
        if (*variable)
        {
          delete[] * variable;
          *variable = 0;
        }
        *variable = strcpy(new char[strlen(value) + 1], value);
      }
      break;
    }
  }
  else
  {
    vtkGenericWarningMacro("Bad XML Format Unknown Option " << arg);
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParserInternal::AddArgument(
  const char* arg, vtkCommandOptionsXMLParserArgumentStructure::Type type, void* var, int ptype)
{
  if (strlen(arg) < 3)
  {
    vtkGenericWarningMacro("AddArgument must take arguments of the form --foo.  "
                           "Argument not added: "
      << arg);
    return;
  }
  vtkCommandOptionsXMLParserArgumentStructure vardata;
  vardata.VariableType = type;
  vardata.Variable = var;
  vardata.ProcessType = ptype;
  this->ArgumentToVariableMap[std::string(arg + 2)] = vardata;
}

//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCommandOptionsXMLParser);

//----------------------------------------------------------------------------
vtkCommandOptionsXMLParser::vtkCommandOptionsXMLParser()
{
  this->InPVXTag = 0;
  this->PVOptions = 0;
  this->Internals = new vtkCommandOptionsXMLParserInternal;
}

//----------------------------------------------------------------------------
vtkCommandOptionsXMLParser::~vtkCommandOptionsXMLParser()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::StartElement(const char* name, const char** atts)
{
  if (strcmp(name, "pvx") == 0)
  {
    this->InPVXTag = 1;
    return;
  }
  if (!this->InPVXTag)
  {
    vtkErrorMacro("Bad XML Element found not in <pvx></pvx> tag: " << name);
    return;
  }
  if (strcmp(name, "Option") == 0)
  {
    // check to see if the Named Option Name=option
    // is valid for this type of process. Each argument
    // has a number of processes that it is valid for.
    if (atts && atts[0] && atts[1])
    {
      if (strcmp(atts[0], "Name") == 0)
      {
        int type = this->Internals->GetArgumentProcessType(atts[1]);
        if (!(type & this->PVOptions->GetProcessType() || type == vtkCommandOptions::EVERYBODY))
        {
          return;
        }
      }
    }
    this->HandleOption(atts);
    return;
  }
  if (strcmp(name, "Process") == 0)
  {
    this->HandleProcessType(atts);
    return;
  }
  this->PVOptions->ParseExtraXMLTag(name, atts);
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::HandleProcessType(const char** atts)
{
  if (atts == nullptr || atts[0] == nullptr || strcmp(atts[0], "Type") != 0)
  {
    vtkErrorMacro(
      "Bad XML Format 0 attributes found in Process Type, expected  Process Type=\"..\" ");
    return;
  }
  if (atts[1] == nullptr)
  {
    vtkErrorMacro("Bad XML Format 1 attributes found in Process Process Type=\"..\" ");
    return;
  }
  this->SetProcessType(atts[1]);
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::HandleOption(const char** atts)
{
  // atts should be { "Name", "somename", "Value", "somevalue" }
  // The Value is optional as it may be a boolean option
  const char* nameTag = atts[0];
  const char* name = 0;
  // make sure there is a Name=
  if (!nameTag || (strcmp(nameTag, "Name") != 0))
  {
    vtkErrorMacro(
      "Bad XML Format 0 attributes found in Option, expected  Name=\"..\" [Value=\"...\"]");
    return;
  }
  // Set name to be the next attribute
  name = atts[1];
  // make sure Name=something
  if (!name)
  {
    vtkErrorMacro("Bad XML Format, Name has no name.");
    return;
  }

  // Now look for Value tag
  const char* valueTag = atts[2];
  const char* value = 0;
  // if there is a value tag and it is "Value"
  if (valueTag && (strcmp(valueTag, "Value") != 0))
  {
    vtkErrorMacro("Bad XML Format missing value tag");
    return;
  }
  else if (valueTag)
  {
    if (atts[3])
    {
      value = atts[3];
    }
    else
    {
      vtkErrorMacro("Bad XML Format missing value tag present but no value");
      return;
    }
  }

  this->Internals->SetArgument(name, value);
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::EndElement(const char* name)
{
  if (strcmp(name, "pvx") == 0)
  {
    this->InPVXTag = 0;
    return;
  }
  if (strcmp(name, "Process") == 0)
  {
    this->Internals->ProcessType = 0;
    return;
  }
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::AddBooleanArgument(const char* longarg, int* var, int type)
{
  this->Internals->AddArgument(
    longarg, vtkCommandOptionsXMLParserArgumentStructure::BOOL_TYPE, var, type);
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::AddArgument(const char* longarg, int* var, int type)
{
  this->Internals->AddArgument(
    longarg, vtkCommandOptionsXMLParserArgumentStructure::INT_TYPE, var, type);
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::AddArgument(const char* longarg, char** var, int type)
{
  this->Internals->AddArgument(
    longarg, vtkCommandOptionsXMLParserArgumentStructure::CHAR_TYPE, var, type);
}

//----------------------------------------------------------------------------
void vtkCommandOptionsXMLParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
