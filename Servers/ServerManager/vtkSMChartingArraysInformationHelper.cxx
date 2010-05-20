/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartingArraysInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartingArraysInformationHelper.h"

#include "vtkObjectFactory.h"

#include "vtkSMChartRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMChartingArraysInformationHelper);
//----------------------------------------------------------------------------
vtkSMChartingArraysInformationHelper::vtkSMChartingArraysInformationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMChartingArraysInformationHelper::~vtkSMChartingArraysInformationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMChartingArraysInformationHelper::UpdateProperty(
  vtkIdType vtkNotUsed(connectionId),
  int vtkNotUsed(serverIds), vtkClientServerID vtkNotUsed(objectId),
  vtkSMProperty* prop)
{
  vtkSMChartRepresentationProxy* cr =
    vtkSMChartRepresentationProxy::SafeDownCast(
      prop->GetParent());
  if (!cr)
    {
    vtkWarningMacro("vtkSMChartingArraysInformationHelper can only be used on"
      " Chart representation proxies.");
    return;
    }

  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkWarningMacro("vtkSMChartingArraysInformationHelper can only update "
      "vtkSMStringVectorProperty.");
    return;
    }
 
  int num_series = cr->GetNumberOfSeries();
  svp->SetNumberOfElements(num_series);
  for (int cc=0; cc < num_series; cc++)
    {
    svp->SetElement(cc, cr->GetSeriesName(cc));
    }
}

//----------------------------------------------------------------------------
void vtkSMChartingArraysInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


