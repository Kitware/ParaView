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

#include "vtkCallbackCommand.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSmartPointer.h"
#include "vtkStringList.h"

#include <assert.h>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

static void string_replace(std::string& string, char c, std::string str)
{
  size_t cc = string.find(c);
  while (cc < std::string::npos)
  {
    string = string.replace(cc, 1, str);
    cc = string.find(c, cc + str.size());
  }
}

class vtkSMReaderFactory::vtkInternals
{
public:
  static std::set<std::pair<std::string, std::string> > ReaderWhitelist;
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

    vtkSMProxy* GetPrototypeProxy(
      vtkSMSession* session, const char* groupName, const char* proxyName)
    {
      return session->GetSessionProxyManager()->GetPrototypeProxy(groupName, proxyName);
    }

    void FillInformation(vtkSMSession* session)
    {
      vtkSMProxy* prototype =
        this->GetPrototypeProxy(session, this->Group.c_str(), this->Name.c_str());
      if (!prototype || !prototype->GetHints())
      {
        return;
      }
      vtkPVXMLElement* rfHint = prototype->GetHints()->FindNestedElementByName("ReaderFactory");
      if (!rfHint)
      {
        return;
      }

      this->Extensions.clear();
      const char* exts = rfHint->GetAttribute("extensions");
      if (exts)
      {
        vtksys::SystemTools::Split(exts, this->Extensions, ' ');
      }
      const char* filename_patterns = rfHint->GetAttribute("filename_patterns");
      if (filename_patterns)
      {
        vtksys::SystemTools::Split(filename_patterns, this->FilenamePatterns, ' ');
        std::vector<std::string>::iterator iter;
        // convert the wild-card based patterns to regular expressions.
        for (iter = this->FilenamePatterns.begin(); iter != this->FilenamePatterns.end(); iter++)
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
      return (this->GetPrototypeProxy(session, this->Group.c_str(), this->Name.c_str()) != NULL);
    }

    // Returns true if the reader can read the file. More correctly, it returns
    // false is the reader reports that it cannot read the file.
    bool CanReadFile(const char* filename, const std::vector<std::string>& extensions,
      vtkSMSession* session, bool skip_filename_test = false);

    // Tests if 'any' of the strings in extensions is contained in
    // this->Extensions.
    bool ExtensionTest(const std::vector<std::string>& extensions);

    // Tests if the FilenameRegEx matches the filename.
    bool FilenameRegExTest(const char* filename);
  };

