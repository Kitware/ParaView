/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineChartViewProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLineChartViewProxy.h"

#include "vtkObjectFactory.h"
#include "vtkQtLineChartView.h"

vtkStandardNewMacro(vtkSMLineChartViewProxy);
//----------------------------------------------------------------------------
vtkSMLineChartViewProxy::vtkSMLineChartViewProxy()
{
}

//----------------------------------------------------------------------------
vtkSMLineChartViewProxy::~vtkSMLineChartViewProxy()
{
}

//----------------------------------------------------------------------------
vtkQtChartView* vtkSMLineChartViewProxy::NewChartView()
{
  return vtkQtLineChartView::New();
}

//----------------------------------------------------------------------------
vtkQtLineChartView* vtkSMLineChartViewProxy::GetLineChartView()
{
  return vtkQtLineChartView::SafeDownCast(this->ChartView);
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::SetHelpFormat(const char* format)
{
  this->GetLineChartView()->SetHelpFormat(format);
}

//----------------------------------------------------------------------------
void vtkSMLineChartViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


