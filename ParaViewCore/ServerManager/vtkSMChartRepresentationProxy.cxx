/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartRepresentationProxy.h"

#include "vtkChartRepresentation.h"
#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMChartRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::vtkSMChartRepresentationProxy()
{
  this->SetSIClassName("vtkSIChartRepresentationProxy");
}

//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::~vtkSMChartRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkChartRepresentation* vtkSMChartRepresentationProxy::GetRepresentation()
{
  this->CreateVTKObjects();
  return vtkChartRepresentation::SafeDownCast(
    this->GetClientSideObject());
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::SetPropertyModifiedFlag(
  const char* name, int flag)
{
  if (name && strcmp(name, "Input") == 0)
    {
    vtkSMPropertyHelper helper(this, name);
    for (unsigned int cc=0; cc < helper.GetNumberOfElements(); cc++)
      {
      vtkSMSourceProxy* input = vtkSMSourceProxy::SafeDownCast(
        helper.GetAsProxy(cc));
      if (input)
        {
        input->CreateSelectionProxies();
        vtkSMSourceProxy* esProxy = input->GetSelectionOutput(
          helper.GetOutputPort(cc));
        if (!esProxy)
          {
          vtkErrorMacro("Input proxy does not support selection extraction.");
          }
        else
          {
          // We use these internal properties since we need to add consumer dependecy
          // on this proxy so that MarkModified() is called correctly.
          vtkSMPropertyHelper(this, "InternalInput1").Set(esProxy, 1);
          this->UpdateProperty("InternalInput1");
          }
        }
      }
    }
  this->Superclass::SetPropertyModifiedFlag(name, flag);
}
