/*=========================================================================

  Program:   ParaView
  Module:    vtkXYChartRepresentationInternals.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXYChartRepresentationInternals
 *
 * Implementation class used by vtkXYChartRepresentation.
*/

#ifndef vtkXYChartRepresentationInternals_h
#define vtkXYChartRepresentationInternals_h

#include "vtkCSVExporter.h"
#include "vtkChartXY.h"
#include "vtkColor.h"
#include "vtkDataArray.h"
#include "vtkPen.h"
#include "vtkPlotBar.h"
#include "vtkPlotFunctionalBag.h"
#include "vtkPlotPoints.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include <array>
#include <map>
#include <string>

class vtkXYChartRepresentation::vtkInternals
{
protected:
  struct PlotInfo
  {
    vtkSmartPointer<vtkPlot> Plot;
    std::string TableName;
    std::string ColumnName;
  };

  typedef std::map<std::string, PlotInfo> PlotsMapItem;
  class PlotsMap : public std::map<std::string, PlotsMapItem>
  {
    bool Contains(const std::string& key, const std::string& role) const
    {
      PlotsMap::const_iterator iter1 = this->find(key);
      if (iter1 != this->end())
      {
        PlotsMapItem::const_iterator iter2 = iter1->second.find(role);
        return (iter2 != iter1->second.end());
      }
      return false;
    }

  public:
    vtkSmartPointer<vtkPlot> GetPlot(vtkChartRepresentation* self, const std::string& tableName,
      const std::string& columnName, const std::string& role = std::string()) const
    {
      const std::string key = self->GetDefaultSeriesLabel(tableName, columnName);
      PlotsMap::const_iterator iter1 = this->find(key);
      if (iter1 != this->end())
      {
        PlotsMapItem::const_iterator iter2 = iter1->second.find(role);
        if (iter2 != iter1->second.end())
        {
          return iter2->second.Plot;
        }
      }
      return vtkSmartPointer<vtkPlot>();
    }

    bool RemovePlot(vtkChartRepresentation* self, const std::string& tableName,
      const std::string& columnName, const std::string& role = std::string())
    {
      const std::string key = self->GetDefaultSeriesLabel(tableName, columnName);
      PlotsMap::iterator iter1 = this->find(key);
      if (iter1 != this->end())
      {
        PlotsMapItem::iterator iter2 = iter1->second.find(role);
        if (iter2 != iter1->second.end())
        {
          iter1->second.erase(iter2);
          return true;
        }
      }
      return false;
    }

    void AddPlot(vtkChartRepresentation* self, const std::string& tableName,
      const std::string& columnName, const std::string& role, vtkPlot* plot)
    {
      const std::string key = self->GetDefaultSeriesLabel(tableName, columnName);
      PlotInfo& info = (*this)[key][role];
      info.TableName = tableName;
      info.ColumnName = columnName;
      info.Plot = plot;
    }

    void SetPlotVisibility(bool val) const
    {
      for (PlotsMap::const_iterator iter1 = this->begin(); iter1 != this->end(); ++iter1)
      {
        for (PlotsMapItem::const_iterator iter2 = iter1->second.begin();
             iter2 != iter1->second.end(); ++iter2)
        {
          iter2->second.Plot->SetVisible(val);
        }
      }
    }

    void RemoveAllPlots(vtkChartXY* chartXY)
    {
      for (PlotsMap::const_iterator iter1 = this->begin(); iter1 != this->end(); ++iter1)
      {
        for (PlotsMapItem::const_iterator iter2 = iter1->second.begin();
             iter2 != iter1->second.end(); ++iter2)
        {
          chartXY->RemovePlotInstance(iter2->second.Plot.GetPointer());
        }
      }
      this->clear();
    }

