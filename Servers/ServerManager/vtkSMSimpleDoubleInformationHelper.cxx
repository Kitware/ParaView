/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSimpleDoubleInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSimpleDoubleInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkSMSimpleDoubleInformationHelper);

//---------------------------------------------------------------------------
vtkSMSimpleDoubleInformationHelper::vtkSMSimpleDoubleInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMSimpleDoubleInformationHelper::~vtkSMSimpleDoubleInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMSimpleDoubleInformationHelper::UpdateProperty(
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
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);

  // Get the result
  const vtkClientServerStream& res =     
    pm->GetLastResult(connectionId, vtkProcessModule::GetRootId(serverIds));


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

  // If single value, both float and double works
  if (argType == vtkClientServerStream::float64_value ||
      argType == vtkClientServerStream::float32_value)
    {
    double ires;
    int retVal = res.GetArgument(0, 0, &ires);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    dvp->SetNumberOfElements(1);
    dvp->SetElement(0, ires);
    }
  // If array, handle 32 bit and 64 bit separately
  else if (argType == vtkClientServerStream::float64_array)
    {
    vtkTypeUInt32 length;
    res.GetArgumentLength(0, 0, &length);
    if (length >= 128)
      {
      vtkErrorMacro("Only arguments of length 128 or less are supported");
      return;
      }
    double values[128];
    int retVal = res.GetArgument(0, 0, values, length);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    dvp->SetNumberOfElements(length);
    dvp->SetElements(values);
    }
  else if (argType == vtkClientServerStream::float32_array)
    {
    vtkTypeUInt32 length;
    res.GetArgumentLength(0, 0, &length);
    if (length >= 128)
      {
      vtkErrorMacro("Only arguments of length 128 or less are supported");
      return;
      }
    float values[128];
    int retVal = res.GetArgument(0, 0, values, length);
    if (!retVal)
      {
      vtkErrorMacro("Error getting argument.");
      return;
      }
    dvp->SetNumberOfElements(length);
    for (unsigned int i=0; i<length; i++)
      {
      dvp->SetElement(i, values[i]);
      }
    }

}

//---------------------------------------------------------------------------
void vtkSMSimpleDoubleInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
