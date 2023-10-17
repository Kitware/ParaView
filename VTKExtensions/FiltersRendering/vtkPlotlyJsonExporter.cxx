// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotlyJsonExporter.h"

#include "vtkAxis.h"
#include "vtkChart.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkGenericDataArray.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPlotPoints.h" // for vtkPlotPoints
#include "vtkTextProperty.h"
#include "vtksys/FStream.hxx"

#include "vtk_nlohmannjson.h"
// clang-format off
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

#include <sstream>

using vtkNJson = nlohmann::json;

namespace
{
void AddArray(vtkNJson& parent, const std::string& key, vtkAbstractArray* array)
{
  if (auto* dpArray = vtkDoubleArray::SafeDownCast(array))
  {
    std::vector<double> vec(dpArray->Begin(), dpArray->End());
    parent[key] = std::move(vec);
  }
  else if (auto* dArray = vtkDataArray::SafeDownCast(array))
  {
    std::vector<double> vec(dArray->GetNumberOfValues());
    for (vtkIdType i = 0; i < dArray->GetNumberOfValues(); i++)
    {
      vec[i] = dArray->GetTuple1(i);
    }
    parent[key] = std::move(vec);
  }
  else
  {
    vtkLogF(WARNING, "Array type is %s is not yet supported by vtkPlotlyJsonExporter",
      array->GetArrayTypeAsString());
  }
}

std::string GetColorAsString(double rgb[3])
{
  return "rgb(" + std::to_string(rgb[0]) + "," + std::to_string(rgb[1]) + "," +
    std::to_string(rgb[2]) + ")";
}

void SetFontProperties(vtkNJson& font, vtkTextProperty* textProperty)
{
  const std::map<int, std::string> VTK_FONT_TO_PLOTLY = {
    { VTK_ARIAL, "Arial" },
    { VTK_TIMES, "Times New Roman" },
    { VTK_COURIER, "Courier New" },
    { VTK_FONT_FILE, "Arial" },
    { VTK_UNKNOWN_FONT, "Arial" },
  };

  double rgb[3];
  textProperty->GetColor(rgb);
  font["color"] = GetColorAsString(rgb);
  font["size"] = textProperty->GetFontSize();
  font["family"] = VTK_FONT_TO_PLOTLY.at(textProperty->GetFontFamily());
}
}

class vtkPlotlyJsonExporter::vtkInternals
{
public:
  vtkNJson Data = vtkNJson::object();
  vtksys::ofstream File;
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
  this->Close();
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
  // setup basic structure
  internals.Data["data"] = vtkNJson::array();
  internals.Data["layout"] = vtkNJson::object();
  if (mode == ExporterModes::STREAM_COLUMNS)
  {
    if (!this->WriteToOutputString)
    {
      internals.File.open(this->FileName);
    }
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
  if (this->WriteToOutputString)
  {
    return;
  }
  if (internals.File.is_open())
  {
    try
    {
      internals.File << internals.Data;
      internals.File.close();
    }
    catch (...)
    {
      vtkLogF(ERROR, "Failed writing json to file");
    }
  }
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::Abort()
{
  if (this->WriteToOutputString)
  {
    return;
  }
  auto& internals = *(this->Internals);
  this->Close();
  vtksys::SystemTools::RemoveFile(this->FileName);
  internals.Data = vtkNJson::object();
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
  dataField["name"] = yarrayname;
  AddArray(dataField, "y", yarray);
  if (xarray)
  {
    AddArray(dataField, "x", xarray);
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
    auto iter = object.find("name");
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
  line["color"] = GetColorAsString(rgb);
  line["dash"] = VTK_LINE_STYLES_TO_PLOTLY.at(plot->GetPen()->GetLineType());

  // default style
  node["mode"] = "lines";

  // marker style if any
  if (vtkPlotPoints* plotPoints = vtkPlotPoints::SafeDownCast(plot))
  {
    auto& marker = node["marker"];
    if (plotPoints->GetMarkerStyle() != VTK_MARKER_NONE)
    {
      marker["symbol"] = VTK_MARKER_STYLES_TO_PLOTLY.at(plotPoints->GetMarkerStyle());
      marker["size"] = plotPoints->GetMarkerSize();
      node["mode"] = "lines+markers"; // FIXME how to do markers only ?
    }
  }
}

//----------------------------------------------------------------------------
void vtkPlotlyJsonExporter::SetGlobalStyle(vtkChart* chart)
{
  auto& internals = *(this->Internals);
  auto& layout = internals.Data["layout"];

  // see https://plotly.com/javascript/reference/layout/

  // Graph Title
  auto p = vtkNJson::json_pointer("/title/text");
  layout[p] = chart->GetTitle();

  vtkTextProperty* titleProperties = chart->GetTitleProperties();

  p = vtkNJson::json_pointer("/title/font");
  auto& tfont = layout[p];
  SetFontProperties(tfont, titleProperties);

  // Xaxis
  p = vtkNJson::json_pointer("/xaxis/title/text");
  layout[p] = chart->GetAxis(0)->GetTitle();
  titleProperties = chart->GetAxis(0)->GetTitleProperties();
  p = vtkNJson::json_pointer("/xaxis/title/font");
  auto& xfont = layout[p];
  SetFontProperties(xfont, titleProperties);

  // Yaxis
  p = vtkNJson::json_pointer("/yaxis/title/text");
  layout[p] = chart->GetAxis(1)->GetTitle();
  titleProperties = chart->GetAxis(1)->GetTitleProperties();
  p = vtkNJson::json_pointer("/yaxis/title/font");
  auto& yfont = layout[p];
  SetFontProperties(yfont, titleProperties);
}
//----------------------------------------------------------------------------
std::string vtkPlotlyJsonExporter::GetOutputString() const
{
  auto& internals = *(this->Internals);
  return internals.Data.dump();
}