    void Intersect(const PlotsMap& other, vtkChartXY* chartXY)
    {
      for (PlotsMap::iterator iter1 = this->begin(); iter1 != this->end(); ++iter1)
      {
        for (PlotsMapItem::iterator iter2 = iter1->second.begin(); iter2 != iter1->second.end();)
        {
          if (other.Contains(iter1->first, iter2->first) == false)
          {
            chartXY->RemovePlotInstance(iter2->second.Plot.GetPointer());
            PlotsMapItem::iterator iter2old = iter2;
            ++iter2;
            iter1->second.erase(iter2old);
          }
          else
          {
            ++iter2;
          }
        }
      }
    }
  };

  PlotsMap SeriesPlots;

  //---------------------------------------------------------------------------
  // Makes is easy to obtain a value for a series parameter, is set, else the
  // default. This class supports two mechanisms for addresses series in a
  // collection  (multiblock) of tables: (1) using a name that combines the
  // table name and the column name (using
  // vtkChartRepresentation::GetDefaultSeriesLabel), or (2) using the column
  // name alone. (1) is always checked before (2).
  template <class T>
  T GetSeriesParameter(vtkXYChartRepresentation* self, const std::string& tableName,
    const std::string& columnName, const std::string& vtkNotUsed(role),
    const std::map<std::string, T>& parameter_map, const T default_value = T()) const
  {
    typename std::map<std::string, T>::const_iterator iter;

    // when setting properties for a series, I want to support two mechanisms:
    // simply specifying the array name or suffixing it with the block-name.
    // This logic makes that possible.

    // first try most specific form of identifying the series.
    std::string key = self->GetDefaultSeriesLabel(tableName, columnName);
    iter = parameter_map.find(key);
    if (iter != parameter_map.end())
    {
      return iter->second;
    }

    // now try the cheap form for identifying it.
    key = self->GetDefaultSeriesLabel(std::string(), columnName);
    iter = parameter_map.find(key);
    if (iter != parameter_map.end())
    {
      return iter->second;
    }
    return default_value;
  }

public:
  typedef vtkXYChartRepresentation::MapOfTables MapOfTables;

  // we have to keep these separate since they are set by different properties
  // and hence may not always match up.
  std::map<std::string, bool> SeriesVisibilities;
  std::map<std::string, int> SeriesOrder;
  std::map<std::string, int> LineThicknesses;
  std::map<std::string, int> LineStyles;
  std::map<std::string, vtkColor3d> Colors;
  std::map<std::string, int> AxisCorners;
  std::map<std::string, int> MarkerStyles;
  std::map<std::string, std::string> Labels;
  std::map<std::string, bool> UseColorMapping;
  std::map<std::string, vtkScalarsToColors*> Lut;

  // These are used to determine when to recalculate chart bounds. If user
  // changes the X axis, we force recalculation of the chart bounds
  // automatically.
  bool PreviousUseIndexForXAxis;
  std::string PreviousXAxisSeriesName;

  vtkInternals()
    : PreviousUseIndexForXAxis(false)
  {
  }

  virtual ~vtkInternals() {}

  //---------------------------------------------------------------------------
  // Hide all plots.
  void HideAllPlots() { this->SeriesPlots.SetPlotVisibility(false); }

  //---------------------------------------------------------------------------
  // Destroy all vtkPlot instances.
  void RemoveAllPlots(vtkChartXY* chartXY) { this->SeriesPlots.RemoveAllPlots(chartXY); }

  //---------------------------------------------------------------------------
  /**
   * Subclasses can override this method to assign a role for a specific data
   * array in the input dataset. This is useful when multiple plots are to be
   * created for a single series.
   */
  virtual std::vector<std::string> GetSeriesRoles(
    const std::string& vtkNotUsed(tableName), const std::string& vtkNotUsed(columnName))
  {
    return { std::string() };
  }

  virtual vtkPlot* NewPlot(vtkXYChartRepresentation* self, const std::string& tableName,
    const std::string& columnName, const std::string& role)
  {
    (void)tableName;
    (void)columnName;
    (void)role;

    assert(self);
    vtkChartXY* chartXY = self->GetChart();

    assert(chartXY);
    return chartXY->AddPlot(self->GetChartType());
  }

