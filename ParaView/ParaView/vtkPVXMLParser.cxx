/*=========================================================================

  Program:   ParaView
  Module:    vtkPVXMLParser.cxx
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
#include "vtkPVXMLParser.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

vtkCxxRevisionMacro(vtkPVXMLParser, "1.2");
vtkStandardNewMacro(vtkPVXMLParser);

//----------------------------------------------------------------------------
vtkPVXMLParser::vtkPVXMLParser()
{
  this->FileName = 0;
  this->InputString = 0;
  this->NumberOfOpenElements = 0;
  this->OpenElementsSize = 10;
  this->OpenElements = new vtkPVXMLElement*[this->OpenElementsSize];
  this->ElementIdIndex = 0;
  this->RootElement = 0;
}

//----------------------------------------------------------------------------
vtkPVXMLParser::~vtkPVXMLParser()
{
  unsigned int i;
  for(i=0;i < this->NumberOfOpenElements;++i)
    {
    this->OpenElements[i]->Delete();
    }
  delete [] this->OpenElements;
  if(this->RootElement)
    {
    this->RootElement->Delete();
    }
  this->SetFileName(0);
}

//----------------------------------------------------------------------------
void vtkPVXMLParser::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName? this->FileName : "(none)")
     << "\n";
}

//----------------------------------------------------------------------------
void vtkPVXMLParser::StartElement(const char* name, const char** atts)
{
  vtkPVXMLElement* element = vtkPVXMLElement::New();
  element->SetName(name);
  element->ReadXMLAttributes(atts);
  const char* id = element->GetAttribute("id");
  if(id)
    {
    element->SetId(id);
    }
  else
    {
    ostrstream idstr;
    idstr << this->ElementIdIndex++ << ends;
    element->SetId(idstr.str());
    idstr.rdbuf()->freeze(0);
    }
  this->PushOpenElement(element);
}

//----------------------------------------------------------------------------
void vtkPVXMLParser::EndElement(const char* vtkNotUsed(name))
{
  vtkPVXMLElement* finished = this->PopOpenElement();
  unsigned int numOpen = this->NumberOfOpenElements;
  if(numOpen > 0)
    {
    this->OpenElements[numOpen-1]->AddNestedElement(finished);
    finished->Delete();
    }
  else
    {
    this->RootElement = finished;
    }
}

//----------------------------------------------------------------------------
void vtkPVXMLParser::PushOpenElement(vtkPVXMLElement* element)
{
  if(this->NumberOfOpenElements == this->OpenElementsSize)
    {
    unsigned int newSize = this->OpenElementsSize*2;
    vtkPVXMLElement** newOpenElements = new vtkPVXMLElement*[newSize];
    unsigned int i;
    for(i=0; i < this->NumberOfOpenElements;++i)
      {
      newOpenElements[i] = this->OpenElements[i];
      }
    delete [] this->OpenElements;
    this->OpenElements = newOpenElements;
    this->OpenElementsSize = newSize;
    }
  
  unsigned int pos = this->NumberOfOpenElements++;
  this->OpenElements[pos] = element;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLParser::PopOpenElement()
{
  if(this->NumberOfOpenElements > 0)
    {
    --this->NumberOfOpenElements;
    return this->OpenElements[this->NumberOfOpenElements];
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkPVXMLParser::PrintXML(ostream& os)
{
  this->RootElement->PrintXML(os, vtkIndent());
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkPVXMLParser::GetRootElement()
{
  return this->RootElement;
}

//----------------------------------------------------------------------------
int vtkPVXMLParser::Parse()
{
  if(!this->FileName)
    {
    vtkErrorMacro("No FileName set!");
    return 0;
    }
  
  ifstream inFile(this->FileName);
  if(!inFile)
    {
    vtkErrorMacro("Error opening " << this->FileName);
    return 0;
    }
  
  // Call the superclass's parser.
  this->SetStream(&inFile);
  int result = this->Superclass::Parse();
  this->SetStream(0);
  return result;
}

//----------------------------------------------------------------------------
int vtkPVXMLParser::Parse(const char* input)
{
  this->InputString = input;
  int result = this->Superclass::Parse();
  this->InputString = 0;
  return result;
}

//----------------------------------------------------------------------------
int vtkPVXMLParser::ParseXML()
{
  // Dispatch parser based on source of data.
  if(this->Stream)
    {
    return this->Superclass::ParseXML();
    }
  else if(this->InputString)
    {
    return this->ParseBuffer(this->InputString);
    }
  else
    {
    vtkErrorMacro("ParseXML() called with no stream or input string.");
    return 0;
    }
}
