// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotlyJsonExporter.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPlotPoints.h" // for vtkPlotPoints
#include "vtk_nlohmannjson.h"
// clang-format off
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

using vtkNJson = nlohmann::json;

namespace
{
void AddArray(vtkNJson& parent, const std::string& key, vtkAbstractArray* array, const char* name)
{
  if (auto* dpArray = vtkDoubleArray::SafeDownCast(array))
  {
    std::vector<double> vec(dpArray->Begin(), dpArray->End());
    parent["name"] = name;
    parent[key] = std::move(vec);
  }
  else
  {
    vtkLogF(WARNING, "Array type is %s is not yet supported by vtkPlotlyJsonExporter",
      array->GetArrayTypeAsString());
  }
}
}

class vtkPlotlyJsonExporter::vtkInternals
{
public:
  vtkNJson Data;

  std::fstream File;
};

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkPlotlyJsonExporter);

//----------------------------------------------------------------------------
vtkPlotlyJsonExporter::vtkPlotlyJsonExporter()
{
  this->Internals = new vtkInternals;
}

//----------------------------------------------------------------------------
vtkPlotlyJsonExporter::~vtkPlotlyJsonExporter()
{
  delete this->Internals;
  this->SetFileName(nullptr);
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPlotlyJsonExporter::Open(ExporterModes mode)
{
  auto& internals = *(this->Internals);
  // setup basic stricture
  internals.Data["data"] = vtkNJson::array();
  internals.Data["layout"] = vtkNJson::object();
  if (mode == ExporterModes::STREAM_COLUMNS)
  {
    internals.File.open(this->GetFileName());
    return true;
  }
  else
  {
    vtkWarningMacro("STREAM_ROW not yet implemented for vtkPlotlyJsonExporter");
    return false;
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::Close()
{
  auto& internals = *(this->Internals);
  internals.File << internals.Data;
  internals.File.close();
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::Abort()
{
  auto& internals = *(this->Internals);
  this->Close();
  vtksys::SystemTools::RemoveFile(this->FileName);
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::WriteHeader(vtkFieldData*)
{
  vtkLogF(ERROR, "STREAM_ROWS mode is not supported for vtkPlotlyJsonExporter");
}
//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::WriteData(vtkFieldData*)
{
  vtkLogF(ERROR, "STREAM_ROWS mode is not supported for vtkPlotlyJsonExporter");
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::AddColumn(
  vtkAbstractArray* yarray, const char* yarrayname, vtkDataArray* xarray)
{
  auto& internals = *(this->Internals);
  auto dataField = vtkNJson::object();
  AddArray(dataField, "y", yarray, yarrayname);
  if (xarray)
  {
    AddArray(dataField, "x", xarray, xarray->GetName());
  }
  else
  {
    vtkWarningMacro("null xarray not supported yet");
  }
  internals.Data["data"].emplace_back(dataField);
}
//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::AddStyle(vtkPlot* plot, const char* plotName)
{
  const std::map<int, std::string> VTK_LINE_STYLES_TO_PLOTLY = {
    { vtkPen::SOLID_LINE, "solid" },
    { vtkPen::DASH_LINE, "dash" },
    { vtkPen::DOT_LINE, "dot" },
    { vtkPen::DASH_DOT_LINE, "dashdot" },
    // fallback to solid for unsupported types
    { vtkPen::DASH_DOT_DOT_LINE, "solid" },
    { vtkPen::DENSE_DOT_LINE, "solid" },
  };

  const std::map<int, std::string> VTK_MARKER_STYLES_TO_PLOTLY = {
    { vtkPlotPoints::NONE, "" },
    { vtkPlotPoints::CROSS, "x" },
    { vtkPlotPoints::PLUS, "cross" },
    { vtkPlotPoints::SQUARE, "square" },
    { vtkPlotPoints::CIRCLE, "circle" },
    { vtkPlotPoints::DIAMOND, "diamond" },
  };

  auto& internals = *(this->Internals);
  auto& data = internals.Data["data"];
  auto iter = std::find_if(data.begin(), data.end(), [&plotName](const vtkNJson& object) {
    auto iter = object.find(plotName);
    return iter != object.end() && iter.value() == std::string(plotName);
  });

  if (iter == data.end())
  {
    vtkLogF(WARNING, "plot \"%s\" has no associated data", plotName);
    return;
  }

  auto& node = (*iter);
  // line style
  auto& line = node["line"];
  double rgb[3];
  plot->GetColorF(rgb);
  line["color"] = "rgb(" + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) + "," +
    std::to_string(rgb[2]) + ")";
  line["dash"] = VTK_LINE_STYLES_TO_PLOTLY.at(plot->GetPen()->GetLineType());
  // marker style if any
  if (vtkPlotPoints* plotPoints = vtkPlotPoints::SafeDownCast(plot))
  {
    auto& marker = node["marker"];
    marker["symbol"] = VTK_MARKER_STYLES_TO_PLOTLY.at(plotPoints->GetMarkerStyle());
    marker["size"] = plotPoints->GetMarkerSize();
    node["mode"] = "lines+markers"; // FIXME how to do markers only ?
  }
  else
  {
    node["mode"] = "lines";
  }
}
