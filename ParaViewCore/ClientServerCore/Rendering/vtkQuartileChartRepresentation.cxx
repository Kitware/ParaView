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
  virtual std::string GetSeriesRole(
    const std::string& vtkNotUsed(tableName), const std::string& columnName)
  {
    if (StatsArrayRe.find(columnName))
    {
      const std::string fn = StatsArrayRe.match(1);
      if (fn == "min" || fn == "max")
      {
        return "minmax";
      }
      else if (fn == "q1" || fn == "q3")
      {
        return "q1q3";
      }
      return fn;
    }
    return std::string();
  }

  virtual vtkPlot* NewPlot(vtkXYChartRepresentation* self, const std::string& tableName,
    const std::string& columnName, const std::string& role)
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
    else if ((role == "avg") || (role == "med") || role.empty())
    {
      assert(self);
      vtkChartXY* chartXY = self->GetChart();

      assert(chartXY);
      return chartXY->AddPlot(vtkChart::LINE);
    }
    return NULL;
  }

  //---------------------------------------------------------------------------
  virtual int GetInputArrayIndex(
    const std::string& tableName, const std::string& columnName, const std::string& role)
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
  virtual bool Export(vtkXYChartRepresentation* self, vtkCSVExporter* exporter)
  {
    for (PlotsMap::iterator iter1 = this->SeriesPlots.begin(); iter1 != this->SeriesPlots.end();
         ++iter1)
    {
      for (PlotsMapItem::const_iterator iter2 = iter1->second.begin(); iter2 != iter1->second.end();
           ++iter2)
      {
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
        if (StatsArrayRe.find(columnName))
        {
          std::string fn = StatsArrayRe.match(1);
          if (fn == "max" || fn == "q3")
          {
            const std::string& tableName = plotInfo.TableName;
            const std::string role = this->GetSeriesRole(tableName, columnName);
            std::string default_label = self->GetDefaultSeriesLabel(tableName, columnName);
            std::string label = this->GetSeriesParameter(
              self, tableName, columnName, role, this->Labels, default_label);

            std::vector<std::pair<std::string, std::string> > yNames;
            yNames.push_back(std::make_pair(columnName, fn + " " + label));
            if (fn == "max")
            {
              yNames.push_back(std::make_pair(columnName.replace(0, 3, "min"), "min " + label));
            }
            else
            {
              yNames.push_back(std::make_pair(columnName.replace(0, 2, "q1"), "q1 " + label));
            }

            for (auto iter3 = yNames.rbegin(); iter3 != yNames.rend(); ++iter3)
            {
              vtkAbstractArray* yarray = table->GetColumnByName(iter3->first.c_str());
              if (yarray != NULL)
              {
                exporter->AddColumn(yarray, iter3->second.c_str(), xarray);
              }
            }
            continue;
          }
        }

        vtkAbstractArray* yarray = table->GetColumnByName(columnName.c_str());
        if (yarray != NULL)
        {
          exporter->AddColumn(yarray, plot->GetLabel().c_str(), xarray);
        }
      }
    }
    return true;
  }

protected:
  virtual bool UpdateSinglePlotProperties(vtkXYChartRepresentation* self,
    const std::string& tableName, const std::string& columnName, const std::string& role,
    vtkPlot* plot)
  {
    vtkQuartileChartRepresentation* qcr = vtkQuartileChartRepresentation::SafeDownCast(self);
    if ((role == "minmax" && (qcr->GetRangeVisibility() == false || qcr->HasOnlyOnePoint)) ||
      (role == "q1q3" && (qcr->GetQuartileVisibility() == false || qcr->HasOnlyOnePoint)) ||
      (role == "avg" && qcr->GetAverageVisibility() == false) ||
      (role == "med" && (qcr->GetMedianVisibility() == false || qcr->HasOnlyOnePoint)))
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
  : QuartileVisibility(true)
  , RangeVisibility(true)
  , AverageVisibility(true)
  , MedianVisibility(true)
  , HasOnlyOnePoint(false)
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
  os << indent << "QuartileVisibility: " << this->QuartileVisibility << endl;
  os << indent << "RangeVisibility: " << this->RangeVisibility << endl;
  os << indent << "AverageVisibility: " << this->AverageVisibility << endl;
  os << indent << "MedianVisibility: " << this->MedianVisibility << endl;
}
