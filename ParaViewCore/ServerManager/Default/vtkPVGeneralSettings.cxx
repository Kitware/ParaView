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
  vtkObjectFactory::ConstructInstance(instance->GetClassName());
  return instance;
}

//----------------------------------------------------------------------------
vtkPVGeneralSettings::vtkPVGeneralSettings()
  : AutoApply(false),
  AutoApplyActiveOnly(false),
  DefaultViewType(NULL)
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
void vtkPVGeneralSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
