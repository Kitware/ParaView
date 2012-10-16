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
#include "vtkPVInformation.h"
#include "vtkPVSession.h"
#include "vtkSISourceProxy.h"
#include "vtkSMMessage.h"
#include "vtkSmartPointer.h"

#include <assert.h>
#include <string>
#include <vector>
#include <vtksys/ios/sstream>

//****************************************************************************/
//                        Internal Class
//****************************************************************************/
class vtkPVCatalystSessionCore::vtkInternal
{
public:
  vtkInternal(vtkPVCatalystSessionCore* parent)
  {
    this->Owner = parent;
    this->ObserverId = this->Owner->AddObserver(
          vtkCommand::UpdateDataEvent,
          this,
          &vtkPVCatalystSessionCore::vtkInternal::UpdateToCatalyseSourceProxy);
  }
  //---------------------------------------------------------------------------
  ~vtkInternal()
  {
    if(this->Owner)
      {
      this->Owner->RemoveObserver(this->ObserverId);
      }
  }

  //---------------------------------------------------------------------------
  void RegisterDataInformation(vtkTypeUInt32 globalid, vtkPVInformation *info)
  {
    std::string id = this->GenerateID(globalid, info);
    this->DataInformationMap[id] = info;
  }
  //---------------------------------------------------------------------------
  void UnRegisterDataInformation(vtkTypeUInt32 globalid)
  {
    std::string id = this->GenerateID(globalid, NULL);
    std::vector<std::string> keysToDelete;
    std::map<std::string, vtkSmartPointer<vtkPVInformation> >::iterator iter;
    for( iter  = this->DataInformationMap.begin();
         iter != this->DataInformationMap.end();
         iter++)
      {
      if(iter->first.find(id) == 0)
        {
        keysToDelete.push_back(iter->first);
        }
      }

    // Cleanup the map
    std::vector<std::string>::iterator deleteIter;
    for( deleteIter  = keysToDelete.begin();
         deleteIter != keysToDelete.end();
         deleteIter++)
      {
      this->DataInformationMap.erase(*deleteIter);
      }
  }
  //---------------------------------------------------------------------------
  bool GatherInformation(vtkTypeUInt32 globalid, vtkPVInformation *info)
  {
    std::string id = this->GenerateID(globalid, info);
    vtkPVInformation *storedValue = this->DataInformationMap[id];
    // We don't care of storedValue is NULL...
    info->CopyFromObject(storedValue);
    return true;
  }
  //---------------------------------------------------------------------------
  std::string GenerateID(vtkTypeUInt32 globalid, vtkPVInformation *info)
  {
    vtksys_ios::ostringstream id;
    id << globalid << ":";
    if(info)
      {
      id << info->GetClassName();
      }
    return id.str();
  }
  //---------------------------------------------------------------------------
  void UpdateToCatalyseSourceProxy(vtkObject*, unsigned long, void* data)
  {
    vtkObject* obj = reinterpret_cast<vtkObject*>(data);
    vtkSISourceProxy* siSourceProxy = vtkSISourceProxy::SafeDownCast(obj);
    if(siSourceProxy)
      {
      siSourceProxy->SetDisablePipelineExecution(true);
      }
  }

private:
  vtkWeakPointer<vtkPVCatalystSessionCore> Owner;
  unsigned long ObserverId;
  std::map<std::string, vtkSmartPointer<vtkPVInformation> > DataInformationMap;
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
bool vtkPVCatalystSessionCore::GatherInformation( vtkTypeUInt32 location,
                                          vtkPVInformation* information,
                                          vtkTypeUInt32 globalid)
{
  if (globalid == 0)
    {
    information->CopyFromObject(NULL);
    return true;
    }

  if(information->IsA("vtkPVDataInformation"))
    {
    return this->CatalystInternal->GatherInformation(globalid, information);
    }
  return this->Superclass::GatherInformation(location, information, globalid);
}

//----------------------------------------------------------------------------
void vtkPVCatalystSessionCore::RegisterDataInformation(vtkTypeUInt32 globalid, vtkPVInformation* info)
{
  this->CatalystInternal->RegisterDataInformation(globalid, info);
}
