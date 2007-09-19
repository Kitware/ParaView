/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYPlotRepresentationProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXYPlotRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMProxyProperty.h"

vtkStandardNewMacro(vtkSMXYPlotRepresentationProxy);
vtkCxxRevisionMacro(vtkSMXYPlotRepresentationProxy, "1.3");
//----------------------------------------------------------------------------
vtkSMXYPlotRepresentationProxy::vtkSMXYPlotRepresentationProxy()
{
}

//----------------------------------------------------------------------------
vtkSMXYPlotRepresentationProxy::~vtkSMXYPlotRepresentationProxy()
{
}

//----------------------------------------------------------------------------
bool vtkSMXYPlotRepresentationProxy::EndCreateVTKObjects()
{
  if (!this->Superclass::EndCreateVTKObjects())
    {
    return false;
    }

  vtkSMProxy* subProxy = this->GetSubProxy("DummyConsumer");
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    subProxy->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(this->PostProcessorProxy);
  return true;
}


//----------------------------------------------------------------------------
void vtkSMXYPlotRepresentationProxy::Update(vtkSMViewProxy* view)
{
  if (!this->ObjectsCreated)
    {
    vtkErrorMacro("Objects not created yet!");
    return;
    }

  this->Superclass::Update(view);

  vtkSMProxy* subProxy = this->GetSubProxy("DummyConsumer");
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    subProxy->GetProperty("Input"));
  pp->UpdateDependentDomains();
}

//----------------------------------------------------------------------------
void vtkSMXYPlotRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


