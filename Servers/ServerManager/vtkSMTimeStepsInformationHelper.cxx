/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeStepsInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeStepsInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMTimeStepsInformationHelper);

//---------------------------------------------------------------------------
vtkSMTimeStepsInformationHelper::vtkSMTimeStepsInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMTimeStepsInformationHelper::~vtkSMTimeStepsInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMTimeStepsInformationHelper::UpdateProperty(
  vtkIdType connectionId, int serverIds, vtkClientServerID objectId, 
  vtkSMProperty* prop)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (!dvp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMDoubleVectorProperty was needed.");
    return;
    }

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID serverObjID = 
    pm->NewStreamObject("vtkPVServerTimeSteps", stream);
  stream << vtkClientServerStream::Invoke
         << serverObjID << "SetProcessModule" << pm->GetProcessModuleID()
         << vtkClientServerStream::End;
  stream << vtkClientServerStream::Invoke
         << serverObjID << "GetTimeSteps" << objectId
         << vtkClientServerStream::End;
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), stream);

  vtkClientServerStream timeSteps;
  int retVal = 
    pm->GetLastResult(connectionId,
      vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &timeSteps);

  if(!retVal)
    {
    vtkErrorMacro("Error getting array settings from server.");
    return;
    }

  int numArgs = timeSteps.GetNumberOfArguments(0);
  if (numArgs >= 2)
    {
    vtkTypeUInt32 length;
    if (timeSteps.GetArgumentLength(0, 1, &length))
      {
      dvp->SetNumberOfElements(length);
      double *values = new double[length];
      if (length>0)
        {
        timeSteps.GetArgument(0, 1, values, length);
        }
      dvp->SetElements(values);
      delete[] values;
      }
    }
  else
    {
    // Clear out the time steps.
    dvp->SetNumberOfElements(0);
    }

  pm->DeleteStreamObject(serverObjID, stream);
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), stream);
}

//---------------------------------------------------------------------------
void vtkSMTimeStepsInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
