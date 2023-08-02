// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDisplayConfiguration.h"

#include "vtkObjectFactory.h"

#include <sstream>
#include <string>
#include <vector>
#include <vtk_pugixml.h>

class vtkDisplayConfiguration::vtkInternals
{
public:
  struct Item
  {
    vtkTuple<int, 4> Geometry{ 0 };
    vtkTuple<double, 3> LowerLeft{ 0.0 };
    vtkTuple<double, 3> LowerRight{ 0.0 };
    vtkTuple<double, 3> UpperRight{ 0.0 };
    std::string Environment;
    bool HasCorners = false;

    void Print(ostream& os, vtkIndent indent) const
    {
      os << indent << "Geometry: " << this->Geometry[0] << ", " << this->Geometry[1] << ", "
         << this->Geometry[2] << ", " << this->Geometry[3] << endl;
      os << indent << "HasCorners: " << this->HasCorners << endl;
      os << indent << "LoweLeft: " << this->LowerLeft[0] << ", " << this->LowerLeft[1] << ", "
         << this->LowerLeft[2] << endl;
      os << indent << "LowerRight: " << this->LowerRight[0] << ", " << this->LowerRight[1] << ", "
         << this->LowerRight[2] << endl;
      os << indent << "UpperRight: " << this->UpperRight[0] << ", " << this->UpperRight[1] << ", "
         << this->UpperRight[2] << endl;
      os << indent << "Environment: " << this->Environment.c_str() << endl;
    }

    bool SetGeometry(const std::string& value)
    {
      this->Geometry = vtkTuple<int, 4>(0);
      if (value.empty())
      {
        return true;
      }
      int matches = sscanf(value.c_str(), "%dx%d+%d+%d", &this->Geometry[2], &this->Geometry[3],
        &this->Geometry[0], &this->Geometry[1]);
      return (matches == 4);
    }

    bool SetCorners(const std::string& ll, const std::string& lr, const std::string& ur)
    {
      this->HasCorners = Item::ScanTuple3(ll, this->LowerLeft) &&
        Item::ScanTuple3(lr, this->LowerRight) && Item::ScanTuple3(ur, this->UpperRight);
      if (!this->HasCorners)
      {
        this->LowerLeft = this->LowerRight = this->UpperRight = vtkTuple<double, 3>(0.0);
      }
      return this->HasCorners;
    }

  private:
    static bool ScanTuple3(const std::string& value, vtkTuple<double, 3>& tuple)
    {
      if (value.empty())
      {
        return false;
      }
      std::istringstream str(const_cast<char*>(value.c_str()));
      str >> tuple[0];
      str >> tuple[1];
      str >> tuple[2];
      return true;
    }
  };

  std::vector<Item> Displays;
};

vtkStandardNewMacro(vtkDisplayConfiguration);
//----------------------------------------------------------------------------
vtkDisplayConfiguration::vtkDisplayConfiguration()
  : Internals(new vtkDisplayConfiguration::vtkInternals())
{
}

//----------------------------------------------------------------------------
vtkDisplayConfiguration::~vtkDisplayConfiguration() = default;

//----------------------------------------------------------------------------
int vtkDisplayConfiguration::GetNumberOfDisplays() const
{
  const auto& internals = (*this->Internals);
  return static_cast<int>(internals.Displays.size());
}

//----------------------------------------------------------------------------
const char* vtkDisplayConfiguration::GetEnvironment(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.Environment.empty() ? nullptr : config.Environment.c_str();
}

//----------------------------------------------------------------------------
vtkTuple<int, 4> vtkDisplayConfiguration::GetGeometry(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.Geometry;
}

//----------------------------------------------------------------------------
bool vtkDisplayConfiguration::GetHasCorners(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.HasCorners;
}

//----------------------------------------------------------------------------
vtkTuple<double, 3> vtkDisplayConfiguration::GetLowerLeft(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.LowerLeft;
}

//----------------------------------------------------------------------------
vtkTuple<double, 3> vtkDisplayConfiguration::GetLowerRight(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.LowerRight;
}

//----------------------------------------------------------------------------
vtkTuple<double, 3> vtkDisplayConfiguration::GetUpperRight(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.UpperRight;
}

//----------------------------------------------------------------------------
bool vtkDisplayConfiguration::LoadPVX(const char* fname)
{
  pugi::xml_document doc;
  auto result = doc.load_file(fname);
  if (!result)
  {
    vtkErrorMacro("Failed to parse XML: " << fname << endl << result.description());
    return false;
  }

  auto root = doc.child("pvx");
  auto process = root;

  // if `Process` nodes exist, find the one which says "server" since that's the
  // only one where Cave config was specified.
  for (auto child : root.children("Process"))
  {
    if (strcmp(child.attribute("Type").as_string(), "server") == 0)
    {
      process = child;
      break;
    }
  }

  if (auto showBorders = process.select_node("/Option[@Name='ShowBorders']").node())
  {
    this->ShowBorders = showBorders.attribute("Value").as_bool(false);
  }

  if (auto fullscreen = process.select_node("/Option[@Name='FullScreen']").node())
  {
    this->FullScreen = fullscreen.attribute("Value").as_bool(false);
  }

  this->EyeSeparation = process.child("EyeSeparation").attribute("Value").as_double(0.0);

  auto& internals = (*this->Internals);
  internals.Displays.clear();
  for (auto display : process.children("Machine"))
  {
    vtkInternals::Item info;
    info.Environment = display.attribute("Environment").as_string();
    if (!info.SetGeometry(display.attribute("Geometry").as_string()))
    {
      vtkErrorMacro("Malformed geometry specification '"
        << display.attribute("Geometry").as_string() << "' (expected <width>x<height>+<X>+<Y>).");
    }

    info.SetCorners(display.attribute("LowerLeft").as_string(),
      display.attribute("LowerRight").as_string(), display.attribute("UpperRight").as_string());

    internals.Displays.push_back(std::move(info));
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkDisplayConfiguration::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ShowBorders: " << this->ShowBorders << endl;
  os << indent << "FullScreen: " << this->FullScreen << endl;

  const auto& internals = (*this->Internals);
  os << indent << "Displays (count=" << internals.Displays.size() << "): " << endl;
  for (auto& item : internals.Displays)
  {
    item.Print(os, indent.GetNextIndent());
  }
}