  //---------------------------------------------------------------------------
  virtual int GetInputArrayIndex(const std::string& vtkNotUsed(tableName),
    const std::string& vtkNotUsed(columnName), const std::string& vtkNotUsed(role))
  {
    return 1;
  }

  //---------------------------------------------------------------------------
  // Update i.e. add/remove plots based on the data in the tables.
  virtual void UpdatePlots(vtkXYChartRepresentation* self, const MapOfTables& tables)
  {
    PlotsMap newPlots;
    assert(self != NULL);
    vtkChartXY* chartXY = self->GetChart();
    this->RemoveAllPlots(chartXY);
    std::multimap<int, std::pair<vtkTable*, std::array<std::string, 3> > > orderMap;
    for (MapOfTables::const_iterator tablesIter = tables.begin(); tablesIter != tables.end();
         ++tablesIter)
    {
      const std::string& tableName = tablesIter->first;
      vtkTable* table = tablesIter->second.GetPointer();
      vtkIdType numCols = table->GetNumberOfColumns();
      for (vtkIdType cc = 0; cc < numCols; ++cc)
      {
        std::string columnName = table->GetColumnName(cc);
        auto roles = this->GetSeriesRoles(tableName, columnName);

        for (const auto& role : roles)
        {
          // recover the order of the current table column
          const int order =
            this->GetSeriesParameter(self, tableName, columnName, role, this->SeriesOrder, -1);

          // store all info in a (sorted) map
          std::array<std::string, 3> mapValue;
          mapValue[0] = tableName;
          mapValue[1] = columnName;
          mapValue[2] = role;
          orderMap.insert(std::make_pair(order, std::make_pair(table, mapValue)));
        }
      }
    }

    // for each ordered column
    for (auto it = orderMap.begin(); it != orderMap.end(); ++it)
    {
      vtkTable* table = it->second.first;
      const std::string& tableName = it->second.second[0];
      const std::string& columnName = it->second.second[1];
      const std::string& role = it->second.second[2];

      vtkSmartPointer<vtkPlot> plot = this->SeriesPlots.GetPlot(self, tableName, columnName, role);
      if (!plot)
      {
        // Create the plot in the right order
        plot = this->NewPlot(self, tableName, columnName, role);
        if (!plot)
        {
          continue;
        }
      }
      plot->SetInputData(table);
      plot->SetUseIndexForXSeries(self->GetUseIndexForXAxis());
      plot->SetInputArray(0, self->GetXAxisSeriesName());
      plot->SetInputArray(this->GetInputArrayIndex(tableName, columnName, role), columnName);
      this->SeriesPlots.AddPlot(self, tableName, columnName, role, plot);
      newPlots.AddPlot(self, tableName, columnName, role, plot);
    }

    // Remove any plots in this->SeriesPlots that are not in newPlots.
    this->SeriesPlots.Intersect(newPlots, chartXY);
  }

