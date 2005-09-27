/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLLookmarkElement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkXMLLookmarkElement.h"

#include "vtkObjectFactory.h"
#include "vtkXMLUtilities.h"

#include <ctype.h>

vtkCxxRevisionMacro(vtkXMLLookmarkElement, "1.1");
vtkStandardNewMacro(vtkXMLLookmarkElement);

//----------------------------------------------------------------------------
vtkXMLLookmarkElement::vtkXMLLookmarkElement()
{

}

//----------------------------------------------------------------------------
vtkXMLLookmarkElement::~vtkXMLLookmarkElement()
{

}

//----------------------------------------------------------------------------
void vtkXMLLookmarkElement::PrintXML(ostream& os, vtkIndent indent)
{
  vtkXMLLookmarkElement *lmkelem;
  int encoding = this->GetAttributeEncoding();
 
  os << indent << "<" << this->Name;
  int i;
  for(i=0;i < this->NumberOfAttributes;++i)
    {
    os << " " << this->AttributeNames[i] << "=\"";
    vtkXMLUtilities::EncodeString(this->AttributeValues[i],encoding,os,encoding,1);
    os << "\"";
    }
  if(this->NumberOfNestedElements > 0)
    {
    os << ">\n";
    for(i=0;i < this->NumberOfNestedElements;++i)
      {
      vtkIndent nextIndent = indent.GetNextIndent();
      lmkelem = (vtkXMLLookmarkElement *)this->NestedElements[i];
      lmkelem->PrintXML(os, nextIndent);
      }
    os << indent << "</" << this->Name << ">\n";
    }
  else
    {
    os << "/>\n";
    }
}

//----------------------------------------------------------------------------
void vtkXMLLookmarkElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

