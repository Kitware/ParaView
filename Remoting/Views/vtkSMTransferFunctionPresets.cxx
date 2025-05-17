// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionPresetsBuiltin.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMSettings.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkTuple.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include "vtk_jsoncpp.h"

#include <cassert>
#include <list>
#include <memory>
#include <sstream>
#include <unordered_set>

vtkSmartPointer<vtkSMTransferFunctionPresets> vtkSMTransferFunctionPresets::Instance;

class vtkSMTransferFunctionPresets::vtkInternals
{
public:
  vtkInternals()
    : CustomPresetsLoaded(false)
  {
  }

  ~vtkInternals() = default;

  void SaveToSettings()
  {
    assert(this->CustomPresetsLoaded);
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    std::ostringstream stream;
    stream << "[\n";
    bool not_first = false;
    for (std::vector<Json::Value>::const_iterator iter = this->CustomPresets.begin();
         iter != this->CustomPresets.end(); ++iter)
    {
      if (not_first)
      {
        stream << ",\n";
      }
      not_first = true;
      stream << iter->toStyledString().c_str();
    }
    stream << "]";
    settings->SetSetting("TransferFunctionPresets.CustomPresets", stream.str());
  }

  void Reload()
  {
    this->Presets.clear();
    this->GetPresets();
  }

  const std::vector<Json::Value>& GetPresets()
  {
    if (this->Presets.empty())
    {
      this->LoadBuiltinPresets();
      this->LoadCustomPresets();
      this->Presets.insert(
        this->Presets.end(), this->BuiltinPresets.begin(), this->BuiltinPresets.end());
      this->Presets.insert(
        this->Presets.end(), this->CustomPresets.begin(), this->CustomPresets.end());
    }
    return this->Presets;
  }

  bool RemovePreset(unsigned int index)
  {
    this->GetPresets();
    if (index >= static_cast<unsigned int>(this->Presets.size()) ||
      index < static_cast<unsigned int>(this->BuiltinPresets.size()))
    {
      return false;
    }
    this->Presets.erase(this->Presets.begin() + index);
    index = (index - static_cast<unsigned int>(this->BuiltinPresets.size()));

    assert(this->CustomPresets.size() > index);
    this->CustomPresets.erase(this->CustomPresets.begin() + index);
    this->SaveToSettings();
    this->Presets.clear();
    return true;
  }

  void AddPreset(const char* name, const Json::Value& value)
  {
    this->LoadCustomPresets();
    this->CustomPresets.push_back(value);
    this->CustomPresets.back()["Name"] = name;
    this->SaveToSettings();
    this->Reload();
  }

  bool IsPresetBuiltin(unsigned int index)
  {
    this->LoadBuiltinPresets();
    return index < static_cast<unsigned int>(this->BuiltinPresets.size());
  }

  bool RenamePreset(unsigned int index, const char* newname)
  {
    if (newname && newname[0])
    {
      // Check that the newname is not already used
      if (std::any_of(this->Presets.begin(), this->Presets.end(),
            [newname](Json::Value const& preset)
            { return preset.get("Name", "").asString() == newname; }))
      {
        return false;
      }

      const std::vector<Json::Value>& presets = this->GetPresets();
      if (index < static_cast<unsigned int>(presets.size()) &&
        index >= static_cast<unsigned int>(this->BuiltinPresets.size()))
      {
        this->Presets[index]["Name"] = newname;

        index = (index - static_cast<unsigned int>(this->BuiltinPresets.size()));
        assert(this->CustomPresets.size() > index);

        this->CustomPresets[index]["Name"] = newname;
        this->SaveToSettings();
        return true;
      }
    }
    return false;
  }

