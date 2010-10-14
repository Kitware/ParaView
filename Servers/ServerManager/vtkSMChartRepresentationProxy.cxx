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
#include "vtkProcessModule.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMChartRepresentationProxy);
//----------------------------------------------------------------------------
vtkSMChartRepresentationProxy::vtkSMChartRepresentationProxy()
{
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
void vtkSMChartRepresentationProxy::AddInput(unsigned int inputPort,
  vtkSMSourceProxy* input, unsigned int outputPort, const char* method)
{
  this->Superclass::AddInput(inputPort, input, outputPort, method);
  input->CreateSelectionProxies();
  if (inputPort == 0)
    {
    vtkSMSourceProxy* esProxy = input->GetSelectionOutput(outputPort);
    if (!esProxy)
      {
      vtkErrorMacro("Input proxy does not support selection extraction.");
      return;
      }

    // We use these internal properties since we need to add consumer dependecy
    // on this proxy so that MarkModified() is called correctly.
    vtkSMPropertyHelper(this, "InternalInput1").Set(esProxy, 1);
    this->UpdateProperty("InternalInput1");
    }
}

//-----------------------------------------------------------------------------
void vtkSMChartRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->Superclass::CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMProxy* optionsProxy = this->GetSubProxy("PlotOptions");
  if (optionsProxy)
    {
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Invoke
      << this->GetID()
      << "SetOptions"
      << optionsProxy->GetID()
      << vtkClientServerStream::End;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID, this->Servers, stream);
    }
}
