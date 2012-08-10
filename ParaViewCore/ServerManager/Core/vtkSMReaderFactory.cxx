/*=========================================================================

  Program:   ParaView
  Module:    vtkSMReaderFactory.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMReaderFactory.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkStringList.h"

#include <list>
#include <set>
#include <string>
#include <vector>
#include <vtksys/ios/sstream>
#include <vtksys/SystemTools.hxx>
#include <vtksys/RegularExpression.hxx>
#include <assert.h>

static void string_replace(std::string& string, char c, std::string str)
{
  size_t cc= string.find(c);
  while (cc < std::string::npos)
    {
    string = string.replace(cc, 1, str);
    cc = string.find(c, cc + str.size());
    }
}

class vtkSMReaderFactory::vtkInternals
{
public:
  struct vtkValue
    {
    vtkWeakPointer<vtkSMSession> Session;
    std::string Group;
    std::string Name;
    std::vector<std::string> Extensions;
    std::vector<vtksys::RegularExpression> FilenameRegExs;
    std::vector<std::string> FilenamePatterns;
    std::string Description;

    vtkSMSessionProxyManager* GetProxyManager(vtkSMSession* session)
      {
      return vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
      }

    vtkSMProxy* GetPrototypeProxy(vtkSMSession* session, const char* groupName, const char* proxyName)
      {
      return session->GetSessionProxyManager()->GetPrototypeProxy(groupName, proxyName);
      }

    void FillInformation(vtkSMSession* session)
      {

      vtkSMProxy* prototype = this->GetPrototypeProxy(session,
                                                      this->Group.c_str(),
                                                      this->Name.c_str());
      if (!prototype || !prototype->GetHints())
        {
        return;
        }
      vtkPVXMLElement* rfHint =
        prototype->GetHints()->FindNestedElementByName("ReaderFactory");
      if (!rfHint)
        {
        return;
        }

      this->Extensions.clear();
      const char* exts = rfHint->GetAttribute("extensions");
      if (exts)
        {
        vtksys::SystemTools::Split(exts, this->Extensions,' ');
        }
      const char* filename_patterns = rfHint->GetAttribute("filename_patterns");
      if (filename_patterns)
        {
        vtksys::SystemTools::Split(filename_patterns, this->FilenamePatterns,' ');
        std::vector<std::string>::iterator iter;
        // convert the wild-card based patterns to regular expressions.
        for (iter = this->FilenamePatterns.begin(); iter !=
          this->FilenamePatterns.end(); iter++)
          {
          std::string regex = *iter;
          ::string_replace(regex, '.', "\\.");
          ::string_replace(regex, '?', ".");
          ::string_replace(regex, '*', ".?");
          this->FilenameRegExs.push_back(vtksys::RegularExpression(regex.c_str()));
          }
        }
      this->Description = rfHint->GetAttribute("file_description");
      }

    // Returns true is a prototype proxy can be created on the given connection.
    // For now, the connection is totally ignored since ServerManager doesn't
    // support that.
    bool CanCreatePrototype(vtkSMSession* session)
      {
      return (this->GetPrototypeProxy(session,
                                      this->Group.c_str(), this->Name.c_str())
              != NULL);
      }

    // Returns true if the reader can read the file. More correctly, it returns
    // false is the reader reports that it cannot read the file.
    bool CanReadFile(const char* filename,
      const std::vector<std::string>& extensions, vtkSMSession* session,
      bool skip_filename_test=false);

    // Tests if 'any' of the strings in extensions is contained in
    // this->Extensions.
    bool ExtensionTest(const std::vector<std::string>& extensions);

    // Tests if the FilenameRegEx matches the filename.
    bool FilenameRegExTest(const char* filename);
    };

  void BuildExtensions(
    const char* filename, std::vector<std::string>& extensions)
    {
    // basically we are filling up extensions with all possible extension
    // combintations eg. myfilename.tar.gz.vtk.000 results in
    // 000, vtk.000, gz.vtk.000, tar.gz.vtk.000,
    // vtk, gz.vtk, tar.gz.vtk
    // gz, tar.gz
    // tar, tar.gz
    // gz
    // in that order.
    std::string extension =
      vtksys::SystemTools::GetFilenameExtension(filename);
    if (extension.size() > 0)
      {
      extension.erase(extension.begin()); // remove the first "."
      }
    std::vector<std::string> parts;
    vtksys::SystemTools::Split(extension.c_str(), parts, '.');
    int num_parts = static_cast<int>(parts.size());
    for (int cc=num_parts-1; cc >= 0; cc--)
      {
      for (int kk=cc; kk >=0; kk--)
        {
        std::string cur_string;
        for (int ii=kk; ii <=cc; ii++)
          {
          if (parts[ii].size() == 0)
            {
            continue;//skip empty parts.
            }
          if (ii != kk)
            {
            cur_string += ".";
            }
          cur_string += parts[ii];
          }
        extensions.push_back(cur_string);
        }
      }
    }

  typedef std::list<vtkValue> PrototypesType;
  PrototypesType Prototypes;
  std::string SupportedFileTypes;
};

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::ExtensionTest(
  const std::vector<std::string>& extensions)
{
  if (this->Extensions.size() == 0)
    {
    return false;
    }

  std::vector<std::string>::const_iterator iter1;
  for (iter1 = extensions.begin(); iter1 != extensions.end(); ++iter1)
    {
    std::vector<std::string>::const_iterator iter2;
    for (iter2 = this->Extensions.begin(); iter2 != this->Extensions.end();
      ++iter2)
      {
      if (*iter1 == *iter2)
        {
        return true;
        }
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::FilenameRegExTest(
  const char* filename)
{
  if (this->FilenameRegExs.size() == 0)
    {
    return false;
    }

  std::vector<vtksys::RegularExpression>::iterator iter;
  for (iter = this->FilenameRegExs.begin();
    iter != this->FilenameRegExs.end(); ++iter)
    {
    if (iter->find(filename))
      {
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::CanReadFile(
  const char* filename,
  const std::vector<std::string>& extensions,
  vtkSMSession* session,
  bool skip_filename_test/*=false*/)
{
  vtkSMSessionProxyManager* pxm = this->GetProxyManager(session);
  vtkSMProxy* prototype = this->GetPrototypeProxy( session,
                                                   this->Group.c_str(),
                                                   this->Name.c_str() );
  if (!prototype)
    {
    return false;
    }

  if (!skip_filename_test)
    {
    this->FillInformation(session);
    if (!this->ExtensionTest(extensions) &&
      !this->FilenameRegExTest(filename))
      {
      return false;
      }
    }

  if (strcmp(prototype->GetXMLName(), "ImageReader") == 0)
    {
    // ImageReader always returns 0 so don't test it
    return true;
    }

  vtkSMProxy* proxy = pxm->NewProxy(this->Group.c_str(), this->Name.c_str());
  proxy->SetLocation(vtkProcessModule::DATA_SERVER_ROOT);
  // we deliberate don't call UpdateVTKObjects() here since CanReadFile() can
  // avoid callind UpdateVTKObjects() all together if not needed.
  // proxy->UpdateVTKObjects();
  bool canRead = vtkSMReaderFactory::CanReadFile(filename, proxy);
  proxy->Delete();
  return canRead;
}
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMReaderFactory);
//----------------------------------------------------------------------------
vtkSMReaderFactory::vtkSMReaderFactory()
{
  this->Internals = new vtkInternals();
  this->Readers = vtkStringList::New();
  this->ReaderName = 0;
  this->ReaderGroup = 0;
}

