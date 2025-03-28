// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVGeneralSettings.h"

#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSMPTools.h"
#include "vtkThreadedCallbackQueue.h"

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters
#include "vtkmFilterOverrides.h"
#endif

#include <cassert>

vtkSmartPointer<vtkPVGeneralSettings> vtkPVGeneralSettings::Instance;

//----------------------------------------------------------------------------
vtkPVGeneralSettings* vtkPVGeneralSettings::New()
{
  vtkPVGeneralSettings* instance = vtkPVGeneralSettings::GetInstance();
  assert(instance);
  instance->Register(nullptr);
  return instance;
}

//----------------------------------------------------------------------------
vtkPVGeneralSettings* vtkPVGeneralSettings::GetInstance()
{
  if (!vtkPVGeneralSettings::Instance)
  {
    vtkPVGeneralSettings* instance = new vtkPVGeneralSettings();
    instance->InitializeObjectBase();
    vtkPVGeneralSettings::Instance.TakeReference(instance);
  }
  return vtkPVGeneralSettings::Instance;
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetUseAcceleratedFilters(bool val)
{
  static_cast<void>(val);

#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters
  if (this->GetUseAcceleratedFilters() != val)
  {
    vtkmFilterOverrides::SetEnabled(val);
    this->Modified();
  }
#endif
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetUseAcceleratedFilters()
{
#if VTK_MODULE_ENABLE_VTK_AcceleratorsVTKmFilters
  return vtkmFilterOverrides::GetEnabled();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
int vtkPVGeneralSettings::GetNumberOfCallbackThreads()
{
  return vtkProcessModule::GetProcessModule()->GetCallbackQueue()->GetNumberOfThreads();
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetNumberOfCallbackThreads(int n)
{
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
  auto session = vtkPVSession::SafeDownCast(processModule->GetSession());

  // We change the number of threads in built-in mode or if current process is the root server node
  if (session->GetProcessRoles() & vtkPVSession::DATA_SERVER ||
    (session->GetProcessRoles() & vtkPVSession::CLIENT &&
      !session->GetController(vtkPVSession::DATA_SERVER)))
  {
    processModule->GetCallbackQueue()->SetNumberOfThreads(n);
  }
}

//----------------------------------------------------------------------------
int vtkPVGeneralSettings::GetNumberOfSMPThreads()
{
  return vtkSMPTools::GetEstimatedNumberOfThreads();
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetNumberOfSMPThreads(int n)
{
  if (n > 0)
  {
    vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();
    auto session = vtkPVSession::SafeDownCast(processModule->GetSession());
    if (session->GetProcessRoles() & vtkPVSession::DATA_SERVER)
    {
      vtkSMPTools::Initialize(n);
    }
  }
  else
  {
    vtkSMPTools::Initialize(0);
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AnimationGeometryCacheLimit: " << this->AnimationGeometryCacheLimit << "\n";
  os << indent << "AutoApply: " << this->AutoApply << "\n";
  os << indent << "AutoApplyActiveOnly: " << this->AutoApplyActiveOnly << "\n";
  os << indent << "AutoApplyDelay: " << this->AutoApplyDelay << "\n";
  os << indent << "CacheGeometryForAnimation: " << this->CacheGeometryForAnimation << "\n";
  os << indent << "DefaultViewType: " << this->DefaultViewType << "\n";
  os << indent << "InterfaceLanguage: " << this->InterfaceLanguage << "\n";
  os << indent << "LockPanels: " << this->LockPanels << "\n";
  os << indent << "PreservePropertyValues: " << this->PreservePropertyValues << "\n";
  os << indent << "PropertiesPanelMode: " << this->PropertiesPanelMode << "\n";
  os << indent << "ScalarBarMode: " << this->ScalarBarMode << "\n";
}
