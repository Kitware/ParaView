/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProxyManagerExtension.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkProxyManagerExtension
// .SECTION Thanks
// <verbatim>
//
//  This file is part of the PointSprites plugin developed and contributed by
//
//  Copyright (c) CSCS - Swiss National Supercomputing Centre
//                EDF - Electricite de France
//
//  John Biddiscombe, Ugo Varetto (CSCS)
//  Stephane Ploix (EDF)
//
// </verbatim>

#include "vtkProxyManagerExtension.h"
#include "vtkSMXML_CSCS_PointSpriteRepresentation.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/string>

struct vtkProxyManagerExtensionValue
{
  vtkSmartPointer<vtkPVXMLElement> XMLElement;
  bool Added;
  vtkProxyManagerExtensionValue(vtkPVXMLElement* xml=0, bool added=false)
    {
    this->XMLElement = xml;
    this->Added = added;
    }
};

class vtkProxyManagerExtension::vtkMapOfElements :
  public vtkstd::map<vtkstd::string,  vtkProxyManagerExtensionValue>
{
};

#define PROXY_MANAGER_EXTENSION_SEPARATOR "-->"

vtkStandardNewMacro(vtkProxyManagerExtension);
vtkCxxRevisionMacro(vtkProxyManagerExtension, "1.1");
//----------------------------------------------------------------------------
vtkProxyManagerExtension::vtkProxyManagerExtension()
{
  this->MapOfElements = new vtkMapOfElements();

  char* xml_to_parse = vtkSMCSCS_PointSpriteRepresentationGetString();
  vtkPVXMLParser* parser = vtkPVXMLParser::New();
  if (!parser->Parse(xml_to_parse))
    {
    vtkErrorMacro("Incorrect XML. Check parsing errors. "
      "Aborting for debugging purposes.");
    abort();
    }

  vtkPVXMLElement* root = parser->GetRootElement();

  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    if (child && child->GetName() && strcmp(child->GetName(), "Extension") == 0)
      {
      vtkstd::string key = child->GetAttribute("group");
      key += PROXY_MANAGER_EXTENSION_SEPARATOR;
      key += child->GetAttribute("name");
      (*this->MapOfElements)[key] = vtkProxyManagerExtensionValue(child);
      }
    }
  parser->Delete();
}

//----------------------------------------------------------------------------
vtkProxyManagerExtension::~vtkProxyManagerExtension()
{
  delete this->MapOfElements;
  this->MapOfElements = 0;
}

//----------------------------------------------------------------------------
bool vtkProxyManagerExtension::CheckCompatibility(
  int vtkNotUsed(major), int vtkNotUsed(minor), int vtkNotUsed(patch))
{
  return true;
}


//----------------------------------------------------------------------------
vtkPVXMLElement* vtkProxyManagerExtension::GetProxyElement(const char* groupName,
  const char* proxyName, vtkPVXMLElement* currentElement)
{
  if (groupName && proxyName && currentElement)
    {
    vtkstd::string key = groupName;
    key += PROXY_MANAGER_EXTENSION_SEPARATOR;
    key += proxyName;


    vtkMapOfElements::iterator iter = this->MapOfElements->find(key);
    if (iter != this->MapOfElements->end())
      {
      vtkPVXMLElement* extElem = iter->second.XMLElement.GetPointer();
      if (!iter->second.Added)
        {
        iter->second.Added = true;
        unsigned int numElems = extElem->GetNumberOfNestedElements();
        for (unsigned int cc=0; cc < numElems; cc++)
          {
          currentElement->AddNestedElement(extElem->GetNestedElement(cc), /*setParent=*/0);
          }
        }
      }
    }

  return this->Superclass::GetProxyElement(groupName, proxyName, currentElement);
}

//----------------------------------------------------------------------------
void vtkProxyManagerExtension::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


