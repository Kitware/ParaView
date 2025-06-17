// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMLoadStateOptionsProxy.h"

#include "vtkClientServerStream.h"
#include "vtkFileSequenceParser.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkSMCoreUtilities.h"
#include "vtkSMFileListDomain.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSMTrace.h"

#include <vtk_pugixml.h>
#include <vtksys/RegularExpression.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <set>
#include <sstream>

//---------------------------------------------------------------------------
class vtkSMLoadStateOptionsProxy::vtkInternals
{
  vtkSmartPointer<vtkPVXMLElement> Hints;

public:
  struct PropertyInfo
  {
  private:
    std::vector<std::string> OriginalFilePaths;

  public:
    pugi::xml_node XMLElement;
    std::vector<std::string> FilePaths;
    bool UpdateProxyName = false;

    std::string GetPropertyXMLName() const { return this->XMLElement.attribute("name").value(); }

    bool IsModified() const { return this->FilePaths != this->OriginalFilePaths; }
    // Populate FilePaths using current XMLElement values.
    void PopulateFilePaths()
    {
      this->FilePaths.clear();
      for (auto child : this->XMLElement.children("Element"))
      {
        this->FilePaths.push_back(child.attribute("value").value());
      }
      this->OriginalFilePaths = this->FilePaths;
    }

    void Update()
    {
      if (!this->IsModified())
      {
        return;
      }
      this->XMLElement.remove_attribute("number_of_elements");
      this->XMLElement.append_attribute("number_of_elements") =
        static_cast<int>(this->FilePaths.size());
      while (this->XMLElement.remove_child("Element"))
      {
      }
      int index = 0;
      for (const auto& fname : this->FilePaths)
      {
        auto child = this->XMLElement.append_child("Element");
        child.append_attribute("index") = index++;
        child.append_attribute("value") = fname.c_str();
      }
    }

    std::string GetFileName(bool woExtension) const
    {
      if (auto element = this->XMLElement.child("Element"))
      {
        auto fname = element.attribute("value").value();
        return woExtension ? vtksys::SystemTools::GetFilenameWithoutExtension(fname)
                           : vtksys::SystemTools::GetFilenameName(fname);
      }
      return std::string();
    }

    std::string GetSequenceName() const
    {
      const auto name = this->GetFileName(/*woExtension=*/false);
      vtkNew<vtkFileSequenceParser> sequenceParser;
      if (!name.empty() && sequenceParser->ParseFileSequence(name.c_str()))
      {
        return sequenceParser->GetSequenceName();
      }
      return name;
    }
  };

  // A map of { id : { property-name: PropertyInfo } }.
  std::map<int, std::map<std::string, PropertyInfo>> PropertiesMap;

  // A map that helps us identify the property (id, propertyname) given its
  // exposed name.
  std::map<std::string, std::pair<int, std::string>> ExposedPropertyNameMap;

  // A map of proxy names used and their count: this is used to ensure each
  // proxy gets a different names when coming up with exposed names for its
  // properties to avoid issues with state files that have same registration
  // name for multiple readers.
  std::map<std::string, int> ProxyNamesUsed;

  // The XML Document.
  pugi::xml_document StateXML;

  std::string GetExposedPropertyName(int id, const std::string& pname) const
  {
    for (auto pair : this->ExposedPropertyNameMap)
    {
      const auto& tuple = pair.second;
      if (tuple.first == id &&
        (tuple.second == pname || tuple.second == vtkSMCoreUtilities::SanitizeName(pname)))
      {
        return pair.first;
      }
    }
    return std::string();
  }

