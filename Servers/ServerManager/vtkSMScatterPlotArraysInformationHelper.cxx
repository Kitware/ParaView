/*=========================================================================

  Program:   ParaView
  Module:    vtkSMScatterPlotArraysInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMScatterPlotArraysInformationHelper.h"

#include "vtkObjectFactory.h"

#include "vtkSMScatterPlotRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkDataObject.h"

vtkStandardNewMacro(vtkSMScatterPlotArraysInformationHelper);
vtkCxxRevisionMacro(vtkSMScatterPlotArraysInformationHelper, "1.2");
//----------------------------------------------------------------------------
vtkSMScatterPlotArraysInformationHelper::vtkSMScatterPlotArraysInformationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMScatterPlotArraysInformationHelper::~vtkSMScatterPlotArraysInformationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotArraysInformationHelper::UpdateProperty(
  vtkIdType vtkNotUsed(connectionId),
  int vtkNotUsed(serverIds), vtkClientServerID vtkNotUsed(objectId),
  vtkSMProperty* prop)
{
  vtkSMScatterPlotRepresentationProxy* cr =
    vtkSMScatterPlotRepresentationProxy::SafeDownCast(
      prop->GetParent());
  if (!cr)
    {
    vtkWarningMacro("vtkSMScatterPlotArraysInformationHelper can only be used on"
      " Scatter Plot representation proxies.");
    return;
    }

  vtkSMStringVectorProperty* svp =
    vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkWarningMacro("vtkSMScatterPlotArraysInformationHelper can only update "
      "vtkSMStringVectorProperty.");
    return;
    }
 
  int num_series = cr->GetNumberOfSeries();
  svp->SetNumberOfElements(num_series);
  for (int cc=0; cc < num_series; cc++)
    {
    vtkStdString name = cr->GetSeriesName(cc);
    switch(cr->GetSeriesType(cc))
      {
      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        name = "point," + name;
        break;
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        name = "cell," + name;
        break;
      case vtkDataObject::NUMBER_OF_ASSOCIATIONS:
      default:
        name = "coord," + name;
        break;
      }
    svp->SetElement(cc, name );
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotArraysInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


