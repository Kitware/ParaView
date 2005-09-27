/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleProxyInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleProxyInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkSMSimpleProxyInformationHelper);
vtkCxxRevisionMacro(vtkSMSimpleProxyInformationHelper, "1.2");


//---------------------------------------------------------------------------
vtkSMSimpleProxyInformationHelper::vtkSMSimpleProxyInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMSimpleProxyInformationHelper::~vtkSMSimpleProxyInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMSimpleProxyInformationHelper::UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(prop);
  if (!pp)
    {
     vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMProxyProperty was needed.");
    return;
    }

  if (!prop->GetCommand())
    {
    return;
    }
  
  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke 
      << objectId << prop->GetCommand()
      << vtkClientServerStream::End;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(vtkProcessModule::GetRootId(serverIds), str);

  // Get the result
  const vtkClientServerStream& res = 
    pm->GetLastResult(vtkProcessModule::GetRootId(serverIds));

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
    {
    return;
    }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
    {
    return;
    }
  
  int argType = res.GetArgumentType(0, 0);

  if (argType != vtkClientServerStream::vtk_object_pointer)
    {
    vtkErrorMacro("Call did not return a vtk object");
    return;
    }
  vtkObjectBase* obj;
  int retVal = res.GetArgument(0, 0, &obj);
  if (!retVal)
    {
    vtkErrorMacro("Error getting argument.");
    return;
    }
  vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(obj);
  if (!proxy)
    {
    vtkErrorMacro("Call did not return a proxy");
    return;
    }
  pp->RemoveAllProxies();
  pp->AddProxy(proxy);
  
}

//---------------------------------------------------------------------------
void vtkSMSimpleProxyInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
