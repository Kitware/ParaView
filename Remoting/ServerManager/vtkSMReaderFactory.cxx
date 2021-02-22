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
#include "vtkCollection.h"
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

#include <algorithm>
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
    std::string Label;
    struct FileEntryHint
    {
      std::vector<std::string> Extensions;
      std::vector<vtksys::RegularExpression> FilenameRegExs;
      std::vector<std::string> FilenamePatterns;
      std::string Description;
      bool IsDirectory = false;
    };
    std::vector<FileEntryHint> FileEntryHints;

    vtkValue() = default;

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
      this->Label = prototype->GetXMLLabel();

      vtkNew<vtkCollection> rfHints;
      prototype->GetHints()->FindNestedElementByName("ReaderFactory", rfHints.GetPointer());
      int size = rfHints->GetNumberOfItems();
      this->FileEntryHints.clear();
      this->FileEntryHints.reserve(size);
      for (int idx = 0; idx < size; ++idx)
      {
        vtkPVXMLElement* rfHint = vtkPVXMLElement::SafeDownCast(rfHints->GetItemAsObject(idx));

        FileEntryHint hint;

        // Description
        hint.Description = rfHint->GetAttribute("file_description");

        // Extensions
        const char* exts = rfHint->GetAttribute("extensions");
        if (exts)
        {
          vtksys::SystemTools::Split(exts, hint.Extensions, ' ');
        }

        // Patterns
        const char* filename_patterns = rfHint->GetAttribute("filename_patterns");
        if (filename_patterns)
        {
          vtksys::SystemTools::Split(filename_patterns, hint.FilenamePatterns, ' ');
          // convert the wild-card based patterns to regular expressions.
          for (auto item : hint.FilenamePatterns)
          {
            ::string_replace(item, '.', "\\.");
            ::string_replace(item, '?', ".");
            ::string_replace(item, '*', ".?");
            hint.FilenameRegExs.emplace_back(vtksys::RegularExpression(item.c_str()));
          }
        }

        // Directory
        int is_directory = 0;
        if (rfHint->GetScalarAttribute("is_directory", &is_directory))
        {
          hint.IsDirectory = (is_directory == 1);
        }
        else
        {
          hint.IsDirectory = false;
        }

        // Add the completed hint
        this->FileEntryHints.emplace_back(std::move(hint));
      }
    }

    // Returns true is a prototype proxy can be created on the given connection.
    // For now, the connection is totally ignored since ServerManager doesn't
    // support that.
    bool CanCreatePrototype(vtkSMSession* session)
    {
      return (this->GetPrototypeProxy(session, this->Group.c_str(), this->Name.c_str()) != nullptr);
    }

    // Returns true if the reader can read the file. More correctly, it returns
    // false is the reader reports that it cannot read the file.
    // is_dir == true, if filename refers to a directory.
    bool CanReadFile(const char* filename, bool is_dir, const std::vector<std::string>& extensions,
      vtkSMSession* session, bool skip_filename_test = false);

    // Tests if 'any' of the strings in extensions is contained in
    // this->Extensions.
    bool ExtensionTest(const std::vector<std::string>& extensions) const;

    // Tests if the FilenameRegEx matches the filename.
    bool FilenameRegExTest(const char* filename) const;
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
  const std::vector<std::string>& extensions) const
{
  for (auto& hint : this->FileEntryHints)
  {
    for (auto& hint_ext : hint.Extensions)
    {
      for (auto& ext : extensions)
      {
        if (ext == hint_ext)
        {
          return true;
        }
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::FilenameRegExTest(const char* filename) const
{
  for (auto& hint : this->FileEntryHints)
  {
    for (auto& hint_regex : hint.FilenameRegExs)
    {
      vtksys::RegularExpressionMatch match;
      if (hint_regex.find(filename, match))
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::vtkInternals::vtkValue::CanReadFile(const char* filename, bool is_dir,
  const std::vector<std::string>& extensions, vtkSMSession* session,
  bool skip_filename_test /*=false*/)
{
  vtkSMSessionProxyManager* pxm = this->GetProxyManager(session);
  vtkSMProxy* prototype = this->GetPrototypeProxy(session, this->Group.c_str(), this->Name.c_str());
  if (!prototype)
  {
    return false;
  }

  this->FillInformation(session);

  if (std::none_of(this->FileEntryHints.begin(), this->FileEntryHints.end(),
        [is_dir](const FileEntryHint& hint) -> bool { return hint.IsDirectory == is_dir; }))
  {
    return false;
  }

  if (!skip_filename_test)
  {
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
// Use VTK's object factory to construct new instances. This allows derived
// applications to derive from vtkSMReaderFactory and implement changes to its
// functionality.
vtkObjectFactoryNewMacro(vtkSMReaderFactory);
//----------------------------------------------------------------------------
vtkSMReaderFactory::vtkSMReaderFactory()
{
  this->Internals = new vtkInternals();
  this->Internals->Groups.insert("sources");
  this->Readers = vtkStringList::New();
  this->ReaderName = nullptr;
  this->ReaderGroup = nullptr;
}

//----------------------------------------------------------------------------
vtkSMReaderFactory::~vtkSMReaderFactory()
{
  delete this->Internals;
  this->SetReaderName(nullptr);
  this->SetReaderGroup(nullptr);
  this->Readers->Delete();
  this->Readers = nullptr;
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

    for (auto& group : this->Internals->Groups)
    {
      vtkPVProxyDefinitionIterator* iter = pdm->NewSingleGroupIterator(group.c_str());
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
    for (auto& group : this->Internals->Groups)
    {
      groups->AddString(group.c_str());
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
  return this->GetPossibleReaders(nullptr, session);
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

  const bool is_dir = vtkSMReaderFactory::GetFilenameIsDirectory(filename, session);
  for (auto& proto : this->Internals->Prototypes)
  {
    if (proto.second.CanCreatePrototype(session))
    {
      proto.second.FillInformation(session);
      if (proto.second.CanReadFile(filename, is_dir, extensions, session))
      {
        this->Readers->AddString(proto.second.Group.c_str());
        this->Readers->AddString(proto.second.Name.c_str());
        this->Readers->AddString(proto.second.Label.c_str());
      }
    }
  }

  return this->Readers;
}

//----------------------------------------------------------------------------
vtkStringList* vtkSMReaderFactory::GetPossibleReaders(const char* filename, vtkSMSession* session)
{
  this->Readers->RemoveAllItems();

  bool empty_filename = (!filename || filename[0] == 0);

  const bool is_dir =
    empty_filename ? false : vtkSMReaderFactory::GetFilenameIsDirectory(filename, session);

  std::vector<std::string> extensions;
  // purposefully set the extensions to empty, since we don't want the extension
  // test to be used for this case.

  for (auto& proto : this->Internals->Prototypes)
  {
    if (proto.second.CanCreatePrototype(session))
    {
      proto.second.FillInformation(session);
      if (empty_filename || proto.second.CanReadFile(filename, is_dir, extensions, session, true))
      {
        this->Readers->AddString(proto.second.Group.c_str());
        this->Readers->AddString(proto.second.Name.c_str());
        this->Readers->AddString(proto.second.Label.c_str());
      }
    }
  }

  return this->Readers;
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::CanReadFile(const char* filename, vtkSMSession* session)
{
  this->SetReaderGroup(nullptr);
  this->SetReaderName(nullptr);

  if (!filename || filename[0] == 0)
  {
    return false;
  }

  const bool is_dir = vtkSMReaderFactory::GetFilenameIsDirectory(filename, session);

  std::vector<std::string> extensions;
  this->Internals->BuildExtensions(filename, extensions);

  for (auto& proto : this->Internals->Prototypes)
  {
    if (proto.second.CanCreatePrototype(session))
    {
      proto.second.FillInformation(session);
      if (proto.second.CanReadFile(filename, is_dir, extensions, session))
      {
        this->SetReaderGroup(proto.second.Group.c_str());
        this->SetReaderName(proto.second.Name.c_str());
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
static std::string vtkJoin(
  const std::vector<std::string>& exts, const char* prefix, const char* separator)
{
  bool is_head = true;
  std::ostringstream stream;
  for (const auto& an_ext : exts)
  {
    stream << (is_head == false ? separator : "") << prefix << an_ext;
    is_head = false;
  }
  return stream.str();
}

//----------------------------------------------------------------------------
const char* vtkSMReaderFactory::GetSupportedFileTypes(vtkSMSession* session)
{
  std::ostringstream all_types;
  all_types << "Supported Files (";

  auto case_insensitive_comp = [](const std::string& s1, const std::string& s2) {
    return vtksys::SystemTools::Strucmp(s1.c_str(), s2.c_str()) < 0;
  };
  std::set<std::string, decltype(case_insensitive_comp)> sorted_types(case_insensitive_comp);

  for (auto& proto : this->Internals->Prototypes)
  {
    if (proto.second.CanCreatePrototype(session))
    {
      proto.second.FillInformation(session);
      for (auto& hint : proto.second.FileEntryHints)
      {
        std::string ext_list;
        if (hint.Extensions.size() > 0)
        {
          ext_list = ::vtkJoin(hint.Extensions, "*.", " ");
        }

        if (hint.FilenameRegExs.size() > 0)
        {
          std::string ext_join = ::vtkJoin(hint.FilenamePatterns, "", " ");
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
          stream << hint.Description << " (" << ext_list << ")";
          sorted_types.insert(stream.str());
          all_types << ext_list << " ";
        }
      }
    }
  }
  all_types << ")";

  for (auto types : sorted_types)
  {
    all_types << ";;" << types;
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

  // create a helper for calling CanReadFile on vtk objects
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
  if (readerxmlgroup != nullptr && readerxmlname != nullptr)
  {
    vtkSMReaderFactory::vtkInternals::ReaderWhitelist.insert(
      std::pair<std::string, std::string>(readerxmlgroup, readerxmlname));
  }
}

//----------------------------------------------------------------------------
bool vtkSMReaderFactory::GetFilenameIsDirectory(const char* fname, vtkSMSession* session)
{
  if (session && fname && fname[0])
  {
    bool is_dir = false;
    auto pxm = session->GetSessionProxyManager();
    if (vtkSMProxy* proxy = pxm->NewProxy("misc", "Directory"))
    {
      proxy->SetLocation(vtkProcessModule::DATA_SERVER_ROOT);
      proxy->UpdateVTKObjects();
      if (vtkSMProxy* helper = pxm->NewProxy("misc", "FilePathEncodingHelper"))
      {
        vtkSMPropertyHelper(helper, "ActiveFileName").Set(fname);
        vtkSMPropertyHelper(helper, "ActiveGlobalId")
          .Set(static_cast<vtkIdType>(proxy->GetGlobalID()));
        helper->UpdateVTKObjects();
        helper->UpdatePropertyInformation(helper->GetProperty("IsDirectory"));
        is_dir = vtkSMPropertyHelper(helper->GetProperty("IsDirectory")).GetAsInt() == 1;
        helper->Delete();
      }
      proxy->Delete();
    }
    return is_dir;
  }

  return false;
}
