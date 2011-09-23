/*=========================================================================

  Program:   ParaView
  Module:    vtkSMWriterFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMWriterFactory.h"

#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMWriterProxy.h"

#include <vtkstd/list>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>

class vtkSMWriterFactory::vtkInternals
{
public:
  struct vtkValue
    {
    vtkstd::string Group;
    vtkstd::string Name;
    vtkstd::set<vtkstd::string> Extensions;
    vtkstd::string Description;

    void FillInformation(vtkSMProxyManager* pxm)
      {
      vtkSMProxy* prototype = pxm->GetPrototypeProxy(
        this->Group.c_str(), this->Name.c_str());
      if (!prototype || !prototype->GetHints())
        {
        return;
        }
      vtkPVXMLElement* rfHint =
        prototype->GetHints()->FindNestedElementByName("WriterFactory");
      if (!rfHint)
        {
        return;
        }

      this->Extensions.clear();
      const char* exts = rfHint->GetAttribute("extensions");
      if (exts)
        {
        vtkstd::vector<vtkstd::string> exts_v;
        vtksys::SystemTools::Split(exts, exts_v,' ');
        this->Extensions.insert(exts_v.begin(), exts_v.end());
        }
      this->Description = rfHint->GetAttribute("file_description");
      }

    // Returns true is a prototype proxy can be created on the given connection.
    // For now, the connection is totally ignored since ServerManager doesn't
    // support that.
    bool CanCreatePrototype(vtkSMProxyManager* pxm)
      {
      return (pxm->GetPrototypeProxy(
        this->Group.c_str(), this->Name.c_str()) != NULL);
      }

    // Returns true if the data from the given output port can be written.
    bool CanWrite(vtkSMSourceProxy* source, unsigned int port,
      vtkSMProxyManager* pxm)
      {
      vtkSMProxy* prototype = pxm->GetPrototypeProxy(
        this->Group.c_str(), this->Name.c_str());
      if (!prototype || !source)
        {
        return false;
        }
      vtkSMWriterProxy* writer = vtkSMWriterProxy::SafeDownCast(prototype);
      // If it's not a vtkSMWriterProxy, then we assume that it can
      // always work in parallel.
      if (writer)
        {
        if (source->GetSession()->GetNumberOfProcesses(source->GetLocation()) > 1)
          {
          if (!writer->GetSupportsParallel())
            {
            return false;
            }
          }
        else
          {
          if (writer->GetParallelOnly())
            {
            return false;
            }
          }
        }
      vtkSMInputProperty* pp = vtkSMInputProperty::SafeDownCast(
        prototype->GetProperty("Input"));
      if (!pp)
        {
        vtkGenericWarningMacro(<< prototype->GetXMLGroup()
          << " : " << prototype->GetXMLName()
          << " has no input property.");
        return false;
        }
      pp->RemoveAllUncheckedProxies();
      pp->AddUncheckedInputConnection(source, port);
      bool status = pp->IsInDomains() != 0;
      pp->RemoveAllUncheckedProxies();
      return status;
      }

    // Returns true if a file with the given extension can be written by this
    // writer. \c extension should not include the starting ".".
    bool ExtensionTest(const char* extension)
      {
      if (!extension || extension[0] == 0)
        {
        return false;
        }
      return (this->Extensions.find(extension) != this->Extensions.end());
      }
    };

  typedef vtkstd::list<vtkValue> PrototypesType;
  PrototypesType Prototypes;
  vtkstd::string SupportedFileTypes;
};

vtkStandardNewMacro(vtkSMWriterFactory);
//----------------------------------------------------------------------------
vtkSMWriterFactory::vtkSMWriterFactory()
{
  this->Internals = new vtkInternals();
  this->ProxyManager = 0;
}

//----------------------------------------------------------------------------
vtkSMWriterFactory::~vtkSMWriterFactory()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::Initialize()
{
  this->Internals->Prototypes.clear();
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::RegisterPrototypes(const char* xmlgroup)
{
  vtkPVProxyDefinitionIterator* iter = NULL;
  vtkSMProxyManager* pxm = this->GetProxyManager();
  iter = pxm->GetProxyDefinitionManager()->NewSingleGroupIterator(xmlgroup,0);
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVXMLElement* hints = pxm->GetProxyHints( iter->GetGroupName(),
                                                 iter->GetProxyName());
    if (hints && hints->FindNestedElementByName("WriterFactory"))
      {
      this->RegisterPrototype(iter->GetGroupName(), iter->GetProxyName());
      }
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
unsigned int vtkSMWriterFactory::GetNumberOfRegisteredPrototypes()
{
  return static_cast<unsigned int>(this->Internals->Prototypes.size());
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::RegisterPrototype(const char* xmlgroup, const char* xmlname)
{
  // If already present, we remove old one and append again so that the priority
  // rule still works.
  this->UnRegisterPrototype(xmlgroup, xmlname);

  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;
  
  // fills extension information etc. from the prototype.
  value.FillInformation(this->ProxyManager);

  this->Internals->Prototypes.push_front(value);
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::RegisterPrototype(
  const char* xmlgroup, const char* xmlname,
  const char* extensions, const char* description)

{
  // If already present, we remove old one and append again so that the priority
  // rule still works.
  this->UnRegisterPrototype(xmlgroup, xmlname);
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;
  
  // fills extension information etc. from the prototype.
  value.FillInformation(this->ProxyManager);
  if (description)
    {
    value.Description = description;
    }
  if (extensions)
    {
    vtkstd::vector<vtkstd::string> exts_v;
    vtksys::SystemTools::Split(extensions, exts_v , ' ');
    value.Extensions.clear();
    value.Extensions.insert(exts_v.begin(), exts_v.end());
    }
  this->Internals->Prototypes.push_front(value);
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::UnRegisterPrototype(
  const char* xmlgroup, const char* xmlname)
{
  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->Group == xmlgroup  && iter->Name == xmlname)
      {
      this->Internals->Prototypes.erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
bool vtkSMWriterFactory::LoadConfigurationFile(const char* filename)
{
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(filename);
  if (!parser->Parse())
    {
    vtkErrorMacro("Failed to parse file: " << filename);
    return false;
    }

  return this->LoadConfiguration(parser->GetRootElement());
}

//----------------------------------------------------------------------------
bool vtkSMWriterFactory::LoadConfiguration(const char* xmlcontents)
{
  vtkSmartPointer<vtkPVXMLParser> parser = 
    vtkSmartPointer<vtkPVXMLParser>::New();

  if (!parser->Parse(xmlcontents))
    {
    vtkErrorMacro("Failed to parse xml. Not a valid XML.");
    return false;
    }

  vtkPVXMLElement* rootElement = parser->GetRootElement();
  return this->LoadConfiguration(rootElement);
}

//----------------------------------------------------------------------------
bool vtkSMWriterFactory::LoadConfiguration(vtkPVXMLElement* elem)
{
  if (!elem)
    {
    return false;
    }

  if (elem->GetName() && 
    strcmp(elem->GetName(), "ParaViewWriters") != 0)
    {
    return this->LoadConfiguration(
      elem->FindNestedElementByName("ParaViewWriters"));
    }

  unsigned int num = elem->GetNumberOfNestedElements();
  for(unsigned int i=0; i<num; i++)
    {
    vtkPVXMLElement* reader = elem->GetNestedElement(i);
    if (reader->GetName() && 
      (strcmp(reader->GetName(),"Writer") == 0 || 
       strcmp(reader->GetName(), "Proxy") == 0))
      {
      const char* name = reader->GetAttribute("name");
      const char* group = reader->GetAttribute("group");
      group = group ? group : "writers";
      if (name && group)
        {
        // NOTE this is N^2. We may want to use a separate set or something to
        // test of existence if this becomes an issue.
        this->RegisterPrototype(group, name,
          reader->GetAttribute("extensions"),
          reader->GetAttribute("file_description"));
        }
      }
    }
  return true;
}


//----------------------------------------------------------------------------
vtkSMProxy* vtkSMWriterFactory::CreateWriter(
  const char* filename, vtkSMSourceProxy* source, unsigned int outputport)
{
  if (!filename || filename[0] == 0)
    {
    vtkErrorMacro("No filename. Cannot create any writer.");
    return NULL;
    }

  vtkstd::string extension =
    vtksys::SystemTools::GetFilenameExtension(filename);
  if (extension.size() > 0)
    {
    // Find characters after last "."
    size_t found = extension.find_last_of(".");
    if(found != -1)
      {
      extension = extension.substr(found+1);
      }
    else
      {
      vtkErrorMacro("No extension. Cannot determine writer to create.");
      return NULL;
      }
    
    }
  else
    {
    vtkErrorMacro("No extension. Cannot determine writer to create.");
    return NULL;
    }

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(this->ProxyManager) &&
      iter->ExtensionTest(extension.c_str()) &&
      iter->CanWrite(source, outputport, this->ProxyManager))
      {
      vtkSMProxy* proxy = this->ProxyManager->NewProxy(
        iter->Group.c_str(),
        iter->Name.c_str());
      vtkSMPropertyHelper(proxy, "FileName").Set(filename);
      vtkSMPropertyHelper(proxy, "Input").Set(source, outputport);
      return proxy;
      }
    }

  vtkErrorMacro("No matching writer found for extension: " << extension );
  return NULL;
}

//----------------------------------------------------------------------------
static vtkstd::string vtkJoin(
  const vtkstd::set<vtkstd::string> exts, const char* prefix,
  const char* suffix)
{
  vtksys_ios::ostringstream stream;
  vtkstd::set<vtkstd::string>::const_iterator iter;
  for (iter = exts.begin(); iter != exts.end(); ++iter)
    {
    stream << prefix << *iter << suffix;
    }
  return stream.str();
}

//----------------------------------------------------------------------------
const char* vtkSMWriterFactory::GetSupportedFileTypes(
  vtkSMSourceProxy* source, unsigned int outputport)
{
  vtkstd::set<vtkstd::string> sorted_types;

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(this->ProxyManager) &&
      iter->CanWrite(source, outputport, this->ProxyManager))
      {
      if (iter->Extensions.size() > 0)
        {
        vtkstd::string ext_join = ::vtkJoin(iter->Extensions, "*.", " ");
        vtksys_ios::ostringstream stream;
        stream << iter->Description << "(" << ext_join << ")";
        sorted_types.insert(stream.str());
        }
      }
    }
  
  vtksys_ios::ostringstream all_types;
  vtkstd::set<vtkstd::string>::iterator iter2;
  for (iter2 = sorted_types.begin(); iter2 != sorted_types.end(); ++iter2)
    {
    if (iter2 != sorted_types.begin())
      {
      all_types << ";;";
      }
    all_types << (*iter2);
    }
  this->Internals->SupportedFileTypes = all_types.str();
  return this->Internals->SupportedFileTypes.c_str();
}

//----------------------------------------------------------------------------
bool vtkSMWriterFactory::CanWrite(vtkSMSourceProxy* source, unsigned int outputport)
{
  if (!source)
    {
    return false;
    }
  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(this->ProxyManager) &&
      iter->CanWrite(source, outputport, this->ProxyManager))
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMWriterFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


