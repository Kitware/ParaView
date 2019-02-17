/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTransferFunctionPresets.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTransferFunctionPresets.h"
#include "vtkSMTransferFunctionPresetsBuiltin.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMSettings.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionProxy.h"
#include "vtkTuple.h"

#include "vtk_jsoncpp.h"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <list>
#include <memory>
#include <set>
#include <sstream>

class vtkSMTransferFunctionPresets::vtkInternals
{
public:
  vtkInternals()
    : Modified(false)
    , CustomPresetsLoaded(false)
  {
  }

  ~vtkInternals()
  {
    if (this->Modified)
    {
      assert(this->CustomPresetsLoaded);
      if (vtkSMSettings* settings = vtkSMSettings::GetInstance())
      {
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
    }
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

    this->Modified = true;
    assert(this->CustomPresets.size() > index);
    this->CustomPresets.erase(this->CustomPresets.begin() + index);
    return true;
  }

  void AddPreset(const char* name, const Json::Value& value)
  {
    this->LoadCustomPresets();
    this->CustomPresets.push_back(value);
    this->CustomPresets.back()["Name"] = name;
    this->Presets.clear();
    this->Modified = true;
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
      const std::vector<Json::Value>& presets = this->GetPresets();
      if (index < static_cast<unsigned int>(presets.size()) &&
        index >= static_cast<unsigned int>(this->BuiltinPresets.size()))
      {
        index = (index - static_cast<unsigned int>(this->BuiltinPresets.size()));
        assert(this->CustomPresets.size() > index);

        this->CustomPresets[index]["Name"] = newname;
        this->Presets.clear();
        this->Modified = true;
        return true;
      }
    }
    return false;
  }

  bool ImportPresets(const char* filename)
  {
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::Value root;
    ifstream file;
    file.open(filename);
    if (!file)
    {
      vtkGenericWarningMacro("Failed to open file: " << filename);
      return false;
    }
    if (!parseFromStream(builder, file, &root, nullptr))
    {
      vtkGenericWarningMacro("File is not is a format suitable for presets: " << filename);
      return false;
    }
    if (!root.isArray())
    {
      vtkGenericWarningMacro("File may not contain presets: " << filename);
      return false;
    }
    return this->ImportPresets(root);
  }

  bool ImportPresets(const Json::Value& root)
  {
    this->LoadCustomPresets();
    this->CustomPresets.insert(this->CustomPresets.end(), root.begin(), root.end());
    this->Modified = true;
    this->Presets.clear();
    return true;
  }

private:
  std::vector<Json::Value> BuiltinPresets;
  std::vector<Json::Value> CustomPresets;
  std::vector<Json::Value> Presets;
  bool Modified;
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
    this->BuiltinPresets.insert(this->BuiltinPresets.end(), value.begin(), value.end());
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
    if (settings == NULL || !settings->HasSetting(settingsKey))
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
    this->CustomPresets.insert(this->CustomPresets.end(), value.begin(), value.end());
  }
};

vtkStandardNewMacro(vtkSMTransferFunctionPresets);
//----------------------------------------------------------------------------
vtkSMTransferFunctionPresets::vtkSMTransferFunctionPresets()
  : Internals(new vtkSMTransferFunctionPresets::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkSMTransferFunctionPresets::~vtkSMTransferFunctionPresets()
{
  delete this->Internals;
  this->Internals = NULL;
}

//----------------------------------------------------------------------------
vtkStdString vtkSMTransferFunctionPresets::GetPresetAsString(unsigned int index)
{
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  return index < static_cast<unsigned int>(presets.size()) ? presets[index].toStyledString()
                                                           : vtkStdString();
}

//----------------------------------------------------------------------------
const Json::Value& vtkSMTransferFunctionPresets::GetPreset(unsigned int index)
{
  static Json::Value nullValue;
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  return index < static_cast<unsigned int>(presets.size()) ? presets[index] : nullValue;
}

//----------------------------------------------------------------------------
const Json::Value& vtkSMTransferFunctionPresets::GetFirstPresetWithName(const char* name)
{
  static Json::Value nullValue;
  if (name == NULL)
  {
    return nullValue;
  }
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  for (std::vector<Json::Value>::const_iterator iter = presets.begin(); iter != presets.end();
       ++iter)
  {
    if (iter->get("Name", Json::Value()).asString() == name)
    {
      return (*iter);
    }
  }
  return nullValue;
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::HasPreset(const char* name)
{
  return !this->GetFirstPresetWithName(name).isNull();
}

//----------------------------------------------------------------------------
vtkStdString vtkSMTransferFunctionPresets::GetPresetName(unsigned int index)
{
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  return index < static_cast<unsigned int>(presets.size()) ? presets[index]["Name"].asString()
                                                           : vtkStdString();
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::RenamePreset(unsigned int index, const char* newname)
{
  return this->Internals->RenamePreset(index, newname);
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::AddPreset(const char* name, const vtkStdString& preset)
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
bool vtkSMTransferFunctionPresets::AddPreset(const char* name, const Json::Value& preset)
{
  this->Internals->AddPreset(name, preset);
  return true;
}

//----------------------------------------------------------------------------
vtkStdString vtkSMTransferFunctionPresets::AddUniquePreset(
  const Json::Value& preset, const char* prefix /*=NULL*/)
{
  prefix = prefix ? prefix : "Preset";

  std::set<std::string> names;
  const std::vector<Json::Value>& presets = this->Internals->GetPresets();
  for (std::vector<Json::Value>::const_iterator iter = presets.begin(); iter != presets.end();
       ++iter)
  {
    names.insert(iter->get("Name", Json::Value()).asString());
  }

  std::string name = prefix;
  std::string separator = name.size() > 0 ? " " : "";
  int suffix = 0;
  while (names.find(name) != names.end())
  {
    std::ostringstream stream;
    stream << prefix << separator.c_str() << suffix;
    name = stream.str();
    suffix++;
    if (suffix > 1000)
    {
      vtkErrorMacro("Giving up. Cannot find a unique name. Please provide a good prefix.");
      return vtkStdString();
    }
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
bool vtkSMTransferFunctionPresets::IsPresetDefault(const Json::Value& preset)
{
  if (preset.empty() || !preset.isMember("DefaultMap"))
  {
    return false;
  }
  return preset["DefaultMap"].asBool();
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
bool vtkSMTransferFunctionPresets::ImportPresets(const char* filename)
{
  if (!filename)
  {
    vtkErrorMacro("Invalid filename specified.");
    return false;
  }

  SM_SCOPED_TRACE(CallFunction).arg("ImportPresets").arg("filename", filename);
  if (vtksys::SystemTools::LowerCase(vtksys::SystemTools::GetFilenameLastExtension(filename)) ==
    ".xml")
  {
    ifstream in(filename);
    if (in)
    {
      std::ostringstream contents;
      contents << in.rdbuf();
      in.close();
      return this->ImportPresets(
        vtkSMTransferFunctionProxy::ConvertMultipleLegacyColorMapXMLToJSON(contents.str().c_str()));
    }
    else
    {
      vtkErrorMacro("Failed to open file: " << filename);
      return false;
    }
  }
  else
  {
    return this->Internals->ImportPresets(filename);
  }
}

//----------------------------------------------------------------------------
bool vtkSMTransferFunctionPresets::ImportPresets(const Json::Value& presets)
{
  return this->Internals->ImportPresets(presets);
}

//----------------------------------------------------------------------------
void vtkSMTransferFunctionPresets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
