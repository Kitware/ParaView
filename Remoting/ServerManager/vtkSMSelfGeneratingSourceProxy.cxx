/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelfGeneratingSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelfGeneratingSourceProxy.h"

#include "vtkClientServerStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMProxyInternals.h"
#include "vtkSmartPointer.h"

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

class vtkSMSelfGeneratingSourceProxy::vtkInternals
{
public:
  std::vector<vtkSmartPointer<vtkPVXMLElement> > ExtendedDefinitionXMLs;

  // This is fairly simplistic, but then we don't expect this to be called too
  // frequently.
  bool Contains(vtkPVXMLElement* other)
  {
    assert(other);
    for (size_t cc = 0; cc < this->ExtendedDefinitionXMLs.size(); ++cc)
    {
      if (other->Equals(this->ExtendedDefinitionXMLs[cc]))
      {
        return true;
      }
    }
    return false;
  }
};

vtkStandardNewMacro(vtkSMSelfGeneratingSourceProxy);
//----------------------------------------------------------------------------
vtkSMSelfGeneratingSourceProxy::vtkSMSelfGeneratingSourceProxy()
{
  this->Internals = new vtkSMSelfGeneratingSourceProxy::vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMSelfGeneratingSourceProxy::~vtkSMSelfGeneratingSourceProxy()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
bool vtkSMSelfGeneratingSourceProxy::ExtendDefinition(const char* additional_properties_definition)
{
  vtkNew<vtkPVXMLParser> parser;
  if (!parser->Parse(additional_properties_definition))
  {
    vtkErrorMacro("Failed to parse extended proxy definition. Not a valid XML.");
    return false;
  }

  return this->ExtendDefinition(parser->GetRootElement());
}

//----------------------------------------------------------------------------
bool vtkSMSelfGeneratingSourceProxy::ExtendDefinition(vtkPVXMLElement* xml)
{
  if (this->CreateSubProxiesAndProperties(this->GetSessionProxyManager(), xml) == 0)
  {
    return false;
  }

  if (this->ObjectsCreated != 0 && this->ExtendDefinitionOnSIProxy(xml) == false)
  {
    return false;
  }

  if (this->ObjectsCreated)
  {
    this->RebuildStateForProperties();
  }

  this->Internals->ExtendedDefinitionXMLs.push_back(xml);
  return true;
}

//----------------------------------------------------------------------------
void vtkSMSelfGeneratingSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
  {
    return;
  }

  for (size_t cc = 0; cc < this->Internals->ExtendedDefinitionXMLs.size(); ++cc)
  {
    this->ExtendDefinitionOnSIProxy(this->Internals->ExtendedDefinitionXMLs[cc]);
  }
}

//----------------------------------------------------------------------------
bool vtkSMSelfGeneratingSourceProxy::ExtendDefinitionOnSIProxy(vtkPVXMLElement* xml)
{
  std::ostringstream str;
  xml->PrintXML(str, vtkIndent());

  // Notify SIProxy about the extended definition so when the property values
  // are pushed it knows what to do.
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke << SIPROXY(this) << "ExtendDefinition"
         << str.str().c_str() << vtkClientServerStream::End;
  this->ExecuteStream(stream);

  vtkClientServerStream result = this->GetLastResult();
  int tmp;
  if (result.GetNumberOfMessages() != 1 || result.GetNumberOfArguments(0) != 1 ||
    !result.GetArgument(0, 0, &tmp) || tmp == 0)
  {
    // failed on server side to load the definition. Should not happen
    // generally, but it may in some weird cases.
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMSelfGeneratingSourceProxy::SaveXMLState(
  vtkPVXMLElement* root, vtkSMPropertyIterator* iter)
{
  if (vtkPVXMLElement* xml = this->Superclass::SaveXMLState(root, iter))
  {
    if (xml->FindNestedElementByName("ExtendedDefinition") == NULL)
    {
      vtkNew<vtkPVXMLElement> node;
      node->SetName("ExtendedDefinition");
      for (size_t cc = 0; cc < this->Internals->ExtendedDefinitionXMLs.size(); ++cc)
      {
        node->AddNestedElement(this->Internals->ExtendedDefinitionXMLs[cc]);
      }
      xml->AddNestedElement(node.GetPointer());
    }
    return xml;
  }

  return NULL;
}

//----------------------------------------------------------------------------
int vtkSMSelfGeneratingSourceProxy::LoadXMLState(
  vtkPVXMLElement* element, vtkSMProxyLocator* locator)
{
  std::vector<vtkPVXMLElement*> extensions;
  vtkPVXMLElement* defs = element ? element->FindNestedElementByName("ExtendedDefinition") : NULL;

  for (unsigned int cc = 0, max = (defs ? defs->GetNumberOfNestedElements() : 0); cc < max; ++cc)
  {
    vtkPVXMLElement* node = defs->GetNestedElement(cc);
    if (node && !this->Internals->Contains(node))
    {
      extensions.push_back(node);
    }
  }

  for (size_t cc = 0; cc < extensions.size(); ++cc)
  {
    if (!this->ExtendDefinition(extensions[cc]))
    {
      vtkErrorMacro("Failed to extend definition!");
      return 0;
    }
  }

  return this->Superclass::LoadXMLState(element, locator);
}

//----------------------------------------------------------------------------
void vtkSMSelfGeneratingSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
