/*=========================================================================

  Program:   ParaView
  Module:    vtkPVBoxChartView.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVBoxChartView.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVBoxChartView);
//----------------------------------------------------------------------------
vtkPVBoxChartView::vtkPVBoxChartView()
{
  this->SetChartType("Box");
}

//----------------------------------------------------------------------------
vtkPVBoxChartView::~vtkPVBoxChartView()
{
}

//----------------------------------------------------------------------------
void vtkPVBoxChartView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
