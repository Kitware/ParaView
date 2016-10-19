/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCatalystSessionCore.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCatalystSessionCore.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVInformation.h"
#include "vtkPVSession.h"
#include "vtkSISourceProxy.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"

#include <assert.h>
#include <sstream>
#include <string>
#include <vector>

//****************************************************************************/
//                        Internal Class
//****************************************************************************/
class vtkPVCatalystSessionCore::vtkInternal
{
public:
  vtkInternal(vtkPVCatalystSessionCore* parent)
  {
    this->Owner = parent;
    this->ObserverId = this->Owner->AddObserver(vtkCommand::UpdateDataEvent, this,
      &vtkPVCatalystSessionCore::vtkInternal::UpdateToCatalyseSourceProxy);
  }
  //---------------------------------------------------------------------------
  ~vtkInternal()
  {
    if (this->Owner)
    {
      this->Owner->RemoveObserver(this->ObserverId);
    }
  }

  //---------------------------------------------------------------------------
  vtkTypeUInt32 RegisterDataInformation(
    vtkTypeUInt32 globalid, unsigned int port, vtkPVInformation* info)
  {
    vtkTypeUInt32 correctId = this->GetMappedId(globalid);
    if (correctId != 0)
    {
      // cout << "RegisterDataInformation: " << correctId << " " << port << endl;
      std::string id = this->GenerateID(correctId, port, info);
      this->DataInformationMap[id] = info;
    }
    return correctId;
  }
  //---------------------------------------------------------------------------
  void UnRegisterDataInformation(vtkTypeUInt32 globalid)
  {
    std::string id = this->GenerateID(globalid, 0, NULL);
    std::vector<std::string> keysToDelete;
    std::map<std::string, vtkSmartPointer<vtkPVInformation> >::iterator iter;
    for (iter = this->DataInformationMap.begin(); iter != this->DataInformationMap.end(); iter++)
    {
      if (iter->first.find(id) == 0)
      {
        keysToDelete.push_back(iter->first);
      }
    }

    // Cleanup the map
    std::vector<std::string>::iterator deleteIter;
    for (deleteIter = keysToDelete.begin(); deleteIter != keysToDelete.end(); deleteIter++)
    {
      this->DataInformationMap.erase(*deleteIter);
    }
  }
  //---------------------------------------------------------------------------
  bool GatherInformation(vtkTypeUInt32 globalid, unsigned int port, vtkPVInformation* info)
  {
    // cout << "GatherInformation: " << globalid << " " << port << endl;
    std::string id = this->GenerateID(globalid, port, info);
    vtkPVInformation* storedValue = this->DataInformationMap[id];

    if (storedValue)
    {
      vtkClientServerStream stream;
      storedValue->CopyToStream(&stream);
      info->CopyFromStream(&stream);
    }
    return true;
  }
  //---------------------------------------------------------------------------
  std::string GenerateID(vtkTypeUInt32 globalid, unsigned int port, vtkPVInformation* info)
  {
    std::ostringstream id;
    id << globalid << ":";
    if (info)
    {
      id << port << ":" << info->GetClassName();
    }
    return id.str();
  }
  //---------------------------------------------------------------------------
  void UpdateToCatalyseSourceProxy(vtkObject*, unsigned long, void* data)
  {
    vtkObject* obj = reinterpret_cast<vtkObject*>(data);
    vtkSISourceProxy* siSourceProxy = vtkSISourceProxy::SafeDownCast(obj);
    if (siSourceProxy)
    {
      siSourceProxy->SetDisablePipelineExecution(true);
    }
  }
  //---------------------------------------------------------------------------
  void ResetIdMap() { this->IdMap.clear(); }
  //---------------------------------------------------------------------------
  void UpdateIdMap(vtkTypeUInt32* data, int size)
  {
    for (int i = 0; i < size; i += 2)
    {
      this->IdMap[data[i]] = data[i + 1];
    }
    cout << endl;
  }
  //---------------------------------------------------------------------------
  vtkTypeUInt32 GetMappedId(vtkTypeUInt32 originalId)
  {
    std::map<vtkTypeUInt32, vtkTypeUInt32>::iterator iter;
    iter = this->IdMap.find(originalId);
    if (iter != this->IdMap.end())
    {
      return iter->second;
    }
    return 0;
  }

private:
  vtkWeakPointer<vtkPVCatalystSessionCore> Owner;
  unsigned long ObserverId;
  std::map<std::string, vtkSmartPointer<vtkPVInformation> > DataInformationMap;
  std::map<vtkTypeUInt32, vtkTypeUInt32> IdMap;
};
//****************************************************************************/
vtkStandardNewMacro(vtkPVCatalystSessionCore);
//----------------------------------------------------------------------------
vtkPVCatalystSessionCore::vtkPVCatalystSessionCore()
{
  this->CatalystInternal = new vtkInternal(this);
}

//----------------------------------------------------------------------------
vtkPVCatalystSessionCore::~vtkPVCatalystSessionCore()
{
  delete this->CatalystInternal;
}

//----------------------------------------------------------------------------
void vtkPVCatalystSessionCore::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkPVCatalystSessionCore::GatherInformation(
  vtkTypeUInt32 location, vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  if (globalid == 0)
  {
    information->CopyFromObject(NULL);
    return true;
  }

  if (vtkPVDataInformation* dataInfo = vtkPVDataInformation::SafeDownCast(information))
  {
    return this->CatalystInternal->GatherInformation(
      globalid, dataInfo->GetPortNumber(), information);
  }
  return this->Superclass::GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
vtkTypeUInt32 vtkPVCatalystSessionCore::RegisterDataInformation(
  vtkTypeUInt32 globalid, unsigned int port, vtkPVInformation* info)
{
  return this->CatalystInternal->RegisterDataInformation(globalid, port, info);
}

//----------------------------------------------------------------------------
void vtkPVCatalystSessionCore::UpdateIdMap(vtkTypeUInt32* idMapArray, int size)
{
  this->CatalystInternal->UpdateIdMap(idMapArray, size);
}

//----------------------------------------------------------------------------
void vtkPVCatalystSessionCore::ResetIdMap()
{
  this->CatalystInternal->ResetIdMap();
}
