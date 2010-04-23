/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleArrayInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDoubleArrayInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMDoubleArrayInformationHelper);
//-----------------------------------------------------------------------------
vtkSMDoubleArrayInformationHelper::vtkSMDoubleArrayInformationHelper()
{
}

//-----------------------------------------------------------------------------
vtkSMDoubleArrayInformationHelper::~vtkSMDoubleArrayInformationHelper()
{
}

//-----------------------------------------------------------------------------
void vtkSMDoubleArrayInformationHelper::UpdateProperty(
  vtkIdType connectionId,  int serverIds, vtkClientServerID objectId, 
  vtkSMProperty* prop)
{
  vtkSMDoubleVectorProperty* ivp = vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (!ivp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when a vtkSMDoubleVectorProperty was needed.");
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkClientServerStream stream;

  vtkClientServerID serverSideID = 
    pm->NewStreamObject("vtkPVServerArrayHelper", stream);

  stream << vtkClientServerStream::Invoke
    << serverSideID << "SetProcessModule" 
    << pm->GetProcessModuleID() 
    << vtkClientServerStream::End;

  stream << vtkClientServerStream::Invoke
    << serverSideID << "GetArray" << objectId << prop->GetCommand() 
    << vtkClientServerStream::End;
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), stream);

  vtkClientServerStream values;
  int retVal = pm->GetLastResult(connectionId,
    vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &values);
  pm->DeleteStreamObject(serverSideID, stream);
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), stream);

  if (!retVal)
    {
    vtkErrorMacro("Error getting array from server.");
    return;
    }

  int numValues = values.GetNumberOfArguments(0);
  ivp->SetNumberOfElements(numValues);
  for (int i=0; i < numValues; ++i)
    {
    double value;
    if (!values.GetArgument(0, i, &value))
      {
      vtkErrorMacro("Error getting value.");
      break;
      }
    ivp->SetElement(i, value);
    }
}


//-----------------------------------------------------------------------------
void vtkSMDoubleArrayInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