//----------------------------------------------------------------------------
vtkSMReaderFactory::~vtkSMReaderFactory()
{
  delete this->Internals;
  this->SetReaderName(0);
  this->SetReaderGroup(0);
  this->Readers->Delete();
  this->Readers = 0;
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::Initialize()
{
  this->Internals->Prototypes.clear();
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototypes(vtkSMSession* session, const char* xmlgroup)
{
  vtkSMSessionProxyManager* pxm = session->GetSessionProxyManager();
  vtkPVProxyDefinitionIterator* iter;
  iter = pxm->GetProxyDefinitionManager()->NewSingleGroupIterator(xmlgroup);
  for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkPVXMLElement* hints = pxm->GetProxyHints( iter->GetGroupName(),
                                                 iter->GetProxyName());
    if (hints && hints->FindNestedElementByName("ReaderFactory"))
      {
      this->RegisterPrototype(iter->GetGroupName(), iter->GetProxyName());
      }
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
unsigned int vtkSMReaderFactory::GetNumberOfRegisteredPrototypes()
{
  return static_cast<unsigned int>(this->Internals->Prototypes.size());
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototype(const char* xmlgroup, const char* xmlname)
{
  // If already present, we remove old one and append again so that the priority
  // rule still works.
  this->UnRegisterPrototype(xmlgroup, xmlname);
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;

  this->Internals->Prototypes.push_front(value);
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototype(
  const char* xmlgroup, const char* xmlname,
  const char* extensions, const char* description)

{
  // If already present, we remove old one and append again so that the priority
  // rule still works.
  this->UnRegisterPrototype(xmlgroup, xmlname);
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;

  if (description)
    {
    value.Description = description;
    }
  if (extensions)
    {
    vtksys::SystemTools::Split(extensions, value.Extensions, ' ');
    }
  this->Internals->Prototypes.push_front(value);
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::UnRegisterPrototype(
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
bool vtkSMReaderFactory::LoadConfigurationFile(const char* filename)
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
bool vtkSMReaderFactory::LoadConfiguration(const char* xmlcontents)
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
bool vtkSMReaderFactory::LoadConfiguration(vtkPVXMLElement* elem)
{
  if (!elem)
    {
    return false;
    }

  if (elem->GetName() &&
    strcmp(elem->GetName(), "ParaViewReaders") != 0)
    {
    return this->LoadConfiguration(
      elem->FindNestedElementByName("ParaViewReaders"));
    }

  unsigned int num = elem->GetNumberOfNestedElements();
  for(unsigned int i=0; i<num; i++)
    {
    vtkPVXMLElement* reader = elem->GetNestedElement(i);
    if (reader->GetName() &&
      (strcmp(reader->GetName(),"Reader") == 0 ||
       strcmp(reader->GetName(), "Proxy") == 0))
      {
      const char* name = reader->GetAttribute("name");
      const char* group = reader->GetAttribute("group");
      group = group ? group : "sources";
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
vtkStringList* vtkSMReaderFactory::GetReaders(vtkSMSession* session)
{
  return this->GetPossibleReaders(NULL, session);
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetReaders(const char* filename,
                                              vtkSMSession* session)
{
  this->Readers->RemoveAllItems();

  if (!filename || filename[0] == 0)
    {
    return this->Readers;
    }

  std::vector<std::string> extensions;
  this->Internals->BuildExtensions(filename, extensions);

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(session) &&
        iter->CanReadFile(filename, extensions, session))
      {
      iter->FillInformation(session);
      this->Readers->AddString(iter->Group.c_str());
      this->Readers->AddString(iter->Name.c_str());
      this->Readers->AddString(iter->Description.c_str());
      }
    }

  return this->Readers;
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetPossibleReaders(const char* filename,
  vtkSMSession* session)
{
  this->Readers->RemoveAllItems();

  if (!filename || filename[0] == 0)
    {
    return this->Readers;
    }

  std::vector<std::string> extensions;
  // purposefully set the extensions to empty, since we don't want the extension
  // test to be used for this case.

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(session) &&
      (!filename || iter->CanReadFile(filename, extensions, session, true)))
      {
      iter->FillInformation(session);
      this->Readers->AddString(iter->Group.c_str());
      this->Readers->AddString(iter->Name.c_str());
      this->Readers->AddString(iter->Description.c_str());
      }
    }

  return this->Readers;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, vtkSMSession* session)
{
  this->SetReaderGroup(0);
  this->SetReaderName(0);

  if (!filename || filename[0] == 0)
    {
    return false;
    }

  std::vector<std::string> extensions;
  this->Internals->BuildExtensions(filename, extensions);

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(session) && iter->CanReadFile(filename, extensions, session))
      {
      this->SetReaderGroup(iter->Group.c_str());
      this->SetReaderName(iter->Name.c_str());
      return true;
      }
    }
  return false;
}

//----------------------------------------------------------------------------
static std::string vtkJoin(
  const std::vector<std::string> exts, const char* prefix,
  const char* suffix)
{
  vtksys_ios::ostringstream stream;
  std::vector<std::string>::const_iterator iter;
  for (iter = exts.begin(); iter != exts.end(); ++iter)
    {
    stream << prefix << *iter << suffix;
    }
  return stream.str();
}

//----------------------------------------------------------------------------
const char* vtkSMReaderFactory::GetSupportedFileTypes(vtkSMSession* session)
{
  vtksys_ios::ostringstream all_types;
  all_types << "Supported Files (";

  std::set<std::string> sorted_types;

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin();
    iter != this->Internals->Prototypes.end(); ++iter)
    {
    if (iter->CanCreatePrototype(session))
      {
      iter->FillInformation(session);
      std::string ext_list;
      if (iter->Extensions.size() > 0)
        {
        ext_list = ::vtkJoin(iter->Extensions, "*.", " ");
        }

      if (iter->FilenameRegExs.size() > 0)
        {
        std::string ext_join = ::vtkJoin(
          iter->FilenamePatterns, "", " ");
        if (ext_list.size() > 0)
          {
          ext_list += " ";
          ext_list += ext_join;
          }
        else
          {
          ext_list = ext_join;
          }
        }

      if (ext_list.size() > 0)
        {
        vtksys_ios::ostringstream stream;
        stream << iter->Description << "(" << ext_list << ")";
        sorted_types.insert(stream.str());
        all_types << ext_list << " ";
        }
      }
    }
  all_types << ")";

  std::set<std::string>::iterator iter2;
  for (iter2 = sorted_types.begin(); iter2 != sorted_types.end(); ++iter2)
    {
    all_types << ";;" << (*iter2);
    }
  this->Internals->SupportedFileTypes = all_types.str();
  return this->Internals->SupportedFileTypes.c_str();
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::TestFileReadability(const char* filename, vtkSMSession* session)
{
  assert("Session should be valid" && session);
  vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("file_listing", "ServerFileListing"));
  if (!proxy)
    {
    vtkGenericWarningMacro("Failed to create ServerFileListing proxy.");
    return false;
    }

  proxy->SetLocation(vtkProcessModule::DATA_SERVER_ROOT);
  vtkSMPropertyHelper(proxy, "ActiveFileName").Set(filename);
  proxy->UpdateVTKObjects();
  proxy->UpdatePropertyInformation();

  if (vtkSMPropertyHelper(proxy, "ActiveFileIsReadable").GetAsInt() != 0)
    {
    return true;
    }

  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, vtkSMProxy* proxy)
{
  // Assume that it can read the file if CanReadFile does not exist.
  int canRead = 1;
  vtkSMSession* session = proxy->GetSession();

  vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
  if (source && session->GetNumberOfProcesses(source->GetLocation()) > 1)
    {
    if (source->GetProcessSupport() == vtkSMSourceProxy::SINGLE_PROCESS)
      {
      return false;
      }
    }
  else
    {
    if (source->GetProcessSupport() == vtkSMSourceProxy::MULTIPLE_PROCESSES)
      {
      return false;
      }
    }

  // ensure that VTK objects are created.
  proxy->UpdateVTKObjects();

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke
         << VTKOBJECT(proxy)
         << "CanReadFile" << filename
         << vtkClientServerStream::End;

  session->ExecuteStream(proxy->GetLocation(), stream, /*ignore_error*/true);
  session->GetLastResult(proxy->GetLocation()).GetArgument(0, 0, &canRead);
  return (canRead != 0);
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename,
  const char* readerxmlgroup, const char* readerxmlname, vtkSMSession* session)
{
  assert("Session should be valid" && session);
  vtkSMSessionProxyManager* pxm =
      vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSMProxy* proxy = pxm->NewProxy( readerxmlgroup, readerxmlname );
  if (!proxy)
    {
    return false;
    }
  proxy->SetLocation(vtkProcessModule::DATA_SERVER_ROOT);
  bool canRead = vtkSMReaderFactory::CanReadFile(filename, proxy);
  proxy->Delete();
  return canRead;
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
