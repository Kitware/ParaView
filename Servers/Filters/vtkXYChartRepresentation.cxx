/*=========================================================================

  Program:   ParaView
  Module:    vtkXYChartRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXYChartRepresentation.h"

#include "vtkChartXY.h"
#include "vtkCommand.h"
#include "vtkContextNamedOptions.h"
#include "vtkContextView.h"
#include "vtkObjectFactory.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkPVContextView.h"

vtkStandardNewMacro(vtkXYChartRepresentation);
//----------------------------------------------------------------------------
vtkXYChartRepresentation::vtkXYChartRepresentation()
{
}

//----------------------------------------------------------------------------
vtkXYChartRepresentation::~vtkXYChartRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkChartXY* vtkXYChartRepresentation::GetChart()
{
  if (this->ContextView)
    {
    return vtkChartXY::SafeDownCast(this->ContextView->GetChart());
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetXAxisSeriesName(const char* name)
{
  this->Options->SetXSeriesName(name);
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetUseIndexForXAxis(bool useIndex)
{
  this->Options->SetUseIndexForXAxis(useIndex);
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetChartType(const char *type)
{
  // Match the string to the vtkChart enum, if no match then stick with default.
  if (strcmp(type, "Line") == 0)
    {
    this->Options->SetChartType(vtkChart::LINE);
    }
  else if (strcmp(type, "Bar") == 0)
    {
    this->Options->SetChartType(vtkChart::BAR);
    }
}

//----------------------------------------------------------------------------
int vtkXYChartRepresentation::GetChartType()
{
  return this->Options->GetChartType();
}
