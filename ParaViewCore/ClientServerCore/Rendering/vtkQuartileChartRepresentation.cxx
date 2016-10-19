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

protected:
  virtual bool UpdateSinglePlotProperties(vtkXYChartRepresentation* self,
    const std::string& tableName, const std::string& columnName, const std::string& role,
    vtkPlot* plot)
  {
    vtkQuartileChartRepresentation* qcr = vtkQuartileChartRepresentation::SafeDownCast(self);
    if ((role == "minmax" && qcr->GetRangeVisibility() == false) ||
      (role == "q1q3" && qcr->GetQuartileVisibility() == false) ||
      (role == "avg" && qcr->GetAverageVisibility() == false) ||
      (role == "med" && qcr->GetMedianVisibility() == false))
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
      plot->SetLabel(role + " " + plot->GetLabel());
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
void vtkQuartileChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "QuartileVisibility: " << this->QuartileVisibility << endl;
  os << indent << "RangeVisibility: " << this->RangeVisibility << endl;
  os << indent << "AverageVisibility: " << this->AverageVisibility << endl;
  os << indent << "MedianVisibility: " << this->MedianVisibility << endl;
}
