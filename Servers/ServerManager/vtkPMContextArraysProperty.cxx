/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMContextArraysProperty.h"

#include "vtkChartRepresentation.h"
#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"

vtkStandardNewMacro(vtkPMContextArraysProperty);
//----------------------------------------------------------------------------
vtkPMContextArraysProperty::vtkPMContextArraysProperty()
{
}

//----------------------------------------------------------------------------
vtkPMContextArraysProperty::~vtkPMContextArraysProperty()
{
}
//----------------------------------------------------------------------------
bool vtkPMContextArraysProperty::Pull(vtkSMMessage* msgToFill)
{
  if (!this->InformationOnly)
    {
    return false;
    }

  vtkChartRepresentation* vtk_rep = vtkChartRepresentation::SafeDownCast(
    this->GetVTKObject());
  if (!vtk_rep)
    {
    vtkErrorMacro(
      "This helper can only be used for proxies with vtkChartRepresentation");
    return false;
    }

  // Create property and add it to the message
  ProxyState_Property *prop = msgToFill->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *variant = prop->mutable_value();
  variant->set_type(Variant::STRING);
  int num_series = vtk_rep->GetNumberOfSeries();
  for (int i = 0; i < num_series; ++i)
    {
    variant->add_txt(vtk_rep->GetSeriesName(i));
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPMContextArraysProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