  void Process(vtkSMLoadStateOptionsProxy* self)
  {
    auto pxm = self->GetSessionProxyManager();
    auto xpath_smstate = this->StateXML.select_node("//ServerManagerState");

    this->PropertiesMap.clear();
    this->ExposedPropertyNameMap.clear();
    this->ProxyNamesUsed.clear();

    // iterate over "Proxy" elements and find proxies/properties that have
    // file-list domains.
    // Let's build the `PropertiesMap` with information about that.
    for (auto proxy : xpath_smstate.node().children("Proxy"))
    {
      auto prototype =
        pxm->GetPrototypeProxy(proxy.attribute("group").value(), proxy.attribute("type").value());
      if (!prototype)
      {
        vtkLogF(TRACE, "failed to find prototype for proxy (%s, %s); skipping",
          proxy.attribute("group").value(), proxy.attribute("type").value());
        continue;
      }

      auto properties = vtkSMCoreUtilities::GetFileNameProperties(prototype);
      if (properties.empty())
      {
        continue;
      }

      // `proxyname` is the registration name used for the proxy in the state.
      // note, multiple proxies can have the same registration name.
      const auto proxyname = this->GetProxyRegistrationName(proxy);

      std::set<std::string> pset(properties.begin(), properties.end());
      for (auto property : proxy.children("Property"))
      {
        if (pset.find(property.attribute("name").value()) != pset.end())
        {
          PropertyInfo info;
          info.XMLElement = property;
          info.PopulateFilePaths();

          // if the proxy's registration name is based on the value of this property,
          // we flag it, so we can change it when it's modified.
          info.UpdateProxyName = (info.GetSequenceName() == proxyname ||
            info.GetFileName(/*woExtension=*/true) == proxyname);
          const auto pname = property.attribute("name").value();
          this->PropertiesMap[proxy.attribute("id").as_int()][pname] = info;
        }
      }

      // Let's expose properties on `self` for the file properties on the reader
      // from the state.
      this->AddProperties(self, proxy, properties);
    }
  }

  void UpdateStateXML()
  {
    for (auto& pair1 : this->PropertiesMap)
    {
      const auto id = pair1.first;
      for (auto& pair2 : pair1.second)
      {
        auto& pinfo = pair2.second;
        pinfo.Update();

        // if this property's value was used to set the registration name for this proxy,
        // let's update it to reflect the new files.
        if (pinfo.UpdateProxyName)
        {
          this->SetProxyName(id, pinfo.GetSequenceName());
        }
      }
    }
  }

  int GetId(const std::string& pname) const
  {
    const auto query =
      std::string("//ServerManagerState/ProxyCollection/Item[@name='") + pname + "']";
    if (auto xpath_node = this->StateXML.select_node(query.c_str()))
    {
      return xpath_node.node().attribute("id").as_int();
    }
    return 0;
  }

  std::string GetProxyRegistrationName(int id) const
  {
    const auto query =
      std::string("//ServerManagerState/ProxyCollection/Item[@id=") + std::to_string(id) + "]";
    if (auto xpath_node = this->StateXML.select_node(query.c_str()))
    {
      return xpath_node.node().attribute("name").value();
    }
    return std::to_string(id);
  }

private:
  void AddProperties(vtkSMLoadStateOptionsProxy* self, const pugi::xml_node& proxy,
    const std::vector<std::string>& fproperties)
  {
    const std::string subproxyname{ proxy.attribute("id").value() };

    auto pxm = self->GetSessionProxyManager();
    auto prototype =
      pxm->NewProxy(proxy.attribute("group").value(), proxy.attribute("type").value());
    prototype->PrototypeOn();
    prototype->SetLocation(0);
    prototype->LoadXMLState(vtkInternals::ConvertXML(proxy), nullptr);

    self->AddSubProxy(subproxyname.c_str(), prototype);
    prototype->FastDelete();

    // this is proxy's registration name; multiple proxies can have same
    // registration name.
    const auto proxyname = this->GetProxyRegistrationName(proxy);

    // Let's come up with a unique name for properties on the reader-proxy to use for
    // exposing them on `self`.
    const auto baseName = this->GetUniqueProxyName(proxyname);

    const int id = proxy.attribute("id").as_int();

    std::ostringstream str;
    str << proxyname
        << " ("
        /* << proxy.attribute("group").value() << ", "*/
        << proxy.attribute("type").value() << ") (id=" << id << ")";
    vtkNew<vtkSMPropertyGroup> group;
    group->SetXMLLabel(str.str().c_str());
    for (auto& pname : fproperties)
    {
      auto prop = prototype->GetProperty(pname.c_str());
      prop->SetPanelVisibility("default");
      prop->SetHints(nullptr); // remove any hints

      const auto exposedName = vtkSMCoreUtilities::SanitizeName((baseName + pname).c_str());

      self->ExposeSubProxyProperty(subproxyname.c_str(), pname.c_str(), exposedName.c_str());
      group->AddProperty(exposedName.c_str(), prop);

      this->ExposedPropertyNameMap[exposedName] = std::make_pair(id, pname);
    }

    // add hints for visibility stuff for the group.
    group->SetHints(this->GetPropertyGroupHints());
    self->AppendPropertyGroup(group);
  }

  std::string GetProxyRegistrationName(const pugi::xml_node& proxy) const
  {
    const auto query = std::string("//ServerManagerState/ProxyCollection/Item[@id=") +
      proxy.attribute("id").value() + "]";
    if (auto xpath_node = this->StateXML.select_node(query.c_str()))
    {
      return xpath_node.node().attribute("name").value();
    }
    return proxy.attribute("id").value();
  }

