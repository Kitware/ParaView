// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPVGeneralSettings.h"

#include "vtkAlgorithm.h"
#include "vtkLegacy.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkProcessModule.h"
#include "vtkSISourceProxy.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMPTools.h"
#include "vtkSMTrace.h"
#include "vtkThreadedCallbackQueue.h"
#include "vtkThreads.h"

#if VTK_MODULE_ENABLE_ParaView_RemotingAnimation
#include "vtkSMAnimationScene.h"
#endif

#if VTK_MODULE_ENABLE_ParaView_RemotingViews
#include "vtkPVView.h"
#include "vtkPVXYChartView.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMTransferFunctionManager.h"
#endif

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
void vtkPVGeneralSettings::SetCacheGeometryForAnimation(bool val)
{
#if VTK_MODULE_ENABLE_ParaView_RemotingAnimation
  if (vtkSMAnimationScene::GetGlobalUseGeometryCache() != val)
  {
    vtkSMAnimationScene::SetGlobalUseGeometryCache(val);
    this->Modified();
  }
#endif
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetCacheGeometryForAnimation()
{
#if VTK_MODULE_ENABLE_ParaView_RemotingAnimation
  return vtkSMAnimationScene::GetGlobalUseGeometryCache();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetAnimationGeometryCacheLimit(unsigned long val)
{
  if (this->AnimationGeometryCacheLimit != val)
  {
    this->AnimationGeometryCacheLimit = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetIgnoreNegativeLogAxisWarning(bool val)
{
  (void)val;
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  if (vtkPVXYChartView::GetIgnoreNegativeLogAxisWarning() != val)
  {
    vtkPVXYChartView::SetIgnoreNegativeLogAxisWarning(val);
    this->Modified();
  }
#endif
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetIgnoreNegativeLogAxisWarning()
{
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  return vtkPVXYChartView::GetIgnoreNegativeLogAxisWarning();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetScalarBarMode(int val)
{
  (void)val;
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  switch (val)
  {
    case AUTOMATICALLY_HIDE_SCALAR_BARS:
      vtkSMParaViewPipelineControllerWithRendering::SetHideScalarBarOnHide(true);
      break;

    case AUTOMATICALLY_SHOW_AND_HIDE_SCALAR_BARS:
      vtkSMParaViewPipelineControllerWithRendering::SetHideScalarBarOnHide(true);
      break;

    default:
      vtkSMParaViewPipelineControllerWithRendering::SetHideScalarBarOnHide(false);
  }
#endif

  if (val != this->ScalarBarMode)
  {
    this->ScalarBarMode = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetInheritRepresentationProperties(bool val)
{
  (void)val;
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  if (val != vtkSMParaViewPipelineControllerWithRendering::GetInheritRepresentationProperties())
  {
    vtkSMParaViewPipelineControllerWithRendering::SetInheritRepresentationProperties(val);
    this->Modified();
  }
#endif
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetLoadAllVariables(bool val)
{
  if (val != vtkSMArraySelectionDomain::GetLoadAllVariables())
  {
    vtkSMArraySelectionDomain::SetLoadAllVariables(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetLoadAllVariables()
{
  return vtkSMArraySelectionDomain::GetLoadAllVariables();
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetLoadNoChartVariables(bool val)
{
  (void)val;
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  if (val != vtkSMChartSeriesSelectionDomain::GetLoadNoChartVariables())
  {
    vtkSMChartSeriesSelectionDomain::SetLoadNoChartVariables(val);
    this->Modified();
  }
#endif
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetLoadNoChartVariables()
{
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  return vtkSMChartSeriesSelectionDomain::GetLoadNoChartVariables();
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetEnableStreaming(bool val)
{
  if (this->GetEnableStreaming() != val)
  {
    this->EnableStreaming = val;
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
    vtkPVView::SetEnableStreaming(val);
#endif
    this->Modified();
  }
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
