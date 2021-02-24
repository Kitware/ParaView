/*=========================================================================

  Program:   ParaView
  Module:    vtkCPDataDescription.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCPDataDescription.h"

#include "vtkCPInputDataDescription.h"
#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObject.h"
#include "vtkDataSet.h"
#include "vtkFieldData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedIntArray.h"

#include <map>
#include <string>

class vtkCPDataDescription::vtkInternals
{
public:
  typedef std::map<std::string, vtkSmartPointer<vtkCPInputDataDescription> > GridDescriptionMapType;
  GridDescriptionMapType GridDescriptionMap;
};

vtkCxxSetObjectMacro(vtkCPDataDescription, UserData, vtkFieldData);
vtkStandardNewMacro(vtkCPDataDescription);
//----------------------------------------------------------------------------
vtkCPDataDescription::vtkCPDataDescription()
{
  this->Time = 0;
  this->TimeStep = 0;
  this->IsTimeDataSet = false;
  this->ForceOutput = false;
  this->UserData = nullptr;

  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkCPDataDescription::~vtkCPDataDescription()
{
  this->SetUserData(nullptr);
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void vtkCPDataDescription::SetTimeData(double time, vtkIdType timeStep)
{
  this->Time = time;
  this->TimeStep = timeStep;
  this->IsTimeDataSet = true;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkCPDataDescription::AddInput(const char* gridName)
{
  if (this->Internals->GridDescriptionMap.find(gridName) ==
    this->Internals->GridDescriptionMap.end())
  {
    this->Internals->GridDescriptionMap[gridName] =
      vtkSmartPointer<vtkCPInputDataDescription>::New();
  }
}

//----------------------------------------------------------------------------
unsigned int vtkCPDataDescription::GetNumberOfInputDescriptions()
{
  return static_cast<unsigned int>(this->Internals->GridDescriptionMap.size());
}

//----------------------------------------------------------------------------
void vtkCPDataDescription::ResetInputDescriptions()
{
  vtkInternals::GridDescriptionMapType::iterator iter;
  for (iter = this->Internals->GridDescriptionMap.begin();
       iter != this->Internals->GridDescriptionMap.end(); ++iter)
  {
    iter->second->Reset();
  }
}

//----------------------------------------------------------------------------
void vtkCPDataDescription::ResetAll()
{
  this->ResetInputDescriptions();
  this->ForceOutput = false;
  this->IsTimeDataSet = false;
}

//----------------------------------------------------------------------------
vtkCPInputDataDescription* vtkCPDataDescription::GetInputDescription(unsigned int index)
{
  unsigned int cur_index = 0;
  vtkInternals::GridDescriptionMapType::iterator iter;
  for (iter = this->Internals->GridDescriptionMap.begin();
       iter != this->Internals->GridDescriptionMap.end(); ++iter, ++cur_index)
  {
    if (cur_index == index)
    {
      return iter->second;
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
const char* vtkCPDataDescription::GetInputDescriptionName(unsigned int index)
{
  unsigned int cur_index = 0;
  vtkInternals::GridDescriptionMapType::iterator iter;
  for (iter = this->Internals->GridDescriptionMap.begin();
       iter != this->Internals->GridDescriptionMap.end(); ++iter, ++cur_index)
  {
    if (cur_index == index)
    {
      return iter->first.c_str();
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
vtkCPInputDataDescription* vtkCPDataDescription::GetInputDescriptionByName(const char* name)
{
  vtkInternals::GridDescriptionMapType::iterator iter =
    this->Internals->GridDescriptionMap.find(name);
  if (iter != this->Internals->GridDescriptionMap.end())
  {
    return iter->second;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkCPDataDescription::GetIfGridIsNecessary(const char* name)
{
  if (this->ForceOutput)
  {
    return true;
  }
  vtkInternals::GridDescriptionMapType::iterator iter =
    this->Internals->GridDescriptionMap.find(name);
  if (iter != this->Internals->GridDescriptionMap.end())
  {
    return iter->second->GetIfGridIsNecessary();
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkCPDataDescription::GetIfAnyGridNecessary()
{
  if (this->ForceOutput)
  {
    return true;
  }
  vtkInternals::GridDescriptionMapType::iterator iter;
  for (iter = this->Internals->GridDescriptionMap.begin();
       iter != this->Internals->GridDescriptionMap.end(); ++iter)
  {
    if (iter->second->GetIfGridIsNecessary())
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkCPDataDescription::Copy(vtkCPDataDescription* dataDescription)
{
  if (dataDescription == nullptr || this == dataDescription)
  {
    return;
  }
  this->Time = dataDescription->GetTime();
  this->TimeStep = dataDescription->GetTimeStep();
  this->IsTimeDataSet = dataDescription->IsTimeDataSet;
  this->ForceOutput = dataDescription->GetForceOutput();
  this->SetUserData(dataDescription->GetUserData());

  for (auto iter = dataDescription->Internals->GridDescriptionMap.begin();
       iter != dataDescription->Internals->GridDescriptionMap.end(); iter++)
  {
    this->AddInput(iter->first.c_str());
    this->GetInputDescriptionByName(iter->first.c_str())->ShallowCopy(iter->second);
  }
}

//----------------------------------------------------------------------------
void vtkCPDataDescription::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Time: " << this->Time << "\n";
  os << indent << "TimeStep: " << this->TimeStep << "\n";
  os << indent << "IsTimeDataSet: " << this->IsTimeDataSet << "\n";
  os << indent << "ForceOutput: " << this->ForceOutput << "\n";
  if (this->UserData)
  {
    os << indent << "UserData: " << this->UserData << "\n";
  }
  else
  {
    os << indent << "UserData: (NULL)\n";
  }
}