  void SetProxyName(int id, const std::string& name)
  {
    if (name.empty())
    {
      return;
    }
    const auto query =
      std::string("//ServerManagerState/ProxyCollection/Item[@id=") + std::to_string(id) + "]";
    if (auto xpath_node = this->StateXML.select_node(query.c_str()))
    {
      xpath_node.node().attribute("name") = name.c_str();
    }
  }

  vtkPVXMLElement* GetPropertyGroupHints()
  {
    if (this->Hints == nullptr)
    {
      //  <Hints>
      //    <PropertyWidgetDecorator type="GenericDecorator"
      //      mode="visibility"
      //      property="LoadStateDataFileOptions"
      //      value="2" />
      //  </Hints>
      this->Hints = vtkSmartPointer<vtkPVXMLElement>::New();
      this->Hints->SetName("Hints");

      vtkNew<vtkPVXMLElement> pwd;
      pwd->SetName("PropertyWidgetDecorator");
      pwd->SetAttribute("type", "GenericDecorator");
      pwd->SetAttribute("mode", "visibility");
      pwd->SetAttribute("property", "LoadStateDataFileOptions");
      pwd->SetAttribute("value", "2");
      this->Hints->AddNestedElement(pwd);
    }
    return this->Hints;
  }

  std::string GetUniqueProxyName(const std::string& proxyname)
  {
    // note: this logic is based on what old code was doing to avoid having
    // issues loading old Python scripts.

    // Let's come up with a unique name for properties on the reader-proxy to use for
    // exposing them on `self`.
    std::string baseName = proxyname;
    // Remove all '.'s in the name to avoid possible name collisions with the appended index
    baseName.erase(std::remove(baseName.begin(), baseName.end(), '.'), baseName.end());
    if (this->ProxyNamesUsed.find(baseName) != this->ProxyNamesUsed.end())
    {
      baseName.append(std::to_string(this->ProxyNamesUsed[baseName]++));
    }
    else
    {
      this->ProxyNamesUsed[baseName] = 1;
    }
    return baseName;
  }

public:
  static vtkSmartPointer<vtkPVXMLElement> ConvertXML(const pugi::xml_node& proxy)
  {
    std::ostringstream stream;
    proxy.print(stream);
    vtkNew<vtkPVXMLParser> parser;
    parser->Parse(stream.str().c_str());
    return parser->GetRootElement();
  }

  static std::string HandleSubstitution(const std::string& path)
  {
    std::vector<std::string> pathComponents;
    vtksys::SystemTools::SplitPath(path, pathComponents);
    std::string variablePath;
    if (vtksys::SystemTools::GetEnv(pathComponents[1].erase(0, 1), variablePath))
    {
      pathComponents.erase(pathComponents.begin(), pathComponents.begin() + 2);
      std::vector<std::string> variablePathComponents;
      vtksys::SystemTools::SplitPath(variablePath, variablePathComponents);
      pathComponents.insert(
        pathComponents.begin(), variablePathComponents.begin(), variablePathComponents.end());
      return vtksys::SystemTools::JoinPath(pathComponents);
    }
    else
    {
      vtkLogF(WARNING, "Environment variable '%s' is not set.", pathComponents[1].c_str());
    }
    return path;
  }
};

vtkStandardNewMacro(vtkSMLoadStateOptionsProxy);
//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::vtkSMLoadStateOptionsProxy()
{
  this->Internals = new vtkInternals();
  this->StateFileName = nullptr;
  this->PathMatchingThreshold = 3;
}

//----------------------------------------------------------------------------
vtkSMLoadStateOptionsProxy::~vtkSMLoadStateOptionsProxy()
{
  delete this->Internals;
  this->SetStateFileName(nullptr);
}

