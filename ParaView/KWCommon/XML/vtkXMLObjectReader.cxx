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
#include "vtkXMLObjectReader.h"

#include "vtkObjectFactory.h"
#include "vtkXMLDataElement.h"
#include "vtkXMLDataParser.h"
#include "vtkXMLUtilities.h"

vtkCxxRevisionMacro(vtkXMLObjectReader, "1.8.4.2");

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
