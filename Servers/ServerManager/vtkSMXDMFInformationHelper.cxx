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
#include "vtkPVXMLElement.h"

vtkStandardNewMacro(vtkSMXDMFInformationHelper);
vtkCxxRevisionMacro(vtkSMXDMFInformationHelper, "1.5");

//---------------------------------------------------------------------------
vtkSMXDMFInformationHelper::vtkSMXDMFInformationHelper()
{
  this->InfoType = 0;
}

//---------------------------------------------------------------------------
vtkSMXDMFInformationHelper::~vtkSMXDMFInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMXDMFInformationHelper::UpdateProperty(
  vtkIdType connectionId, int serverIds, vtkClientServerID objectId, 
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

  // Create server-side helper if necessary.
  vtkClientServerStream str;

  vtkClientServerID serverSideID = 
    pm->NewStreamObject("vtkPVServerXDMFParameters", str);
  
  switch (this->InfoType)
    {
    case 0:
    {
    // Get the parameters from the server.
    str << vtkClientServerStream::Invoke
        << serverSideID << "GetParameters" << objectId
        << vtkClientServerStream::End;
    pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);
    
    vtkClientServerStream parameters;
    int retVal = pm->GetLastResult(connectionId,
        vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &parameters);
    
    pm->DeleteStreamObject(serverSideID, str);
    pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);
    
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
    return;
    case 1:
    {
    // Get the domain names from the server.
    str << vtkClientServerStream::Invoke
        << serverSideID << "GetDomains" << objectId
        << vtkClientServerStream::End;
    pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);

    vtkClientServerStream domains;
    int retVal = pm->GetLastResult(connectionId,
        vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &domains);
    
    pm->DeleteStreamObject(serverSideID, str);
    pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);
    
    if(!retVal)
      {
      vtkErrorMacro("Error getting domains from server.");
      return;
      }

    // Add each parameter locally.
    int numDomains = domains.GetNumberOfArguments(0);
    
    svp->SetNumberOfElements(numDomains);
    for(int i=0; i < numDomains; ++i)
      {
      const char* name;
      if(!domains.GetArgument(0, i, &name))
        {
        vtkErrorMacro("Error parsing domain name.");
        return;
        } 
      svp->SetElement(i, name);
      }
    }
    return;
    case 2:
    {
    // Get the grid names from the server.
    str << vtkClientServerStream::Invoke
        << serverSideID << "GetGrids" << objectId
        << vtkClientServerStream::End;
    pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);

    vtkClientServerStream grids;
    int retVal = pm->GetLastResult(connectionId,
        vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, &grids);
    
    pm->DeleteStreamObject(serverSideID, str);
    pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str);
    
    if(!retVal)
      {
      vtkErrorMacro("Error getting grids from server.");
      return;
      }

    // Add each parameter locally.
    int numGrids = grids.GetNumberOfArguments(0);
    
    svp->SetNumberOfElements(numGrids);
    for(int i=0; i < numGrids; ++i)
      {
      const char* name;
      if(!grids.GetArgument(0, i, &name))
        {
        vtkErrorMacro("Error parsing grid name.");
        return;
        } 
      svp->SetElement(i, name);
      }
    }
    return;
    }
}
//---------------------------------------------------------------------------
int vtkSMXDMFInformationHelper::ReadXMLAttributes(
  vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
    {
    return 0;
    }

  const char* info_type = element->GetAttribute("info_type");
  this->InfoType = 0;
  if(info_type)
    {
    if (strcmp(info_type, "domains") == 0)
      {
      this->InfoType = 1;
      }
    else if (strcmp(info_type, "grids") == 0)
      {
      this->InfoType = 2;
      }
    }  
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMXDMFInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