//----------------------------------------------------------------------------
namespace
{
// Replace all "$FOO" with environment variable values, if present. Otherwise
// they are left unchanged.
void ReplaceEnvironmentVariables(std::string& contents)
{
  std::map<std::string, std::string> varmap;
  vtksys::RegularExpression regex("\\$([a-zA-Z0-9_-]+)");
  size_t pos = 0;
  if (pos < contents.size() && regex.find(contents.c_str() + pos))
  {
    std::string envvalue;
    if (vtksys::SystemTools::GetEnv(regex.match(1), envvalue))
    {
      varmap.insert(std::make_pair(regex.match(0), envvalue));
    }
    pos = regex.end();
  }

  for (const auto& pair : varmap)
  {
    vtksys::SystemTools::ReplaceString(contents, pair.first, pair.second);
  }
}
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::PNGHasStateFile(
  const char* statefilename, std::string& contents, vtkTypeUInt32 location)
{
  const auto pxm = vtkSMProxyManager::GetProxyManager();
  auto proxy = vtkSMSourceProxy::SafeDownCast(pxm->NewProxy("sources", "PNGSeriesReader"));
  if (!proxy)
  {
    vtkErrorWithObjectMacro(nullptr, "Failed to create png proxy reader");
    return false;
  }
  proxy->SetLocation(location);
  vtkSMPropertyHelper(proxy, "FileNames").Set(statefilename);
  proxy->UpdateVTKObjects();
  proxy->UpdatePipeline();

  auto textKeysProp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("TextKeys"));
  auto textValuesProp = vtkSMStringVectorProperty::SafeDownCast(proxy->GetProperty("TextValues"));
  proxy->UpdatePropertyInformation(textKeysProp);
  proxy->UpdatePropertyInformation(textValuesProp);

