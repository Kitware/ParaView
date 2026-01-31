// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDisplayConfiguration.h"

#include "vtkObjectFactory.h"
#include "vtkStringScanner.h"

#include <sstream>
#include <string>
#include <vector>
#include <vtk_pugixml.h>

namespace
{
// Redefined from vtkRenderWindow.h
const int VTK_STEREO_CRYSTAL_EYES = 1;
const int VTK_STEREO_RED_BLUE = 2;
const int VTK_STEREO_INTERLACED = 3;
const int VTK_STEREO_LEFT = 4;
const int VTK_STEREO_RIGHT = 5;
const int VTK_STEREO_DRESDEN = 6;
const int VTK_STEREO_ANAGLYPH = 7;
const int VTK_STEREO_CHECKERBOARD = 8;
const int VTK_STEREO_SPLITVIEWPORT_HORIZONTAL = 9;
const int VTK_STEREO_FAKE = 10;
const int VTK_STEREO_EMULATE = 11;

// Default eye separation to match the proxy property definition default
constexpr double DEFAULT_EYE_SEPARATION = 0.065;
}

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
    bool Coverable = false;
    bool Show2DOverlays = true;
    int ViewerId = 0;
    int StereoType = -1;
    std::string Name;

    void Print(ostream& os, vtkIndent indent) const
    {
      os << indent << "Geometry: " << this->Geometry[0] << ", " << this->Geometry[1] << ", "
         << this->Geometry[2] << ", " << this->Geometry[3] << endl;
      os << indent << "HasCorners: " << this->HasCorners << endl;
      os << indent << "StereoType: " << this->StereoType << endl;
      os << indent << "Coverable: " << this->Coverable << endl;
      os << indent << "Show2DOverlays: " << this->Show2DOverlays << endl;
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
      auto result = vtk::scan<int, int, int, int>(value, "{:d}x{:d}+{:d}+{:d}");
      if (result)
      {
        std::tie(this->Geometry[2], this->Geometry[3], this->Geometry[0], this->Geometry[1]) =
          result->values();
      }
      return result.has_value();
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

  struct IndependentViewer
  {
    int ViewerId;
    double EyeSeparation;
  };

  std::vector<Item> Displays;
  std::vector<IndependentViewer> Viewers;
  std::map<int, size_t> MachineCountsById;
};

vtkStandardNewMacro(vtkDisplayConfiguration);
//----------------------------------------------------------------------------
vtkDisplayConfiguration::vtkDisplayConfiguration()
  : Internals(new vtkDisplayConfiguration::vtkInternals())
{
  ShowBorders = false;
  Coverable = false;
  FullScreen = false;
  EyeSeparation = DEFAULT_EYE_SEPARATION;
}

//----------------------------------------------------------------------------
vtkDisplayConfiguration::~vtkDisplayConfiguration() = default;

//----------------------------------------------------------------------------
int vtkDisplayConfiguration::ParseStereoType(const std::string& value)
{
  if (value == "Crystal Eyes")
  {
    return VTK_STEREO_CRYSTAL_EYES;
  }
  else if (value == "Red-Blue")
  {
    return VTK_STEREO_RED_BLUE;
  }
  else if (value == "Interlaced")
  {
    return VTK_STEREO_INTERLACED;
  }
  else if (value == "Left")
  {
    return VTK_STEREO_LEFT;
  }
  else if (value == "Right")
  {
    return VTK_STEREO_RIGHT;
  }
  else if (value == "Dresden")
  {
    return VTK_STEREO_DRESDEN;
  }
  else if (value == "Anaglyph")
  {
    return VTK_STEREO_ANAGLYPH;
  }
  else if (value == "Checkerboard")
  {
    return VTK_STEREO_CHECKERBOARD;
  }
  else if (value == "SplitViewportHorizontal")
  {
    return VTK_STEREO_SPLITVIEWPORT_HORIZONTAL;
  }
  return -1;
}

//----------------------------------------------------------------------------
const char* vtkDisplayConfiguration::GetStereoTypeAsString(int stereoType)
{
  switch (stereoType)
  {
    case VTK_STEREO_CRYSTAL_EYES:
      return "Crystal Eyes";
    case VTK_STEREO_RED_BLUE:
      return "Red-Blue";
    case VTK_STEREO_INTERLACED:
      return "Interlaced";
    case VTK_STEREO_LEFT:
      return "Left";
    case VTK_STEREO_RIGHT:
      return "Right";
    case VTK_STEREO_DRESDEN:
      return "Dresden";
    case VTK_STEREO_ANAGLYPH:
      return "Anaglyph";
    case VTK_STEREO_CHECKERBOARD:
      return "Checkerboard";
    case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      return "SplitViewportHorizontal";
  }
  return "";
}

//----------------------------------------------------------------------------
int vtkDisplayConfiguration::GetNumberOfDisplays() const
{
  const auto& internals = (*this->Internals);
  return static_cast<int>(internals.Displays.size());
}

