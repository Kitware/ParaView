/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeneralSettings.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGeneralSettings.h"

#include "vtkCacheSizeKeeper.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkSISourceProxy.h"
#include "vtkSMInputArrayDomain.h"

#include <cassert>

vtkSmartPointer<vtkPVGeneralSettings> vtkPVGeneralSettings::Instance;

vtkInstantiatorNewMacro(vtkPVGeneralSettings);
//----------------------------------------------------------------------------
vtkPVGeneralSettings* vtkPVGeneralSettings::New()
{
  vtkPVGeneralSettings* instance = vtkPVGeneralSettings::GetInstance();
  assert(instance);
  instance->Register(NULL);
  return instance;
}

//----------------------------------------------------------------------------
vtkPVGeneralSettings::vtkPVGeneralSettings()
  : AutoApply(false),
  AutoApplyActiveOnly(false),
  DefaultViewType(NULL),
  TransferFunctionResetMode(vtkPVGeneralSettings::GROW_ON_APPLY),
  ScalarBarMode(vtkPVGeneralSettings::AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS),
  CacheGeometryForAnimation(false),
  AnimationGeometryCacheLimit(0)
{
  this->SetDefaultViewType("RenderView");
}

//----------------------------------------------------------------------------
vtkPVGeneralSettings::~vtkPVGeneralSettings()
{
  this->SetDefaultViewType(NULL);
}

//----------------------------------------------------------------------------
vtkPVGeneralSettings* vtkPVGeneralSettings::GetInstance()
{
  if (!vtkPVGeneralSettings::Instance)
    {
    vtkPVGeneralSettings* instance = new vtkPVGeneralSettings();
    vtkObjectFactory::ConstructInstance(instance->GetClassName());
    vtkPVGeneralSettings::Instance.TakeReference(instance);
    }
  return vtkPVGeneralSettings::Instance;
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetAutoConvertProperties(bool val)
{
  if (this->GetAutoConvertProperties() != val)
    {
    vtkSMInputArrayDomain::SetAutomaticPropertyConversion(val);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetAutoConvertProperties()
{
  return vtkSMInputArrayDomain::GetAutomaticPropertyConversion();
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetStrictLoadBalancing(bool val)
{
  if (this->GetStrictLoadBalancing() != val)
    {
    vtkSISourceProxy::SetDisableExtentsTranslator(val);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetStrictLoadBalancing()
{
  return vtkSISourceProxy::GetDisableExtentsTranslator();
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetEnableAutoMPI(bool val)
{
  if (this->GetEnableAutoMPI() != val)
    {
    vtkProcessModuleAutoMPI::SetEnableAutoMPI(val);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetEnableAutoMPI()
{
  return vtkProcessModuleAutoMPI::EnableAutoMPI;
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetAutoMPILimit(int val)
{
  if (this->GetAutoMPILimit() != val && val >= 1)
    {
    vtkProcessModuleAutoMPI::SetNumberOfCores(val);
    }
}

//----------------------------------------------------------------------------
int vtkPVGeneralSettings::GetAutoMPILimit()
{
  return vtkProcessModuleAutoMPI::NumberOfCores;
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetCacheGeometryForAnimation(bool val)
{
  vtkCacheSizeKeeper::GetInstance()->SetCacheLimit(
    val? this->AnimationGeometryCacheLimit : 0);
  if (this->CacheGeometryForAnimation != val)
    {
    this->CacheGeometryForAnimation = val;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetAnimationGeometryCacheLimit(unsigned long val)
{
  vtkCacheSizeKeeper::GetInstance()->SetCacheLimit(
    this->CacheGeometryForAnimation? val : 0);
  if (this->AnimationGeometryCacheLimit != val)
    {
    this->AnimationGeometryCacheLimit = val;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