  //---------------------------------------------------------------------------
  // Update properties for plots in the chart.
  virtual void UpdatePlotProperties(vtkXYChartRepresentation* self)
  {
    vtkChartXY* chartXY = self->GetChart();
    vtkPlot* lastFunctionalBagPlot = 0;
    for (PlotsMap::iterator iter1 = this->SeriesPlots.begin(); iter1 != this->SeriesPlots.end();
         ++iter1)
    {
      for (PlotsMapItem::const_iterator iter2 = iter1->second.begin(); iter2 != iter1->second.end();
           ++iter2)
      {
        const PlotInfo& plotInfo = iter2->second;
        const std::string& tableName = plotInfo.TableName;
        const std::string& columnName = plotInfo.ColumnName;
        vtkPlot* plot = plotInfo.Plot;
        const std::string& role = iter2->first;

        if (this->UpdateSinglePlotProperties(self, tableName, columnName, role, plot))
        {
          // Functional bag plots shall be stacked under the other plots.
          vtkPlotFunctionalBag* plotBag = vtkPlotFunctionalBag::SafeDownCast(plot);
          if (plotBag)
          {
            // We can't select the median line as it may not exist in other dataset.
            if (columnName == "QMedianLine")
            {
              plotBag->SelectableOff();
            }
            if (plotBag->IsBag())
            {
              if (!lastFunctionalBagPlot)
              {
                chartXY->LowerPlot(plotBag);
              }
              else
              {
                chartXY->StackPlotAbove(plotBag, lastFunctionalBagPlot);
              }
              lastFunctionalBagPlot = plotBag;
            }
          }
        }
      }
    }
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
        const std::string& columnName = plotInfo.ColumnName;
        vtkTable* table = plot->GetInput();
        vtkDataArray* xarray = self->GetUseIndexForXAxis()
          ? NULL
          : vtkDataArray::SafeDownCast(table->GetColumnByName(self->GetXAxisSeriesName()));
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
  //---------------------------------------------------------------------------
  // Returns false for in-visible plots.
  virtual bool UpdateSinglePlotProperties(vtkXYChartRepresentation* self,
    const std::string& tableName, const std::string& columnName, const std::string& role,
    vtkPlot* plot)
  {
    vtkChartXY* chartXY = self->GetChart();
    const bool visible =
      this->GetSeriesParameter(self, tableName, columnName, role, this->SeriesVisibilities, false);
    plot->SetVisible(visible);
    if (!visible)
    {
      return false;
    }

    std::string default_label = self->GetDefaultSeriesLabel(tableName, columnName);
    std::string label =
      this->GetSeriesParameter(self, tableName, columnName, role, this->Labels, default_label);
    if (self->GetSeriesLabelPrefix())
    {
      label = std::string(self->GetSeriesLabelPrefix()) + label;
    }
    plot->SetLabel(label);

    vtkColor3d color = this->GetSeriesParameter(
      self, tableName, columnName, role, this->Colors, vtkColor3d(0, 0, 0));
    plot->SetColor(color.GetRed(), color.GetGreen(), color.GetBlue());
    plot->GetSelectionPen()->SetColorF(self->SelectionColor);

    plot->SetWidth(
      this->GetSeriesParameter(self, tableName, columnName, role, this->LineThicknesses, 2));
    plot->GetPen()->SetLineType(this->GetSeriesParameter(
      self, tableName, columnName, role, this->LineStyles, static_cast<int>(vtkPen::SOLID_LINE)));

    if (vtkPlotPoints* plotPoints = vtkPlotPoints::SafeDownCast(plot))
    {
      plotPoints->SetMarkerStyle(this->GetSeriesParameter(self, tableName, columnName, role,
        this->MarkerStyles, static_cast<int>(vtkPlotPoints::NONE)));
      // the vtkValidPointMask array is used by some filters (like plot
      // over line) to indicate invalid points. this instructs the line
      // plot to not render those points
      plotPoints->SetValidPointMaskName("vtkValidPointMask");
    }

    chartXY->SetPlotCorner(
      plot, this->GetSeriesParameter(self, tableName, columnName, role, this->AxisCorners, 0));

    // for now only vtkPlotBar has color mapping
    vtkPlotBar* plotBar = vtkPlotBar::SafeDownCast(plot);
    if (plotBar && columnName == "bin_values")
    {
      bool colorMapping =
        this->GetSeriesParameter(self, tableName, columnName, role, this->UseColorMapping, false);
      plotBar->SetScalarVisibility(colorMapping);
      plotBar->SelectColorArray("bin_extents");
      vtkScalarsToColors* lut = this->GetSeriesParameter(
        self, tableName, columnName, role, this->Lut, static_cast<vtkScalarsToColors*>(NULL));
      if (lut)
      {
        plotBar->SetLookupTable(lut);
      }
    }
    return true;
  }
};

#endif
// VTK-HeaderTest-Exclude: vtkXYChartRepresentationInternals.h
