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
vtkCxxRevisionMacro(vtkSMTimeStepsInformationHelper, "1.1.2.1");

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
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop)
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
  pm->SendStream(vtkProcessModule::GetRootId(serverIds), stream);

  vtkClientServerStream timeSteps;
  int retVal = 
    pm->GetLastResult(
      vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &timeSteps);

  if(!retVal)
    {
    vtkErrorMacro("Error getting array settings from server.");
    return;
    }

  int numArgs = timeSteps.GetNumberOfArguments(0);
  if (numArgs == 1)
    {
    vtkTypeUInt32 length;
    if (timeSteps.GetArgumentLength(0, 0, &length))
      {
      dvp->SetNumberOfElements(length);
      if (length>0)
        {
        timeSteps.GetArgument(0, 0, dvp->GetElements(), length);
        }
      }
    }

  pm->DeleteStreamObject(serverObjID, stream);
  pm->SendStream(vtkProcessModule::GetRootId(serverIds), stream);
}

//---------------------------------------------------------------------------
void vtkSMTimeStepsInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