  auto pngTextChunks = textKeysProp->GetNumberOfElements();
  bool stateFound = false;
  for (unsigned int i = 0; i < pngTextChunks; ++i)
  {
    if (strcmp(textKeysProp->GetElement(i), "ParaViewState") == 0)
    {
      contents = textValuesProp->GetElement(i);
      stateFound = true;
      break;
    }
  }
  proxy->Delete();
  return stateFound;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::PrepareToLoad(const char* statefilename, vtkTypeUInt32 location)
{
  const auto pxm = this->GetSession()->GetSessionProxyManager();
  if (!pxm)
  {
    vtkErrorWithObjectMacro(nullptr, "Proxy manager is invalid");
    return false;
  }
  this->SetStateFileName(statefilename);
  std::string contents;
  const auto fileNameExt = vtksys::SystemTools::GetFilenameLastExtension(statefilename);
  if (fileNameExt == ".png")
  {
    if (!vtkSMLoadStateOptionsProxy::PNGHasStateFile(statefilename, contents, location))
    {
      vtkErrorMacro("Failed to find state in png file '" << statefilename << "'.");
      return false;
    }
  }
  else
  {
    contents = pxm->LoadString(statefilename, location);
    if (contents.empty())
    {
      vtkErrorMacro("Failed to load state file '" << statefilename << "'.");
      return false;
    }
  }
  ::ReplaceEnvironmentVariables(contents);

  auto& internals = (*this->Internals);
  auto result = internals.StateXML.load_string(contents.c_str());
  if (!result)
  {
    vtkErrorMacro(
      "Error parsing state file XML from " << statefilename << ": " << result.description());
    return false;
  }

  internals.Process(this);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::HasDataFiles()
{
  // This will always return false until PrepareToLoad has been called
  return !this->Internals->PropertiesMap.empty();
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::LocateFilesInDirectory(
  std::vector<std::string>& filepaths, int path, bool clearFilenameIfNotFound)
{
  std::string lastLocatedPath;
  int numOfPathMatches = 0;
  std::vector<std::string>::iterator fIter;
  for (fIter = filepaths.begin(); fIter != filepaths.end(); ++fIter)
  {
    if (fIter->empty())
    {
      // don't attempt to fix empty strings (see paraview/paraview#19137).
      continue;
    }
    // TODO: Inefficient - need vtkPVInfomation class to bundle file paths
    if (numOfPathMatches < this->PathMatchingThreshold)
    {
      vtkClientServerStream stream;
      stream << vtkClientServerStream::Invoke << VTKOBJECT(this) << "LocateFileInDirectory"
             << *fIter << path << vtkClientServerStream::End;
      this->ExecuteStream(stream, false, vtkPVSession::DATA_SERVER_ROOT);
      vtkClientServerStream result = this->GetLastResult();
      std::string locatedPath;
      if (result.GetNumberOfMessages() == 1 && result.GetNumberOfArguments(0) == 1)
      {
        result.GetArgument(0, 0, &locatedPath);
      }

      if (!locatedPath.empty())
      {
        *fIter = locatedPath;
        if (vtksys::SystemTools::GetParentDirectory(locatedPath) ==
          vtksys::SystemTools::GetParentDirectory(lastLocatedPath))
        {
          numOfPathMatches++;
        }
        else
        {
          numOfPathMatches = 0;
        }
        lastLocatedPath = locatedPath;
      }
      else if (clearFilenameIfNotFound)
      {
        vtkErrorMacro(<< "Could not find file " << *fIter << " in data directory.");
        *fIter = "";
      }
      else
      {
        return false;
      }
    }
    else
    {
      std::vector<std::string> directoryPathComponents;
      vtksys::SystemTools::SplitPath(
        vtksys::SystemTools::GetParentDirectory(lastLocatedPath), directoryPathComponents);
      directoryPathComponents.push_back(vtksys::SystemTools::GetFilenameName(*fIter));
      *fIter = vtksys::SystemTools::JoinPath(directoryPathComponents);
    }
  }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::Load()
{
  SM_SCOPED_TRACE(LoadState).arg("filename", this->StateFileName).arg("options", this);
  this->DataFileOptions = vtkSMPropertyHelper(this, "LoadStateDataFileOptions").GetAsInt();
  this->OnlyUseFilesInDataDirectory =
    (vtkSMPropertyHelper(this, "OnlyUseFilesInDataDirectory").GetAsInt() == 1);

  auto& internals = (*this->Internals);
  switch (this->DataFileOptions)
  {
    case USE_FILES_FROM_STATE:
      // Nothing to do
      break;

    case USE_DATA_DIRECTORY:
      for (auto& pair : internals.PropertiesMap)
      {
        for (auto& pair2 : pair.second)
        {
          vtkInternals::PropertyInfo& info = pair2.second;
          if (pair2.first.find("FilePattern") == std::string::npos)
          {
            bool path = pair2.first.compare("FilePrefix") == 0;
            this->LocateFilesInDirectory(info.FilePaths, path, this->OnlyUseFilesInDataDirectory);
          }
        }
      }
      break;

    case CHOOSE_FILES_EXPLICITLY:
      for (auto& pair1 : internals.PropertiesMap)
      {
        vtkSMProxy* subProxy = this->GetSubProxy(std::to_string(pair1.first).c_str());
        for (auto& pair2 : pair1.second)
        {
          vtkInternals::PropertyInfo& info = pair2.second;
          vtkSMPropertyHelper helper(subProxy, info.GetPropertyXMLName().c_str());

          // First check if environment variable shows up in user specified path
          std::string propertyValue = helper.GetAsString();
          if (propertyValue.compare(0, 1, "$") == 0)
          {
            info.FilePaths = { vtkInternals::HandleSubstitution(propertyValue) };
          }
          else
          {
            info.FilePaths.clear();
            for (unsigned int cc = 0, max = helper.GetNumberOfElements(); cc < max; ++cc)
            {
              info.FilePaths.push_back(helper.GetAsString(cc));
            }
          }
        }
      }
      break;
  }

  // update State XML based on values from info.FilePaths for modified items.
  internals.UpdateStateXML();

  auto pxm = this->GetSessionProxyManager();
  pxm->LoadXMLState(vtkInternals::ConvertXML(internals.StateXML));
  return true;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMLoadStateOptionsProxy::FindProperty(const char* name, int id, const char* pname)
{
  if (pname == nullptr)
  {
    return nullptr;
  }

  const auto& internals = *this->Internals;
  if (id != 0)
  {
    const auto realname = internals.GetExposedPropertyName(id, pname);
    if (auto prop = this->GetProperty(realname.c_str()))
    {
      return prop;
    }
  }

  if (name != nullptr)
  {
    return this->FindProperty(nullptr, internals.GetId(name), pname);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMLoadStateOptionsProxy::FindLegacyProperty(const char* sanitizedName)
{
  if (sanitizedName == nullptr)
  {
    return nullptr;
  }

  auto& internals = (*this->Internals);
  if (internals.ExposedPropertyNameMap.find(sanitizedName) !=
    internals.ExposedPropertyNameMap.end())
  {
    return this->GetProperty(sanitizedName);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
std::string vtkSMLoadStateOptionsProxy::GetReaderName(int id) const
{
  auto& internals = (*this->Internals);
  return internals.GetProxyRegistrationName(id);
}

//----------------------------------------------------------------------------
bool vtkSMLoadStateOptionsProxy::IsPropertyModified(int id, const char* pname)
{
  auto& internals = (*this->Internals);
  auto iter = internals.PropertiesMap.find(id);
  if (iter != internals.PropertiesMap.end())
  {
    auto iter2 = iter->second.find(vtkSMCoreUtilities::SanitizeName(pname));
    if (iter2 != iter->second.end())
    {
      return iter2->second.IsModified();
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkSMLoadStateOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