  bool ImportPresets(std::string contents, std::vector<ImportedPreset>* importedPresets)
  {
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value root;
    std::istringstream inputStream(contents);
    if (!parseFromStream(builder, inputStream, &root, nullptr))
    {
      vtkGenericWarningMacro("File is not is a format suitable for presets");
      return false;
    }
    if (!root.isArray())
    {
      vtkGenericWarningMacro("File may not contain presets");
      return false;
    }

    std::unordered_set<std::string> presetNames;
    presetNames.reserve(this->Presets.size());
    std::transform(this->Presets.begin(), this->Presets.end(),
      std::inserter(presetNames, presetNames.end()),
      [](Json::Value const& preset) { return preset.get("Name", Json::Value()).asString(); });

    for (Json::Value& preset : root)
    {
      const std::string basename = preset.get("Name", "Preset").asString();

      const std::string name = this->FindUniquePresetNameWithinPresets(basename, presetNames);

      preset["Name"] = name;
      presetNames.insert(name);
    }

    return this->ImportPresets(root, importedPresets);
  }

  bool ImportPresets(const Json::Value& root,
    std::vector<vtkSMTransferFunctionPresets::ImportedPreset>* importedPresets)
  {
    if (importedPresets != nullptr)
    {
      // The root is an array of presets that contain a "Name" and "Groups" fields that are
      // extracted to fill the importedPresets vector.
      std::transform(root.begin(), root.end(), std::back_inserter(*importedPresets),
        [&](Json::Value const& preset)
        {
          auto result = vtkSMTransferFunctionPresets::ImportedPreset{};
          result.name = preset.get("Name", "").asString();
          auto groups = preset.get("Groups", Json::Value());
          // This checks if the "Groups" field is an array, but also that it is present to make the
          // distinction between an empty "Groups" field which means the user wants the preset to be
          // added to no group, and a non-present "Groups" field which means the preset will be
          // added to default groups, "Default" and "User" at the time this comment is written.
          if (groups.isArray())
          {
            result.potentialGroups.isValid = true;
            std::transform(groups.begin(), groups.end(),
              std::back_inserter(result.potentialGroups.groups),
              [&](Json::Value const& groupName) { return groupName.asString(); });
          }
          else
          {
            result.potentialGroups.isValid = false;
          }
          return result;
        });
    }
    this->LoadCustomPresets();
    this->CustomPresets.insert(this->CustomPresets.end(), root.begin(), root.end());
    this->SaveToSettings();
    this->Reload();
    return true;
  }

  // Adds a suffix to basename to make sure the preset name is unique
  std::string FindUniquePresetName(std::string const& basename)
  {
    std::unordered_set<std::string> presetNames;
    presetNames.reserve(this->Presets.size());
    std::transform(this->Presets.begin(), this->Presets.end(),
      std::inserter(presetNames, presetNames.end()),
      [](Json::Value const& preset) { return preset.get("Name", Json::Value()).asString(); });
    return this->FindUniquePresetNameWithinPresets(basename, presetNames);
  }

  // Same as FindUniquePresetName, but takes the unordered_set containing all preset names, when the
  // caller wants to avoid creating it on each call for performances sake
  std::string FindUniquePresetNameWithinPresets(
    std::string const& basename, std::unordered_set<std::string> const& presetNames)
  {
    std::string const _basename = !basename.empty() ? basename : "Preset";
    std::string name = _basename;
    std::string separator = " ";
    int suffix = 0;

    while (presetNames.find(_basename) != presetNames.end())
    {
      std::ostringstream stream;
      stream << _basename << separator.c_str() << suffix;
      name = stream.str();
      suffix++;
      if (suffix > 1000)
      {
        vtkGenericWarningMacro(<< "Giving up. Cannot find a unique name for '" << _basename
                               << "'. Please provide a good prefix.");
        return std::string();
      }
    }
    return name;
  }

private:
  std::vector<Json::Value> BuiltinPresets;
  std::vector<Json::Value> CustomPresets;
  std::vector<Json::Value> Presets;
  bool CustomPresetsLoaded;

  void LoadBuiltinPresets()
  {
    if (this->BuiltinPresets.empty() == false)
    {
      return;
    }
    char* rawJSON = vtkSMTransferFunctionPresetsColorMapsJSON();
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string formattedErrors;
    Json::Value value;
    if (!reader->parse(rawJSON, rawJSON + strlen(rawJSON), &value, &formattedErrors))
    {
      vtkGenericWarningMacro(<< "Failed to parse builtin transfer function presets: "
                             << formattedErrors);
    }
    delete[] rawJSON;
    for (auto const& preset : value)
    {
      this->BuiltinPresets.push_back(preset);
    }
  }