//----------------------------------------------------------------------------
const char* vtkDisplayConfiguration::GetName(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.Name.empty() ? nullptr : config.Name.c_str();
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
bool vtkDisplayConfiguration::GetCoverable(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.Coverable;
}

//----------------------------------------------------------------------------
bool vtkDisplayConfiguration::GetShow2DOverlays(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.Show2DOverlays;
}

//----------------------------------------------------------------------------
int vtkDisplayConfiguration::GetViewerId(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.ViewerId;
}

//----------------------------------------------------------------------------
int vtkDisplayConfiguration::GetStereoType(int index) const
{
  const auto& internals = (*this->Internals);
  auto& config = internals.Displays.at(index);
  return config.StereoType;
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
int vtkDisplayConfiguration::GetNumberOfViewers() const
{
  const auto& internals = (*this->Internals);
  return static_cast<int>(internals.Viewers.size());
}

//----------------------------------------------------------------------------
int vtkDisplayConfiguration::GetId(int viewerIndex) const
{
  const auto& internals = (*this->Internals);
  auto& viewerConfig = internals.Viewers.at(viewerIndex);
  return viewerConfig.ViewerId;
}

//----------------------------------------------------------------------------
double vtkDisplayConfiguration::GetEyeSeparation(int viewerIndex) const
{
  const auto& internals = (*this->Internals);
  auto& viewerConfig = internals.Viewers.at(viewerIndex);
  return viewerConfig.EyeSeparation;
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

  // PARAVIEW_DEPRECATED_IN_6_1_0
  // Specifying a process is not needed and deprecated, warn user to remove it
  bool warnProcess = false;
  for (auto child : root.children("Process"))
  {
    warnProcess = true;

    // Use the last one OR the one of Type "server" if any.
    process = child;
    if (strcmp(child.attribute("Type").as_string(), "server") == 0)
    {
      break;
    }
  }

  // PARAVIEW_DEPRECATED_IN_6_1_0
  if (warnProcess)
  {
    vtkLogF(WARNING,
      "\"Process\" and \"Type\" specification is not needed in .pvx file and therefore deprecated, "
      "please remove that layer");
  }

  if (auto showBorders = process.select_node("/Option[@Name='ShowBorders']").node())
  {
    this->ShowBorders = showBorders.attribute("Value").as_bool(false);
  }

  if (auto fullscreen = process.select_node("/Option[@Name='FullScreen']").node())
  {
    this->FullScreen = fullscreen.attribute("Value").as_bool(false);
  }

  this->EyeSeparation =
    process.child("EyeSeparation").attribute("Value").as_double(DEFAULT_EYE_SEPARATION);

  this->UseOffAxisProjection =
    process.child("UseOffAxisProjection").attribute("Value").as_bool(true);

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

    info.Coverable = display.attribute("Coverable").as_bool();

    auto vidAttr = display.attribute("ViewerId");
    info.ViewerId = vidAttr.empty() ? 0 : display.attribute("ViewerId").as_int();

    if (internals.MachineCountsById.count(info.ViewerId) < 1)
    {
      internals.MachineCountsById[info.ViewerId] = 0;
    }

    internals.MachineCountsById[info.ViewerId] += 1;

    auto showAttr = display.attribute("Show2DOverlays");
    if (!showAttr.empty())
    {
      info.Show2DOverlays = showAttr.as_bool();
    }

    auto nameAttr = display.attribute("Name");
    if (!nameAttr.empty())
    {
      info.Name = nameAttr.as_string();
    }

    auto stereoTypeAttr = display.attribute("StereoType");
    if (!stereoTypeAttr.empty())
    {
      std::string sTypeStr = stereoTypeAttr.as_string("");
      info.StereoType = ParseStereoType(sTypeStr);
      if (info.StereoType < 0)
      {
        vtkWarningMacro(<< "Ignoring unrecognized stereo type value, " << sTypeStr.c_str()
                        << ", from .pvx file");
      }
    }

    internals.Displays.push_back(std::move(info));
  }

  for (int i = 0; i < internals.MachineCountsById.size(); ++i)
  {
    if (internals.MachineCountsById.count(i) != 1)
    {
      vtkErrorMacro(<< "If ViewerId attributes are provided, they must include all "
                    << "values from 0 to count - 1, and only those values.");
      return false;
    }
  }

  size_t numViewerIds = internals.MachineCountsById.size();
  internals.Viewers.clear();
  internals.Viewers.resize(numViewerIds);

  // Create a default independent viewer for each viewer id found in the Machine
  // elements (or just a single one for the implied ViewerId="0", if no Machine elements
  // specified a ViewerId)
  for (size_t i = 0; i < numViewerIds; ++i)
  {
    vtkInternals::IndependentViewer viewer;
    viewer.ViewerId = static_cast<int>(i);
    viewer.EyeSeparation = this->EyeSeparation;
    internals.Viewers[i] = viewer;
  }

  // Look for optional IndependentViewers element
  auto viewers = process.child("IndependentViewers");

  if (viewers)
  {
    // Viewer elements found here can override the default EyeSeparation for
    // any/all viewers.
    for (auto viewer : viewers.children("Viewer"))
    {
      auto idAttr = viewer.attribute("Id");
      auto eyeAttr = viewer.attribute("EyeSeparation");

      if (idAttr.empty() || eyeAttr.empty())
      {
        vtkWarningMacro("Viewer elements without both Id and EyeSeparation will be ignored");
        continue;
      }

      int viewerId = idAttr.as_int();

      if (viewerId < 0 || viewerId >= internals.Viewers.size())
      {
        vtkWarningMacro(<< "Id attributes of Viewer elements must correspond to ViewerId "
                        << "attributes of Machine elements, and " << viewerId << " does "
                        << "not correspond to any known ViewerId.");
        continue;
      }

      size_t viewerIndex = static_cast<size_t>(viewerId);
      internals.Viewers[viewerIndex].EyeSeparation = eyeAttr.as_double(this->EyeSeparation);
    }
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
