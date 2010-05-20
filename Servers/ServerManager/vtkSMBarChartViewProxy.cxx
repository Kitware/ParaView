/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBarChartViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkQtBarChartView.h"

vtkStandardNewMacro(vtkSMBarChartViewProxy);
//----------------------------------------------------------------------------
vtkSMBarChartViewProxy::vtkSMBarChartViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMBarChartViewProxy::~vtkSMBarChartViewProxy()
{
}

//----------------------------------------------------------------------------
vtkQtChartView* vtkSMBarChartViewProxy::NewChartView()
{
  return vtkQtBarChartView::New();
}

//----------------------------------------------------------------------------
vtkQtBarChartView* vtkSMBarChartViewProxy::GetBarChartView()
{
  return vtkQtBarChartView::SafeDownCast(this->ChartView);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetOutlineStyle(int outline)
{
  this->GetBarChartView()->SetOutlineStyle(outline);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetBarGroupFraction(float fraction)
{
  this->GetBarChartView()->SetBarGroupFraction(fraction);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetBarWidthFraction(float fraction)
{
  this->GetBarChartView()->SetBarWidthFraction(fraction);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::SetHelpFormat(const char* format)
{
  this->GetBarChartView()->SetHelpFormat(format);
}

//----------------------------------------------------------------------------
void vtkSMBarChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