  void LoadCustomPresets()
  {
    if (this->CustomPresetsLoaded)
    {
      return;
    }
    this->CustomPresetsLoaded = true;

    const char* const settingsKey = "TransferFunctionPresets.CustomPresets";
    vtkSMSettings* settings = vtkSMSettings::GetInstance();
    if (settings == nullptr || !settings->HasSetting(settingsKey))
    {
      return;
    }

    std::string presetJSON = settings->GetSettingAsString(settingsKey, "");
    const char* input = presetJSON.c_str();
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string formattedErrors;
    Json::Value value;
    if (!presetJSON.empty() &&
      !reader->parse(input, input + strlen(input), &value, &formattedErrors))
    {
      vtkGenericWarningMacro(<< "Failed to parse custom transfer function presets: "
                             << formattedErrors);
    }
    for (auto const& preset : value)
    {
      this->CustomPresets.push_back(preset);
    }
  }
};

vtkSMTransferFunctionPresets* vtkSMTransferFunctionPresets::New()
{
  auto instance = vtkSMTransferFunctionPresets::GetInstance();
  instance->Register(nullptr);
  return instance;
}

//----------------------------------------------------------------------------
vtkSMTransferFunctionPresets::vtkSMTransferFunctionPresets()
  : Internals(new vtkSMTransferFunctionPresets::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMTransferFunctionPresets::~vtkSMTransferFunctionPresets()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
vtkSMTransferFunctionPresets* vtkSMTransferFunctionPresets::GetInstance()
{
  if (vtkSMTransferFunctionPresets::Instance.GetPointer() == nullptr)
  {
    auto presets = new vtkSMTransferFunctionPresets();
    presets->InitializeObjectBase();
    vtkSMTransferFunctionPresets::Instance.TakeReference(presets);
  }

  return Instance;
}

//----------------------------------------------------------------------------
std::string vtkSMTransferFunctionPresets::GetPresetAsString(unsigned int index)
{
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  return index < static_cast<unsigned int>(presets.size()) ? presets[index].toStyledString()
                                                           : std::string();
}

//----------------------------------------------------------------------------
const Json::Value& vtkSMTransferFunctionPresets::GetPreset(unsigned int index)
{
  static Json::Value nullValue;
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  return index < static_cast<unsigned int>(presets.size()) ? presets[index] : nullValue;
}

//----------------------------------------------------------------------------
const Json::Value& vtkSMTransferFunctionPresets::GetFirstPresetWithName(const char* name, int& idx)
{
  static Json::Value nullValue;
  if (name == nullptr)
  {
    return nullValue;
  }
  idx = 0;
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  for (std::vector<Json::Value>::const_iterator iter = presets.begin(); iter != presets.end();
       ++iter)
  {
    if (iter->get("Name", Json::Value()).asString() == name)
    {
      return (*iter);
    }
    idx++;
  }

  idx = -1;
  return nullValue;
}

//----------------------------------------------------------------------------
const Json::Value& vtkSMTransferFunctionPresets::GetFirstPresetWithName(const char* name)
{
  int idx;
  return this->GetFirstPresetWithName(name, idx);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::HasPreset(const char* name)
{
  return !this->GetFirstPresetWithName(name).isNull();
}

//----------------------------------------------------------------------------
std::string vtkSMTransferFunctionPresets::GetPresetName(unsigned int index)
{
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  return index < static_cast<unsigned int>(presets.size()) ? presets[index]["Name"].asString()
                                                           : std::string();
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::RenamePreset(unsigned int index, const char* newname)
{
  return this->Internals->RenamePreset(index, newname);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::AddPreset(const char* name, const std::string& preset)
{
  if (!name)
  {
    vtkErrorMacro("Invalid 'name' specified.");
    return false;
  }

  Json::Value value;
  const char* input = preset.c_str();
  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  std::string formattedErrors;
  if (!reader->parse(input, input + strlen(input), &value, &formattedErrors))
  {
    vtkErrorMacro("Invalid preset string.");
    vtkGenericWarningMacro(<< "Failed to parse builtin transfer function presets: "
                           << formattedErrors);
    return false;
  }
  this->Internals->AddPreset(name, value);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::SetPreset(const char* name, const Json::Value& preset)
{
  int idx;
  this->GetFirstPresetWithName(name, idx);
  if (idx < 0 || this->IsPresetBuiltin(idx))
  {
    return false;
  }

  if (idx >= 0)
  {
    this->RemovePreset(idx);
  }
  return this->AddPreset(name, preset);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::AddPreset(const char* name, const Json::Value& preset)
{
  this->Internals->AddPreset(name, preset);
  return true;
}

//----------------------------------------------------------------------------
std::string vtkSMTransferFunctionPresets::AddUniquePreset(
  const Json::Value& preset, const char* prefix /*=nullptr*/)
{
  prefix = prefix ? prefix : "Preset";

  std::string name = this->Internals->FindUniquePresetName(prefix);
  if (name.empty())
  {
    return name;
  }

  this->AddPreset(name.c_str(), preset);
  return name;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::RemovePreset(unsigned int index)
{
  return this->Internals->RemovePreset(index);
}

//----------------------------------------------------------------------------
unsigned int vtkSMTransferFunctionPresets::GetNumberOfPresets()
{
  return static_cast<unsigned int>(this->Internals->GetPresets().size());
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::GetPresetHasOpacities(const Json::Value& preset)
{
  return (!preset.empty() && preset.isMember("Points"));
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::GetPresetHasIndexedColors(const Json::Value& preset)
{
  return (!preset.empty() && preset.isMember("IndexedColors"));
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::GetPresetHasAnnotations(const Json::Value& preset)
{
  return (!preset.empty() && preset.isMember("Annotations"));
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::IsPresetBuiltin(unsigned int index)
{
  if (index >= this->GetNumberOfPresets())
  {
    vtkWarningMacro("Invalid 'index' specified:" << index);
    return false;
  }

  return this->Internals->IsPresetBuiltin(index);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::ImportPresets(const char* filename, vtkTypeUInt32 location)
{
  return this->ImportPresets(filename, nullptr, location);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::ImportPresets(
  const char* filename, std::vector<ImportedPreset>* importedPresets, vtkTypeUInt32 location)
{
  if (!filename)
  {
    vtkErrorMacro("Invalid filename specified.");
    return false;
  }

  SM_SCOPED_TRACE(CallFunction)
    .arg("ImportPresets")
    .arg("filename", filename)
    .arg("location", static_cast<int>(location));
  const auto ext =
    vtksys::SystemTools::LowerCase(vtksys::SystemTools::GetFilenameLastExtension(filename));
  std::string contents;
  if (auto activePxm = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager())
  {
    contents = activePxm->LoadString(filename, location);
  }
  else
  {
    vtkNew<vtkSMSession> session;
    auto pxm = session->GetSessionProxyManager();
    contents = pxm->LoadString(filename, location);
  }
  if (ext == ".xml")
  {
    if (!contents.empty())
    {
      return this->ImportPresets(
        vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(contents.c_str()));
    }
    else
    {
      vtkErrorMacro("Failed to open file: " << filename);
      return false;
    }
  }
  else if (ext == ".ct")
  {
    if (!contents.empty())
    {
      auto converted = vtkSMTransferFunctionProxy::ConvertVisItColorMapXMLToJSON(contents.c_str());
      if (!converted.empty())
      {
        // VisIt colormaps are named only by their filename
        converted["Name"] = vtksys::SystemTools::GetFilenameWithoutLastExtension(filename);
        Json::Value root(Json::arrayValue);
        root.append(converted);
        return this->ImportPresets(root);
      }
      return false;
    }
    else
    {
      vtkErrorMacro("Failed to open file: " << filename);
      return false;
    }
  }
  else
  {
    return this->Internals->ImportPresets(contents, importedPresets);
  }
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::ImportPresets(const Json::Value& presets)
{
  return this->ImportPresets(presets, nullptr);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::ImportPresets(
  const Json::Value& presets, std::vector<ImportedPreset>* importedPresets)
{
  return this->Internals->ImportPresets(presets, importedPresets);
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionPresets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionPresets::ReloadPresets()
{
  this->Internals->Reload();
}
