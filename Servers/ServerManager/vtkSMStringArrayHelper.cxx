/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringArrayHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringArrayHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMStringArrayHelper);

//---------------------------------------------------------------------------
vtkSMStringArrayHelper::vtkSMStringArrayHelper()
{
}

//---------------------------------------------------------------------------
vtkSMStringArrayHelper::~vtkSMStringArrayHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMStringArrayHelper::UpdateProperty(
  vtkIdType connectionId,  int serverIds, vtkClientServerID objectId, 
  vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMStringVectorProperty was needed.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke 
      << objectId << prop->GetCommand()
      << vtkClientServerStream::End;
  vtkClientServerID arrayID = pm->GetUniqueID();
  str << vtkClientServerStream::Assign << arrayID
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  
  vtkClientServerID serverSideID = 
    pm->NewStreamObject("vtkPVStringArrayHelper", str);

  str << vtkClientServerStream::Invoke
      << serverSideID << "SetProcessModule" << pm->GetProcessModuleID()
      << vtkClientServerStream::End;

  // Get the parameters from the server.
  str << vtkClientServerStream::Invoke
      << serverSideID << "GetStringList" << arrayID
      << vtkClientServerStream::End;
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str, 1);

  vtkClientServerStream stringList;
  int retVal = 
    pm->GetLastResult(connectionId, 
      vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &stringList);

  pm->DeleteStreamObject(serverSideID, str);
  pm->DeleteStreamObject(arrayID, str);
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str, 0);

  if(!retVal)
    {
    vtkErrorMacro("Error getting array settings from server.");
    return;
    }

  int numStrings = stringList.GetNumberOfArguments(0);
  svp->SetNumberOfElements(numStrings);
  for (int i=0; i<numStrings; i++)
    {
    const char* astring;
    if(!stringList.GetArgument(0, i, &astring))
      {
      vtkErrorMacro("Error getting string name from object.");
      break;
      }
    svp->SetElement(i, astring);
    }
}

//---------------------------------------------------------------------------
void vtkSMStringArrayHelper::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