  void BuildExtensions(const char* filename, std::vector<std::string>& extensions)
  {
    // basically we are filling up extensions with all possible extension
    // combintations eg. myfilename.tar.gz.vtk.000 results in
    // 000, vtk.000, gz.vtk.000, tar.gz.vtk.000,
    // vtk, gz.vtk, tar.gz.vtk
    // gz, tar.gz
    // tar, tar.gz
    // gz
    // in that order.
    std::string extension = vtksys::SystemTools::GetFilenameExtension(filename);
    if (extension.size() > 0)
    {
      extension.erase(extension.begin()); // remove the first "."
    }
    std::vector<std::string> parts;
    vtksys::SystemTools::Split(extension.c_str(), parts, '.');
    int num_parts = static_cast<int>(parts.size());
    for (int cc = num_parts - 1; cc >= 0; cc--)
    {
      for (int kk = cc; kk >= 0; kk--)
      {
        std::string cur_string;
        for (int ii = kk; ii <= cc; ii++)
        {
          if (parts[ii].size() == 0)
          {
            continue; // skip empty parts.
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

  // we use a map here instead of a set because I'm avoiding const
  // correctness of the methods of vtkValue. The key is a
  // combination of the prototype name and group.
  typedef std::map<std::string, vtkValue> PrototypesType;
  PrototypesType Prototypes;
  std::string SupportedFileTypes;
  // The set of groups that are searched for readers. By default "sources" is
  // included.
  std::set<std::string> Groups;
};
std::set<std::pair<std::string, std::string> > vtkSMReaderFactory::vtkInternals::ReaderWhitelist;

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
    for (iter2 = this->Extensions.begin(); iter2 != this->Extensions.end(); ++iter2)
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
bool vtkSMReaderFactory::vtkInternals::vtkValue::FilenameRegExTest(const char* filename)
{
  if (this->FilenameRegExs.size() == 0)
  {
    return false;
  }

  std::vector<vtksys::RegularExpression>::iterator iter;
  for (iter = this->FilenameRegExs.begin(); iter != this->FilenameRegExs.end(); ++iter)
  {
    if (iter->find(filename))
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::CanReadFile(const char* filename,
  const std::vector<std::string>& extensions, vtkSMSession* session,
  bool skip_filename_test /*=false*/)
{
  vtkSMSessionProxyManager* pxm = this->GetProxyManager(session);
  vtkSMProxy* prototype = this->GetPrototypeProxy(session, this->Group.c_str(), this->Name.c_str());
  if (!prototype)
  {
    return false;
  }

  if (!skip_filename_test)
  {
    this->FillInformation(session);
    if (!this->ExtensionTest(extensions) && !this->FilenameRegExTest(filename))
    {
      return false;
    }
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
  this->Internals->Groups.insert("sources");
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
  this->Internals->Groups.clear();
  this->Internals->Groups.insert("sources");
}

//----------------------------------------------------------------------------
unsigned int vtkSMReaderFactory::GetNumberOfRegisteredPrototypes()
{
  return static_cast<unsigned int>(this->Internals->Prototypes.size());
}

void vtkSMReaderFactory::UpdateAvailableReaders()
{
  vtkSMProxyManager* proxyManager = vtkSMProxyManager::GetProxyManager();
  // when we change the server we may not have a session yet. that's ok
  // since we'll come back here after the proxy definitions are loaded
  // from that session.
  if (vtkSMSession* session = proxyManager->GetActiveSession())
  {
    vtkSMSessionProxyManager* sessionProxyManager = session->GetSessionProxyManager();
    vtkSMProxyDefinitionManager* pdm = sessionProxyManager->GetProxyDefinitionManager();

    for (std::set<std::string>::iterator group = this->Internals->Groups.begin();
         group != this->Internals->Groups.end(); group++)
    {
      vtkPVProxyDefinitionIterator* iter = pdm->NewSingleGroupIterator(group->c_str());
      for (iter->GoToFirstItem(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        vtkPVXMLElement* hints =
          sessionProxyManager->GetProxyHints(iter->GetGroupName(), iter->GetProxyName());
        if (hints && hints->FindNestedElementByName("ReaderFactory"))
        {
          // By default this does no filtering on the readers available.  However, if the
          // application has specified that it is only interested in a subset of the readers
          // then only that subset will be available.
          std::pair<std::string, std::string> reader(iter->GetGroupName(), iter->GetProxyName());
          if (vtkInternals::ReaderWhitelist.empty() ||
            vtkInternals::ReaderWhitelist.find(reader) != vtkInternals::ReaderWhitelist.end())
          {
            this->RegisterPrototype(iter->GetGroupName(), iter->GetProxyName());
          }
        }
      }
      iter->Delete();
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::AddGroup(const char* groupName)
{
  if (groupName)
  {
    this->Internals->Groups.insert(groupName);
  }
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::RemoveGroup(const char* groupName)
{
  if (groupName)
  {
    this->Internals->Groups.erase(groupName);
  }
}

//----------------------------------------------------------------------------
void vtkSMReaderFactory::GetGroups(vtkStringList* groups)
{
  if (groups)
  {
    groups->RemoveAllItems();
    for (std::set<std::string>::iterator group = this->Internals->Groups.begin();
         group != this->Internals->Groups.end(); group++)
    {
      groups->AddString(group->c_str());
    }
  }
}
//----------------------------------------------------------------------------
void vtkSMReaderFactory::RegisterPrototype(const char* xmlgroup, const char* xmlname)
{
  vtkInternals::vtkValue value;
  value.Group = xmlgroup;
  value.Name = xmlname;
  std::string key = value.Name + value.Group;

  this->Internals->Prototypes[key] = value;
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetReaders(vtkSMSession* session)
{
  return this->GetPossibleReaders(NULL, session);
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetReaders(const char* filename, vtkSMSession* session)
{
  this->Readers->RemoveAllItems();

  if (!filename || filename[0] == 0)
  {
    return this->Readers;
  }

  std::vector<std::string> extensions;
  this->Internals->BuildExtensions(filename, extensions);

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin(); iter != this->Internals->Prototypes.end();
       ++iter)
  {
    if (iter->second.CanCreatePrototype(session) &&
      iter->second.CanReadFile(filename, extensions, session))
    {
      iter->second.FillInformation(session);
      this->Readers->AddString(iter->second.Group.c_str());
      this->Readers->AddString(iter->second.Name.c_str());
      this->Readers->AddString(iter->second.Description.c_str());
    }
  }

  return this->Readers;
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetPossibleReaders(const char* filename, vtkSMSession* session)
{
  this->Readers->RemoveAllItems();

  bool empty_filename = (!filename || filename[0] == 0);

  std::vector<std::string> extensions;
  // purposefully set the extensions to empty, since we don't want the extension
  // test to be used for this case.

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin(); iter != this->Internals->Prototypes.end();
       ++iter)
  {
    if (iter->second.CanCreatePrototype(session) &&
      (empty_filename || iter->second.CanReadFile(filename, extensions, session, true)))
    {
      iter->second.FillInformation(session);
      this->Readers->AddString(iter->second.Group.c_str());
      this->Readers->AddString(iter->second.Name.c_str());
      this->Readers->AddString(iter->second.Description.c_str());
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
  for (iter = this->Internals->Prototypes.begin(); iter != this->Internals->Prototypes.end();
       ++iter)
  {
    if (iter->second.CanCreatePrototype(session) &&
      iter->second.CanReadFile(filename, extensions, session))
    {
      this->SetReaderGroup(iter->second.Group.c_str());
      this->SetReaderName(iter->second.Name.c_str());
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
static std::string vtkJoin(
  const std::vector<std::string> exts, const char* prefix, const char* suffix)
{
  std::ostringstream stream;
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
  std::ostringstream all_types;
  all_types << "Supported Files (";

  std::set<std::string> sorted_types;

  vtkInternals::PrototypesType::iterator iter;
  for (iter = this->Internals->Prototypes.begin(); iter != this->Internals->Prototypes.end();
       ++iter)
  {
    if (iter->second.CanCreatePrototype(session))
    {
      iter->second.FillInformation(session);
      std::string ext_list;
      if (iter->second.Extensions.size() > 0)
      {
        ext_list = ::vtkJoin(iter->second.Extensions, "*.", " ");
      }

      if (iter->second.FilenameRegExs.size() > 0)
      {
        std::string ext_join = ::vtkJoin(iter->second.FilenamePatterns, "", " ");
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
        std::ostringstream stream;
        stream << iter->second.Description << "(" << ext_list << ")";
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

  // first check if the source requires MPI to be initialized and
  // that it is initialized on the server.
  if (source && source->GetMPIRequired() &&
    session->IsMPIInitialized(source->GetLocation()) == false)
  {
    return false;
  }

  // Check if the source requires multiple processes and if we have
  // multiple processes on the server.
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

  // creat a helper for calling CanReadFile on vtk objects
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSmartPointer<vtkSMProxy> helper;
  helper.TakeReference(pxm->NewProxy("misc", "FilePathEncodingHelper"));
  helper->UpdateVTKObjects();
  vtkSMPropertyHelper(helper->GetProperty("ActiveFileName")).Set(filename);
  vtkSMPropertyHelper(helper->GetProperty("ActiveGlobalId"))
    .Set(static_cast<vtkIdType>(proxy->GetGlobalID()));
  helper->UpdateVTKObjects();
  helper->UpdatePropertyInformation(helper->GetProperty("ActiveFileIsReadable"));
  canRead = vtkSMPropertyHelper(helper->GetProperty("ActiveFileIsReadable")).GetAsInt();
  return (canRead != 0);
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, const char* readerxmlgroup,
  const char* readerxmlname, vtkSMSession* session)
{
  assert("Session should be valid" && session);
  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetSessionProxyManager(session);
  vtkSMProxy* proxy = pxm->NewProxy(readerxmlgroup, readerxmlname);
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

//----------------------------------------------------------------------------
void vtkSMReaderFactory::AddReaderToWhitelist(const char* readerxmlgroup, const char* readerxmlname)
{
  if (readerxmlgroup != NULL && readerxmlname != NULL)
  {
    vtkSMReaderFactory::vtkInternals::ReaderWhitelist.insert(
      std::pair<std::string, std::string>(readerxmlgroup, readerxmlname));
  }
}
