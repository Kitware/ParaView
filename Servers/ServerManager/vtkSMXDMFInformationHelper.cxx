/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXDMFInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXDMFInformationHelper.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMXDMFInformationHelper);
vtkCxxRevisionMacro(vtkSMXDMFInformationHelper, "1.2");

//---------------------------------------------------------------------------
vtkSMXDMFInformationHelper::vtkSMXDMFInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMXDMFInformationHelper::~vtkSMXDMFInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMXDMFInformationHelper::UpdateProperty(
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (!svp)
    {
    vtkErrorMacro("A null property or a property of a different type was "
                  "passed when vtkSMStringVectorProperty was needed.");
    return;
    }

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Create server-side helper if necessary.
  vtkClientServerStream str;

  vtkClientServerID serverSideID = 
    pm->NewStreamObject("vtkPVServerXDMFParameters", str);
  
  // Get the parameters from the server.
  str << vtkClientServerStream::Invoke
      << serverSideID << "GetParameters" << objectId
      << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::GetRootId(serverIds), str);

  vtkClientServerStream parameters;
  int retVal = 
    pm->GetLastResult(vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &parameters);

  pm->DeleteStreamObject(serverSideID, str);
  pm->SendStream(vtkProcessModule::GetRootId(serverIds), str);

  if(!retVal)
    {
    vtkErrorMacro("Error getting parameters from server.");
    return;
    }

  // Add each parameter locally.
  int numParameters = parameters.GetNumberOfArguments(0)/3;

  // 5 component tuples: name, current value, first index, stride, count
  svp->SetNumberOfElements(numParameters*5);
  for(int i=0; i < numParameters; ++i)
    {
    const char* name;
    int index;
    int range[3];
    if(!parameters.GetArgument(0, 3*i, &name))
      {
      vtkErrorMacro("Error parsing parameter name.");
      return;
      }
    svp->SetElement(5*i, name);
    if(!parameters.GetArgument(0, 3*i + 1, &index))
      {
      vtkErrorMacro("Error parsing parameter index.");
      return;
      }
    char tmpstr[128];
    sprintf(tmpstr, "%d", index);
    svp->SetElement(5*i+1, tmpstr);
    if(!parameters.GetArgument(0, 3*i + 2, range, 3))
      {
      vtkErrorMacro("Error parsing parameter range.");
      return;
      }
    for (int j=0; j<3; j++)
      {
      sprintf(tmpstr, "%d", range[j]);
      svp->SetElement(5*i+2+j, tmpstr);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMXDMFInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
