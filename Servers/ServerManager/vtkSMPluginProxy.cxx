/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPluginProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMPluginProxy.h"

#include "vtkClientServerStream.h"
#include "vtkIntArray.h"
#include "vtkStringArray.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVPluginInformation.h"
#include "vtkSMIntVectorProperty.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMPluginProxy);
vtkCxxRevisionMacro(vtkSMPluginProxy, "1.1");

//----------------------------------------------------------------------------
vtkSMPluginProxy::vtkSMPluginProxy()
{
  this->PluginInfo = vtkPVPluginInformation::New(); 
  this->ServerManagerXML = vtkStringArray::New();
  this->PythonModuleNames = vtkStringArray::New();
  this->PythonModuleSources = vtkStringArray::New();
  this->PythonPackageFlags = vtkIntArray::New();
}

//----------------------------------------------------------------------------
vtkSMPluginProxy::~vtkSMPluginProxy()
{
  this->PluginInfo->Delete();
  this->ServerManagerXML->Delete();
  this->PythonModuleNames->Delete();
  this->PythonModuleSources->Delete();
  this->PythonPackageFlags->Delete();
}

//----------------------------------------------------------------------------
vtkPVPluginInformation* vtkSMPluginProxy::Load(const char* filename)
{
  vtkSMIntVectorProperty* loadedProperty = 
    vtkSMIntVectorProperty::SafeDownCast(
      this->GetProperty("Loaded"));
  if(!loadedProperty)
    {
    vtkErrorMacro("The plugin proxy don't have Loaded property!");
    return 0;
    }
  vtkClientServerStream stream;
  stream  << vtkClientServerStream::Invoke
          << this->GetID()
          << "SetFileName"
          << filename
          << vtkClientServerStream::End;
          
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendStream(this->ConnectionID, this->Servers, stream);

  this->UpdatePropertyInformation();

  pm->GatherInformation(this->GetConnectionID(), this->Servers, 
    this->PluginInfo, this->GetID());
  
  return this->PluginInfo;
}

//----------------------------------------------------------------------------
void vtkSMPluginProxy::UpdatePropertyInformation()
{
  this->Superclass::UpdatePropertyInformation();
  vtkSMProperty* prop = this->GetProperty("ServerManagerXML");
  this->GetPropertyArray(this->GetConnectionID(),
    this->GetServers(), this->GetID(), prop, this->ServerManagerXML);

#ifdef VTK_WRAP_PYTHON
  prop = this->GetProperty("PythonModuleNames");
  this->GetPropertyArray(this->GetConnectionID(),
    this->GetServers(), this->GetID(), prop, this->PythonModuleNames);
  prop = this->GetProperty("PythonModuleSources");
  this->GetPropertyArray(this->GetConnectionID(),
    this->GetServers(), this->GetID(), prop, this->PythonModuleSources);
  prop = this->GetProperty("PythonPackageFlags");
  this->GetPropertyArray(this->GetConnectionID(),
    this->GetServers(), this->GetID(), prop, this->PythonPackageFlags);
#endif //VTK_WRAP_PYTHON
}
  
//---------------------------------------------------------------------------
int vtkSMPluginProxy::GetPropertyArray(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId, vtkSMProperty* prop,
    vtkAbstractArray* valuearray)
{
  vtkStringArray* sva = vtkStringArray::SafeDownCast(valuearray);
  vtkIntArray* iva = vtkIntArray::SafeDownCast(valuearray);
  if (!sva && !iva)
    {
    vtkErrorMacro("A null array or an array of a different type was "
                  "passed when vtkStringArray or vtkIntArray was expected.");
    return 0;
    }

  vtkstd::string streamobjectname, streamobjectcmd;
  if(sva)
    {
    streamobjectname = "vtkPVStringArrayHelper";
    streamobjectcmd = "GetStringList";
    }
  else
    {
    streamobjectname = "vtkPVServerArrayHelper";
    streamobjectcmd = "GetArray";
    }
    
  vtkClientServerStream valueList;
  int retVal = this->GetArrayStream(connectionId, serverIds, objectId,
    prop->GetCommand(), &valueList, streamobjectname.c_str(), streamobjectcmd.c_str());
  if(!retVal)
    {
    vtkErrorMacro("Error getting array from server.");
    return 0;
    }

  int numValues = valueList.GetNumberOfArguments(0);
  int i;
  if(sva)
    {
    sva->SetNumberOfComponents(1);
    sva->SetNumberOfTuples(numValues);
    for (i=0; i<numValues; i++)
      {
      const char* astring;
      if(!valueList.GetArgument(0, i, &astring))
        {
        vtkErrorMacro("Error getting string name from object.");
        break;
        }
      sva->SetValue(i, astring);
      }
    }
  else
    {
    iva->SetNumberOfComponents(1);
    iva->SetNumberOfTuples(numValues);
    for (i=0; i < numValues; ++i)
      {
      int value;
      if (!valueList.GetArgument(0, i, &value))
        {
        vtkErrorMacro("Error getting value.");
        break;
        }
      iva->SetValue(i, value);
      }
    }
  return 1;
}
//---------------------------------------------------------------------------
int vtkSMPluginProxy::GetArrayStream(vtkIdType connectionId,
    int serverIds, vtkClientServerID objectId,
    const char* serverCmd, vtkClientServerStream* stringlist,
    const char* streamobjectname, const char* streamobjectcmd)
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke 
      << objectId << serverCmd
      << vtkClientServerStream::End;
  vtkClientServerID arrayID = pm->GetUniqueID();
  str << vtkClientServerStream::Assign << arrayID
      << vtkClientServerStream::LastResult
      << vtkClientServerStream::End;
  
  vtkClientServerID serverSideID = 
    pm->NewStreamObject(streamobjectname, str);

  str << vtkClientServerStream::Invoke
      << serverSideID << "SetProcessModule" << pm->GetProcessModuleID()
      << vtkClientServerStream::End;

  // Get the parameters from the server.
  str << vtkClientServerStream::Invoke
      << serverSideID << streamobjectcmd << arrayID
      << vtkClientServerStream::End;
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str, 1);

  int retVal = 
    pm->GetLastResult(connectionId, 
      vtkProcessModule::GetRootId(serverIds)).GetArgument(0, 0, stringlist);

  pm->DeleteStreamObject(serverSideID, str);
  pm->DeleteStreamObject(arrayID, str);
  pm->SendStream(connectionId, vtkProcessModule::GetRootId(serverIds), str, 0);

  return retVal;
}

//----------------------------------------------------------------------------
void vtkSMPluginProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkIndent i2 = indent.GetNextIndent();
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ServerManagerXML: " 
    << (this->ServerManagerXML ? "(exists)" : "(none)") << endl;
  os << indent << "PythonModuleNames: " 
    << (this->PythonModuleNames ? "(exists)" : "(none)") << endl;
  os << indent << "PythonModuleSources: " 
    << (this->PythonModuleSources ? "(exists)" : "(none)") << endl;
  os << indent << "PythonPackageFlags: " 
    << (this->PythonPackageFlags ? "(exists)" : "(none)") << endl;
  os << indent << "PluginInfo: "  << endl;
  this->PluginInfo->PrintSelf(os, i2);
}
