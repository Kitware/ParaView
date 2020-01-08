/*=========================================================================

  Program:   ParaView
  Module:    vtkQuartileChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkQuartileChartRepresentation.h"
#include "vtkXYChartRepresentationInternals.h"

#include "vtkBrush.h"
#include "vtkChartXY.h"
#include "vtkObjectFactory.h"
#include "vtkPlotArea.h"
#include "vtkPlotLine.h"
#include "vtkStdString.h"

#include <map>
#include <vtksys/RegularExpression.hxx>

namespace
{
static vtksys::RegularExpression StatsArrayRe("^([^( ]+)\\((.+)\\)$");
}

class vtkQuartileChartRepresentation::vtkQCRInternals
  : public vtkXYChartRepresentation::vtkInternals
{
  typedef vtkXYChartRepresentation::vtkInternals Superclass;

public:
  //---------------------------------------------------------------------------
  // Description:
  // Subclasses can override this method to assign a role for a specific data
  // array in the input dataset. This is useful when multiple plots are to be
  // created for a single series.
  std::vector<std::string> GetSeriesRoles(
    const std::string& vtkNotUsed(tableName), const std::string& columnName) override
  {
    if (StatsArrayRe.find(columnName))
    {
      const std::string fn = StatsArrayRe.match(1);
      if (fn == "min" || fn == "max")
      {
        return { std::string("minmax"), fn };
      }
      else if (fn == "q1" || fn == "q3")
      {
        return { "q1q3" };
      }
      return { fn };
    }
    return { std::string() };
  }

  vtkPlot* NewPlot(vtkXYChartRepresentation* self, const std::string& tableName,
    const std::string& columnName, const std::string& role) override
  {
    if (role == "minmax" || role == "q1q3")
    {
      vtkPlot* plot = this->Superclass::NewPlot(self, tableName, columnName, role);
      if (vtkPlotArea* aplot = vtkPlotArea::SafeDownCast(plot))
      {
        aplot->SetLegendVisibility(false);
      }
      return plot;
    }
    else if ((role == "avg") || (role == "med") || (role == "min") || (role == "max") ||
      role.empty())
    {
      assert(self);
      vtkChartXY* chartXY = self->GetChart();

      assert(chartXY);
      return chartXY->AddPlot(vtkChart::LINE);
    }
    return NULL;
  }

  //---------------------------------------------------------------------------
  int GetInputArrayIndex(
    const std::string& tableName, const std::string& columnName, const std::string& role) override
  {
    if (role == "minmax")
    {
      if (StatsArrayRe.find(columnName))
      {
        const std::string fn = StatsArrayRe.match(1);
        return fn == "min" ? 1 : 2;
      }
    }
    else if (role == "q1q3")
    {
      if (StatsArrayRe.find(columnName))
      {
        const std::string fn = StatsArrayRe.match(1);
        return fn == "q1" ? 1 : 2;
      }
    }
    return this->Superclass::GetInputArrayIndex(tableName, columnName, role);
  }

  //---------------------------------------------------------------------------
  // Export visible plots to a CSV file.
  bool Export(vtkXYChartRepresentation* self, vtkCSVExporter* exporter) override
  {
    // used to avoid adding duplicate columns.
    std::set<std::string> added_column_names;

    for (PlotsMap::iterator iter1 = this->SeriesPlots.begin(); iter1 != this->SeriesPlots.end();
         ++iter1)
    {
      for (PlotsMapItem::const_iterator iter2 = iter1->second.begin(); iter2 != iter1->second.end();
           ++iter2)
      {
        const std::string role = iter2->first;
        const PlotInfo& plotInfo = iter2->second;
        vtkPlot* plot = plotInfo.Plot;
        if (!plot->GetVisible())
        {
          continue;
        }
        std::string columnName = plotInfo.ColumnName;
        vtkTable* table = plot->GetInput();
        vtkDataArray* xarray = self->GetUseIndexForXAxis()
          ? NULL
          : vtkDataArray::SafeDownCast(table->GetColumnByName(self->GetXAxisSeriesName()));

        if (role == "minmax" || role == "q1q3")
        {
          // need to add two separate columns for these.
          const std::string& tableName = plotInfo.TableName;
          const std::string default_label = self->GetDefaultSeriesLabel(tableName, columnName);
          const std::string label = this->GetSeriesParameter(
            self, tableName, columnName, role, this->Labels, default_label);

          std::vector<std::pair<std::string, std::string> > yNames;
          if (role == "minmax")
          {
            yNames.push_back(std::make_pair(columnName.replace(0, 3, "min"), "min " + label));
            yNames.push_back(std::make_pair(columnName, "max " + label));
          }
          else
          {
            yNames.push_back(std::make_pair(columnName.replace(0, 2, "q1"), "q1 " + label));
            yNames.push_back(std::make_pair(columnName, "q3 " + label));
          }

          for (const auto& apair : yNames)
          {
            vtkAbstractArray* yarray = table->GetColumnByName(apair.first.c_str());
            if (yarray != NULL && added_column_names.find(apair.second) == added_column_names.end())
            {
              added_column_names.insert(apair.second);
              exporter->AddColumn(yarray, apair.second.c_str(), xarray);
            }
          }
          continue;
        }

        vtkAbstractArray* yarray = table->GetColumnByName(columnName.c_str());
        if (yarray != NULL && added_column_names.find(plot->GetLabel()) == added_column_names.end())
        {
          added_column_names.insert(plot->GetLabel());
          exporter->AddColumn(yarray, plot->GetLabel().c_str(), xarray);
        }
      }
    }
    return true;
  }

protected:
  bool UpdateSinglePlotProperties(vtkXYChartRepresentation* self, const std::string& tableName,
    const std::string& columnName, const std::string& role, vtkPlot* plot) override
  {
    vtkQuartileChartRepresentation* qcr = vtkQuartileChartRepresentation::SafeDownCast(self);
    if ((role == "minmax" && (qcr->GetRangeVisibility() == false || qcr->HasOnlyOnePoint)) ||
      (role == "q1q3" && (qcr->GetQuartileVisibility() == false || qcr->HasOnlyOnePoint)) ||
      (role == "avg" && qcr->GetAverageVisibility() == false) ||
      (role == "med" && (qcr->GetMedianVisibility() == false || qcr->HasOnlyOnePoint)) ||
      (role == "min" && (qcr->GetMinVisibility() == false || qcr->HasOnlyOnePoint)) ||
      (role == "max" && (qcr->GetMaxVisibility() == false || qcr->HasOnlyOnePoint)))
    {
      plot->SetVisible(false);
      return false;
    }
    if (!this->Superclass::UpdateSinglePlotProperties(self, tableName, columnName, role, plot))
    {
      return false;
    }
    if (role == "minmax")
    {
      plot->GetBrush()->SetOpacityF(0.25);
      plot->GetPen()->SetOpacityF(0.25);
      plot->SetLabel("min/max " + plot->GetLabel());
    }
    else if (role == "q1q3")
    {
      plot->GetBrush()->SetOpacityF(0.5);
      plot->GetPen()->SetOpacityF(0.5);
      plot->SetLabel("q1/q3 " + plot->GetLabel());
    }
    else if (role.empty() == false)
    {
      if (role == "med")
      {
        plot->GetBrush()->SetOpacityF(0.3);
        plot->GetPen()->SetOpacityF(0.3);
      }
      else if (role == "min")
      {
        plot->GetBrush()->SetOpacityF(0.3);
        plot->GetPen()->SetOpacityF(0.3);
        plot->GetPen()->SetLineType(vtkPen::DASH_LINE);
      }
      else if (role == "max")
      {
        plot->GetPen()->SetLineType(vtkPen::DASH_LINE);
      }
      if (!qcr->HasOnlyOnePoint)
      {
        plot->SetLabel(role + " " + plot->GetLabel());
      }
    }
    return true;
  }
};

vtkStandardNewMacro(vtkQuartileChartRepresentation);
//----------------------------------------------------------------------------
vtkQuartileChartRepresentation::vtkQuartileChartRepresentation()
  : AverageVisibility(true)
  , HasOnlyOnePoint(false)
  , MaxVisibility(false)
  , MedianVisibility(true)
  , MinVisibility(false)
  , QuartileVisibility(true)
  , RangeVisibility(true)
{
  delete this->Internals;
  this->Internals = new vtkQCRInternals();
}

//----------------------------------------------------------------------------
vtkQuartileChartRepresentation::~vtkQuartileChartRepresentation()
{
}

//----------------------------------------------------------------------------
vtkStdString vtkQuartileChartRepresentation::GetDefaultSeriesLabel(
  const vtkStdString& tableName, const vtkStdString& columnName)
{
  // statsArrayRe1: e.g. "min(EQPS)".
  if (StatsArrayRe.find(columnName))
  {
    return this->Superclass::GetDefaultSeriesLabel(tableName, StatsArrayRe.match(2));
  }
  return this->Superclass::GetDefaultSeriesLabel(tableName, columnName);
}

//----------------------------------------------------------------------------
void vtkQuartileChartRepresentation::PrepareForRendering()
{
  vtkChartRepresentation::MapOfTables tables;
  if (!this->GetLocalOutput(tables))
  {
    this->Internals->HideAllPlots();
    return;
  }

  this->HasOnlyOnePoint = false;
  for (auto itr = tables.begin(); itr != tables.end(); ++itr)
  {
    vtkTable* table = itr->second;
    vtkAbstractArray* column = table->GetColumnByName("N");
    if (column)
    {
      vtkDataArray* narray = vtkDataArray::SafeDownCast(column);
      double range[2];
      narray->GetRange(range);
      if (range[0] == range[1] && range[0] == 1)
      {
        this->HasOnlyOnePoint = true;
      }
    }
  }

  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkQuartileChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AverageVisibility: " << this->AverageVisibility << endl;
  os << indent << "MaxVisibility: " << this->MinVisibility << endl;
  os << indent << "MedianVisibility: " << this->MedianVisibility << endl;
  os << indent << "MinVisibility: " << this->MaxVisibility << endl;
  os << indent << "QuartileVisibility: " << this->QuartileVisibility << endl;
  os << indent << "RangeVisibility: " << this->RangeVisibility << endl;
}
