/*=========================================================================

  Module:    vtkXMLObjectReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLObjectReader.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLUtilities.h"

vtkCxxRevisionMacro(vtkXMLObjectReader, "1.11");

//----------------------------------------------------------------------------
vtkXMLObjectReader::vtkXMLObjectReader()
{
  this->XMLParser = 0;  
  this->LastParsedElement = 0;
}

//----------------------------------------------------------------------------
vtkXMLObjectReader::~vtkXMLObjectReader()
{
  this->DestroyXMLParser();
}

//----------------------------------------------------------------------------
void vtkXMLObjectReader::CreateXMLParser()
{
  this->DestroyXMLParser();
  if(!this->XMLParser)
    {
    this->XMLParser = vtkXMLDataParser::New();
    }
}

//----------------------------------------------------------------------------
void vtkXMLObjectReader::DestroyXMLParser()
{
  if (this->XMLParser)
    {
    this->XMLParser->Delete();
    this->XMLParser = 0;
    }
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::ParseStream(istream &is)
{
  this->CreateXMLParser();
  this->XMLParser->SetStream(&is);
  this->XMLParser->SetAttributesEncoding(this->GetDefaultCharacterEncoding());

  if (this->XMLParser->Parse())
    {
    vtkXMLUtilities::UnFactorElements(this->XMLParser->GetRootElement());
    return this->Parse(this->XMLParser->GetRootElement());
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::ParseString(const char *str)
{
  if (!str)
    {
    return 0;
    }

  strstream strstr;
  strstr << str;
  int res = this->ParseStream(strstr);
  strstr.rdbuf()->freeze(0);
  return res;
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::ParseFile(const char *filename)
{
  ifstream is(filename);

  return this->ParseStream(is);
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::InitializeParsing()
{
  this->LastParsedElement = 0;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::Parse(vtkXMLDataElement *elem)
{
  this->InitializeParsing();

  if (!this->Object)
    {
    vtkWarningMacro(<< "The Object is not set!");
    return 0;
    }

  if (!elem)
    {
    vtkWarningMacro(<< "Can not parse a NULL XML element!");
    return 0;
    }

  this->LastParsedElement = elem;

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::ParseInElement(vtkXMLDataElement *parent)
{
  if (!parent)
    {
    return 0;
    }

  // Look for the nested element matching the root name

  vtkXMLDataElement *nested_elem = 
    parent->FindNestedElementWithName(this->GetRootElementName());
  if (!nested_elem)
    {
    return 0;
    }

  // Parse the nested element

  return this->Parse(nested_elem);
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::IsInElement(vtkXMLDataElement *parent)
{
  if (!parent)
    {
    return 0;
    }

  // Look for the nested element matching the root name

  vtkXMLDataElement *nested_elem = 
    parent->FindNestedElementWithName(this->GetRootElementName());
  if (!nested_elem)
    {
    return 0;
    }

  // It is in the nested element

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::ParseInNestedElement(vtkXMLDataElement *grandparent,
                                             const char *name)
{
  if (!grandparent || !name || !*name)
    {
    return 0;
    }

  // Look for the nested parent element matching the name

  vtkXMLDataElement *parent = grandparent->FindNestedElementWithName(name);
  if (!parent)
    {
    return 0;
    }

  // Parse the parent (look for the element inside)

  return this->ParseInElement(parent);
}

//----------------------------------------------------------------------------
int vtkXMLObjectReader::IsInNestedElement(vtkXMLDataElement *grandparent,
                                          const char *name)
{
  if (!grandparent || !name || !*name)
    {
    return 0;
    }

  // Look for the nested parent element matching the name

  vtkXMLDataElement *parent = grandparent->FindNestedElementWithName(name);
  if (!parent)
    {
    return 0;
    }

  // Is it in parent ?

  return this->IsInElement(parent);
}
