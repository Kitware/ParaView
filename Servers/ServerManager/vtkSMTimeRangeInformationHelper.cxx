/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTimeRangeInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMTimeRangeInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMTimeRangeInformationHelper);

//---------------------------------------------------------------------------
vtkSMTimeRangeInformationHelper::vtkSMTimeRangeInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMTimeRangeInformationHelper::~vtkSMTimeRangeInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMTimeRangeInformationHelper::UpdateProperty(
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

  vtkClientServerStream timeRange;
  int retVal = 
    pm->GetLastResult(connectionId,
      vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &timeRange);

  if(!retVal)
    {
    vtkErrorMacro("Error getting array settings from server.");
    return;
    }

  int numArgs = timeRange.GetNumberOfArguments(0);
  if (numArgs >= 1)
    {
    vtkTypeUInt32 length;
    if (timeRange.GetArgumentLength(0, 0, &length))
      {
      dvp->SetNumberOfElements(length);
      double *values = 0;
      if (length == 2)
        {
        values = new double[length];
        timeRange.GetArgument(0, 0, values, length);
        dvp->SetElements(values);
        delete[] values;
        }
      else
        {
        vtkErrorMacro("vtkPVServerTimeSteps returned invalid array length "
                      "for time range.");
        }
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
void vtkSMTimeRangeInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
