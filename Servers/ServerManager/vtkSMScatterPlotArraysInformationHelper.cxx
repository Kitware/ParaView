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

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkSMScatterPlotRepresentationProxy.h"
#include "vtkSMStringVectorProperty.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMScatterPlotArraysInformationHelper);
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
 
  int num_arrays = cr->GetNumberOfSeries();
  
  int numOfEntries = 0;
  for (int i = 0; i < num_arrays; ++i)
    {
    int numOfComponents = cr->GetSeriesNumberOfComponents(i);
    numOfEntries += numOfComponents;
    if (numOfComponents > 1)
      {
      ++numOfEntries;
      }
    }
  svp->SetNumberOfElements(numOfEntries);

  int entrie = 0;
  for (int cc=0; cc < num_arrays; cc++)
    {
    vtkStdString name = cr->GetSeriesName(cc);
    vtkStdString type = "";
    switch(cr->GetSeriesType(cc))
      {
      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        type = "point";
        break;
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        type = "cell";
        break;
      case vtkDataObject::NUMBER_OF_ASSOCIATIONS:
      default:
        type = "coord";
        break;
      }
    int numberOfComponents = cr->GetSeriesNumberOfComponents(cc);
    if (numberOfComponents > 1 )
      {
      vtksys_ios::stringstream str;
      str << type << "," << name << ",-1" ;
      svp->SetElement(entrie++, str.str().c_str());
      for( int i=0; i < numberOfComponents; ++i)
        {
        vtksys_ios::stringstream str2;
        str2 << type << "," << name << "," << i ;
        svp->SetElement(entrie++, str2.str().c_str());
        }
      }
    else
      {
      vtksys_ios::stringstream str;
      str << type << "," << name;
      svp->SetElement(entrie++, str.str().c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMScatterPlotArraysInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


