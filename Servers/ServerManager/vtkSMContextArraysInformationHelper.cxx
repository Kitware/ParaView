/*=========================================================================

  Program:   ParaView
  Module:    vtkSMContextArraysInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMContextArraysInformationHelper.h"

#include "vtkObjectFactory.h"

#include "vtkSMXYChartRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMContextArraysInformationHelper);
//----------------------------------------------------------------------------
vtkSMContextArraysInformationHelper::vtkSMContextArraysInformationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMContextArraysInformationHelper::~vtkSMContextArraysInformationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMContextArraysInformationHelper::UpdateProperty(
  vtkIdType vtkNotUsed(connectionId),
  int vtkNotUsed(serverIds), vtkClientServerID vtkNotUsed(objectId),
  vtkSMProperty* prop)
{
  vtkSMXYChartRepresentationProxy* rep =
    vtkSMXYChartRepresentationProxy::SafeDownCast(
      prop->GetParent());
  if (!rep)
    {
    vtkWarningMacro("vtkSMContextArraysInformationHelper can only be used on"
      " XY Chart representation proxies.");
    return;
    }

  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkWarningMacro("vtkSMContextArraysInformationHelper can only update "
      "vtkSMStringVectorProperty.");
    return;
    }

  int num_series = rep->GetNumberOfSeries();
  svp->SetNumberOfElements(num_series);
  for (int i = 0; i < num_series; ++i)
    {
    svp->SetElement(i, rep->GetSeriesName(i));
    }
}

//----------------------------------------------------------------------------
void vtkSMContextArraysInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


