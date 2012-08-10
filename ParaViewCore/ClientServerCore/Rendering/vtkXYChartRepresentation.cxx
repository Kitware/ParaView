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
#include "vtkXYChartNamedOptions.h"
#include "vtkContextView.h"
#include "vtkObjectFactory.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkPVContextView.h"

vtkStandardNewMacro(vtkXYChartRepresentation);
//----------------------------------------------------------------------------
vtkXYChartRepresentation::vtkXYChartRepresentation() : XYOptions(NULL)
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
void vtkXYChartRepresentation::SetOptions(vtkChartNamedOptions *options)
{
  vtkChartRepresentation::SetOptions(options);
  this->XYOptions = vtkXYChartNamedOptions::SafeDownCast(options);
  if (!this->XYOptions)
    {
    vtkErrorMacro(<<"Error, options is of wrong type: "
                  << options->GetClassName()
                  << " when it should be vtkXYChartNamedOptions.");
    }
}

//----------------------------------------------------------------------------
vtkChartXY* vtkXYChartRepresentation::GetChart()
{
  if (this->ContextView)
    {
    return vtkChartXY::SafeDownCast(this->ContextView->GetContextItem());
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
const char* vtkXYChartRepresentation::GetXAxisSeriesName()
{
  return this->XYOptions ? this->XYOptions->GetXSeriesName() : NULL;
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetXAxisSeriesName(const char* name)
{
  this->XYOptions->SetXSeriesName(name);
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetUseIndexForXAxis(bool useIndex)
{
  this->XYOptions->SetUseIndexForXAxis(useIndex);
}

//----------------------------------------------------------------------------
void vtkXYChartRepresentation::SetChartType(const char *type)
{
  // Match the string to the vtkChart enum, if no match then stick with default.
  if (strcmp(type, "Line") == 0)
    {
    this->XYOptions->SetChartType(vtkChart::LINE);
    }
  else if (strcmp(type, "Bar") == 0)
    {
    this->XYOptions->SetChartType(vtkChart::BAR);
    }
}

//----------------------------------------------------------------------------
int vtkXYChartRepresentation::GetChartType()
{
  return this->XYOptions->GetChartType();
}
