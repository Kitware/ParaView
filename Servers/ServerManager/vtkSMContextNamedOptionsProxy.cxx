/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextNamedOptionsProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMContextNamedOptionsProxy.h"

#include "vtkObjectFactory.h"
#include "vtkStringList.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkChart.h"
#include "vtkPlot.h"
#include "vtkPen.h"
#include "vtkTable.h"
#include "vtkWeakPointer.h"

#include "vtkstd/map"

#include <QString>

class vtkSMContextNamedOptionsProxy::vtkInternals
{
public:
  vtkstd::map<vtkstd::string, vtkWeakPointer<vtkPlot> > PlotMap;
  vtkstd::string XSeriesName;

  vtkWeakPointer<vtkChart> Chart;
  vtkWeakPointer<vtkTable> Table;

  vtkInternals()
    {
    }

  ~vtkInternals()
    {
    }

};

vtkStandardNewMacro(vtkSMContextNamedOptionsProxy);
vtkCxxRevisionMacro(vtkSMContextNamedOptionsProxy, "1.6");
//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::vtkSMContextNamedOptionsProxy()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMContextNamedOptionsProxy::~vtkSMContextNamedOptionsProxy()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetChart(vtkChart* chart)
{
  if (this->Internals->Chart == chart)
    {
    return;
    }
  this->Internals->Chart = chart;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetTable(vtkTable* table)
{
  if (this->Internals->Table == table)
    {
    return;
    }
  this->Internals->Table = table;
  // If the table changed then update the plot map
  this->InitializePlotMap();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetXSeriesName(const char* name)
{
  //cout << "Setting the X series name: " << vtkstd::string(name) << endl;
  if (!name)
    {
    this->Internals->XSeriesName = "";
    }
  else
    {
    this->Internals->XSeriesName = name;
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::InitializePlotMap()
{
  this->Internals->PlotMap.clear();
  if (!this->Internals->Table)
    {
    return;
    }
  else
    {
    for (int i = 0; i < this->Internals->Table->GetNumberOfColumns(); ++i)
      {
      this->Internals->PlotMap[this->Internals->Table->GetColumnName(i)] = 0;
      }
    // Choose a default plot series if one has not already been chosen
    if (this->Internals->Table->GetNumberOfColumns() > 1 &&
        this->Internals->Chart)
      {
      vtkPlot *plot = this->Internals->Chart->AddPlot(vtkChart::LINE);
      plot->SetUseIndexForXSeries(true);
      plot->SetInput(this->Internals->Table, vtkIdType(0), vtkIdType(0));
      this->Internals->PlotMap[this->Internals->Table->GetColumnName(0)] =
          plot;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::UpdatePropertyInformationInternal(
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp =
      vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && this->Internals->Table && svp->GetInformationOnly())
    {
    vtkStringList* new_values = vtkStringList::New();
    int numOptions = this->Internals->Table->GetNumberOfColumns();
    const char* propname = this->GetPropertyName(prop);
    bool skip = false;
    for (int i = 0; i < numOptions; ++i)
      {
      QString name = this->Internals->Table->GetColumnName(i);
      vtkPlot *plot = this->Internals->PlotMap[name.toStdString()];
      if (strcmp(propname, "VisibilityInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());
        if (this->Internals->PlotMap[name.toStdString()] &&
            this->Internals->PlotMap[name.toStdString()]->GetVisible())
          {
          new_values->AddString("1");
          }
        else
          {
          new_values->AddString("0");
          }
        }
      else if (strcmp(propname, "LabelInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());
        if (plot)
          {
          new_values->AddString(plot->GetLabel() ? plot->GetLabel() :
                                name.toAscii().data());
          }
        else
          {
          new_values->AddString(name.toAscii().data());
          }
        }
      else if (strcmp(propname, "LineThicknessInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());
        if (plot)
          {
          new_values->AddString(
              QString::number(plot->GetWidth()).toAscii().data());
          }
        else
          {
          new_values->AddString("2"); // The default value
          }
        }
      else if (strcmp(propname, "ColorInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());
        if (plot)
          {
          double rgb[3];
          plot->GetColor(rgb);
          new_values->AddString(QString::number(rgb[0]).toAscii().data());
          new_values->AddString(QString::number(rgb[1]).toAscii().data());
          new_values->AddString(QString::number(rgb[2]).toAscii().data());
          }
        else
          {
          new_values->AddString("0.0");
          new_values->AddString("0.0");
          new_values->AddString("0.0");
          }
        }
      else if (strcmp(propname, "LineStyleInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());
        if (plot)
          {
          new_values->AddString(
              QString::number(plot->GetPen()->GetLineType()).toAscii().data());
          }
        else
          {
          new_values->AddString("1");
          }
        }
      else if (strcmp(propname, "MarkerStyleInfo") == 0)
        {
        new_values->AddString(name.toAscii().data());
        if (plot && false)
          {
          new_values->AddString(
              QString::number(plot->GetPen()->GetWidth()).toAscii().data());
          }
        else
          {
          new_values->AddString("0");
          }
        }
      else
        {
        skip = true;
        break;
        }
      }
    if (!skip)
      {
      svp->SetElements(new_values);
      }
    new_values->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetVisibility(const char* name, int visible)
{
  if (this->Internals->PlotMap[name])
    {
    vtkPlot *plot = this->Internals->PlotMap[name];
    plot->SetVisible(static_cast<bool>(visible));
    // If the X series has changed update this in the plot plot
    if (plot->GetUseIndexForXSeries() !=
        (this->Internals->XSeriesName.length() == 0))
      {
      plot->SetUseIndexForXSeries(this->Internals->XSeriesName.length() == 0);
      if (this->Internals->XSeriesName.length() > 0)
        {
        plot->SetInput(this->Internals->Table,
                       this->Internals->XSeriesName.c_str(),
                       name);
        }
      }
    }
  else
    {
    // Only create a new plot if the request is to make the plot visible
    if (visible && this->Internals->Chart)
      {
      vtkPlot *plot = this->Internals->Chart->AddPlot(vtkChart::LINE);
      if (this->Internals->XSeriesName.length() == 0)
        {
        plot->SetUseIndexForXSeries(true);
        plot->SetInput(this->Internals->Table, name, name);
        }
      else
        {
        plot->SetUseIndexForXSeries(false);
        plot->SetInput(this->Internals->Table,
                       this->Internals->XSeriesName.c_str(),
                       name);
        }
      // Update the map with the pointer
      this->Internals->PlotMap[name] = plot;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetLineThickness(const char* name,
                                                     int value)
{
  if (this->Internals->PlotMap[name])
    {
    this->Internals->PlotMap[name]->SetWidth(value);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetLineStyle(const char* name, int style)
{
  if (this->Internals->PlotMap[name])
    {
    this->Internals->PlotMap[name]->GetPen()->SetLineType(style);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetColor(const char* name,
                                             double r, double g, double b)
{
  if (this->Internals->PlotMap[name])
    {
    this->Internals->PlotMap[name]->SetColor(r, g, b);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetAxisCorner(const char*, int)
{

}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetMarkerStyle(const char*, int)
{
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::SetLabel(const char* name,
                                             const char* label)
{
  if (this->Internals->PlotMap[name])
    {
    this->Internals->PlotMap[name]->SetLabel(label);
    }
}

//----------------------------------------------------------------------------
void vtkSMContextNamedOptionsProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
