/*=========================================================================

  Module:    vtkPVOptionsXMLParser.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVOptionsXMLParser.h"
#include "vtkPVOptions.h"
#include "vtkObjectFactory.h"
#include <vtkstd/map>
#include "vtkStdString.h"

struct vtkPVOptionsXMLParserArgumentStructure
{
  enum Type { INT_TYPE, BOOL_TYPE, CHAR_TYPE };
  void* Variable;
  int VariableType;
  int ProcessType;
};
 
//----------------------------------------------------------------------------
//****************************************************************************
class vtkPVOptionsXMLParserInternal
{
public:
  vtkPVOptionsXMLParserInternal()
    {
      this->ProcessType = vtkPVOptions::ALLPROCESS;
    }
  
  void AddArgument(const char* arg, vtkPVOptionsXMLParserArgumentStructure::Type type, void* var, int ptype);
  int SetArgument(const char* arg, const char* value);
  void SetProcessType(const char* ptype);
  int GetArgumentProcessType(const char* arg)
    {
      if(this->ArgumentToVariableMap.count(arg) == 0)
        {
        return 0;
        }
      return this->ArgumentToVariableMap[arg].ProcessType;
    }
  vtkstd::map<vtkstd::string, vtkPVOptionsXMLParserArgumentStructure> ArgumentToVariableMap;
  vtkPVOptions::ProcessTypeEnum ProcessType;
};


void vtkPVOptionsXMLParserInternal::SetProcessType(const char* ptype)
{
  if(!ptype)
    {
    this->ProcessType = vtkPVOptions::ALLPROCESS;
    return;
    }
  vtkstd::string type = ptype;
  if(type == "client")
    {
    this->ProcessType = vtkPVOptions::PVCLIENT;
    return;
    }
  if(type == "server")
    {
    this->ProcessType = vtkPVOptions::PVSERVER;
    return;
    }
  if(type == "render-server")
    {
    this->ProcessType = vtkPVOptions::PVRENDER_SERVER;
    return;
    }
  if(type == "data-server")
    {
    this->ProcessType = vtkPVOptions::PVDATA_SERVER;
    return;
    }
  if(type == "paraview")
    {
    this->ProcessType = vtkPVOptions::PARAVIEW;
    return;
    }
}

int vtkPVOptionsXMLParserInternal::SetArgument(const char* arg, const char* value)
{
  if(this->ArgumentToVariableMap.count(arg))
    {
    vtkPVOptionsXMLParserArgumentStructure tmp = this->ArgumentToVariableMap[arg];
    if(!(tmp.ProcessType & this->ProcessType))
      {
      // Silently skip argument in xml because the process type does not match
      return 1;
      }
    switch(tmp.VariableType)
      {
      case vtkPVOptionsXMLParserArgumentStructure::BOOL_TYPE:
        {
        int* variable = (int*)tmp.Variable;
        *variable = 1;
        }
        break;
      case vtkPVOptionsXMLParserArgumentStructure::INT_TYPE:
        { 
        if(!value)
          {
          vtkGenericWarningMacro("Bad XML Format missing Value for Name=\"" << arg << "\"");
          return 0;
          }
        int* variable = (int*)tmp.Variable;
        *variable = atoi(value);
        }
        break;
      case vtkPVOptionsXMLParserArgumentStructure::CHAR_TYPE: 
        { 
        if(!value)
          {
          vtkGenericWarningMacro("Bad XML Format missing Value for Name=\"" << arg << "\"");
          return 0;
          }
        char** variable = static_cast<char**>(tmp.Variable);
        if(*variable)
          {
          delete [] *variable;
          *variable = 0;
          }
        *variable = strcpy(new char[strlen(value)+1], value);
        }
        break;
      }
    }
  else
    {
    vtkGenericWarningMacro("Bad XML Format Unknown Option " << arg );
    return 0;
    }
  return 1;
}

void vtkPVOptionsXMLParserInternal::AddArgument(const char* arg, 
                                                vtkPVOptionsXMLParserArgumentStructure::Type type, 
                                                void* var,
                                                int ptype)
{
  if(strlen(arg) < 3)
    {
    vtkGenericWarningMacro(
      "AddArgument must take arguments of the form --foo.  Argument not added: " << arg );
    return;
    }
  vtkPVOptionsXMLParserArgumentStructure vardata;
  vardata.VariableType = type;
  vardata.Variable = var;
  vardata.ProcessType = ptype;
  this->ArgumentToVariableMap[vtkstd::string(arg+2)] = vardata;
}

//****************************************************************************
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVOptionsXMLParser);
vtkCxxRevisionMacro(vtkPVOptionsXMLParser, "1.1");

//----------------------------------------------------------------------------
vtkPVOptionsXMLParser::vtkPVOptionsXMLParser()
{  
  this->InPVXTag = 0;
  this->PVOptions = 0;
  this->Internals = new vtkPVOptionsXMLParserInternal;
}



//----------------------------------------------------------------------------
vtkPVOptionsXMLParser::~vtkPVOptionsXMLParser()
{
  delete this->Internals;
}



//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::StartElement(const char* name, const char** atts)
{
  if(strcmp(name, "pvx") == 0)
    {
    this->InPVXTag = 1;
    return;
    }
  if(!this->InPVXTag)
    {
    vtkErrorMacro("Bad XML Element found not in <pvx></pvx> tag: " << name);
    return;
    }
  if(strcmp(name, "Option") == 0)
    {
    // check to see if the Named Option Name=option 
    // is valid for this type of process. Each argument
    // has a number of processes that it is valid for.
    if(atts && atts[0] && atts[1])
      {
      if(strcmp(atts[0],"Name") == 0)
        {
        if(!(this->Internals->GetArgumentProcessType(atts[1]) & this->PVOptions->GetProcessType()))
          {
          return;
          }
        }
      }
    this->HandleOption(atts);
    return;
    }
  if(strcmp(name, "Process") == 0)
    {
    this->HandleProcessType(atts);
    return;
    }
  this->PVOptions->ParseExtraXMLTag(name, atts);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::HandleProcessType(const char** atts)
{
  if(!atts[0] && strcmp(atts[0], "Type"))
    {
    vtkErrorMacro("Bad XML Format 0 attributes found in Process Type, expected  Process Type=\"..\" ");
    return;
    }
  if(!atts[1])
    {
    vtkErrorMacro("Bad XML Format 1 attributes found in Process Process Type=\"..\" ");
    return;
    }
  this->Internals->SetProcessType(atts[1]);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::HandleOption(const char** atts)
{
  // atts should be { "Name", "somename", "Value", "somevalue" }
  // The Value is optional as it may be a boolean option
  const char* nameTag = atts[0];
  const char* name = 0;
  // make sure there is a Name= 
  if(!nameTag || (strcmp(nameTag, "Name") != 0))
    {
    vtkErrorMacro("Bad XML Format 0 attributes found in Option, expected  Name=\"..\" [Value=\"...\"]");
    return;
    }
  // Set name to be the next attribute
  name = atts[1];
  // make sure Name=somthing
  if(!name)
    {
    vtkErrorMacro("Bad XML Format, Name has no name.");
    return;
    }
  
  // Now look for Value tag
  const char* valueTag = atts[2];
  const char* value = 0;
  // if there is a value tag and it is "Vaule"
  if(valueTag && (strcmp(valueTag, "Value") != 0))
    {
    vtkErrorMacro("Bad XML Format missing value tag");
    return;
    }
  else
    {
    value = atts[3];
    if(!value)
      {
      vtkErrorMacro("Bad XML Format missing value tag present but no value");
      return;
      }
    }
  this->Internals->SetArgument(name, value);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::EndElement(const char* name)
{ 
  if(strcmp(name, "pvx") == 0)
    {
    this->InPVXTag = 0;
    return;
    }
  if(strcmp(name, "Process") == 0)
    {
    this->Internals->ProcessType = vtkPVOptions::ALLPROCESS;
    return;
    }
  
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::AddBooleanArgument(const char* longarg, int* var, int type)
{
  this->Internals->AddArgument(longarg, vtkPVOptionsXMLParserArgumentStructure::BOOL_TYPE, var, type);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::AddArgument(const char* longarg, int* var, int type)
{
  this->Internals->AddArgument(longarg, vtkPVOptionsXMLParserArgumentStructure::INT_TYPE, var, type);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::AddArgument(const char* longarg, char** var, int type)
{
  this->Internals->AddArgument(longarg, vtkPVOptionsXMLParserArgumentStructure::CHAR_TYPE, var, type);
}

//----------------------------------------------------------------------------
void vtkPVOptionsXMLParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
