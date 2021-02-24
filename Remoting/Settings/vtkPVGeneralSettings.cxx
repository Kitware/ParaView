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
#include "vtkPVOptions.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkSISourceProxy.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMTrace.h"

#if VTK_MODULE_ENABLE_ParaView_RemotingAnimation
#include "vtkSMAnimationScene.h"
#endif

#if VTK_MODULE_ENABLE_ParaView_RemotingViews
#include "vtkPVXYChartView.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMTransferFunctionManager.h"
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
vtkPVGeneralSettings::vtkPVGeneralSettings()
  : BlockColorsDistinctValues(7)
  , AutoApply(false)
  , AutoApplyActiveOnly(false)
  , DefaultViewType(nullptr)
#if VTK_MODULE_ENABLE_ParaView_RemotingViews
  , TransferFunctionResetMode(vtkSMTransferFunctionManager::GROW_ON_APPLY)
#else
  , TransferFunctionResetMode(0)
#endif
  , ScalarBarMode(vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS)
  , AnimationGeometryCacheLimit(0)
  , AnimationTimePrecision(6)
  , ShowAnimationShortcuts(0)
  , RealNumberDisplayedNotation(vtkPVGeneralSettings::DISPLAY_REALNUMBERS_USING_FIXED_NOTATION)
  , RealNumberDisplayedPrecision(6)
  , ResetDisplayEmptyViews(0)
  , PropertiesPanelMode(vtkPVGeneralSettings::ALL_IN_ONE)
  , LockPanels(false)
  , GUIFontSize(0)
  , GUIOverrideFont(false)
  , ColorByBlockColorsOnApply(true)
  , AnimationTimeNotation(vtkPVGeneralSettings::MIXED)
  , EnableStreaming(false)
{
  this->SetDefaultViewType("RenderView");
}

//----------------------------------------------------------------------------
vtkPVGeneralSettings::~vtkPVGeneralSettings()
{
  this->SetDefaultViewType(nullptr);
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
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    if (!pm)
    {
      vtkErrorMacro("vtkProcessModule not initialized. Igoring streaming change.");
      return;
    }
    auto options = pm->GetOptions();
    options->SetEnableStreaming(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "AutoApply: " << this->AutoApply << "\n";
  os << indent << "AutoApplyActiveOnly: " << this->AutoApplyActiveOnly << "\n";
  os << indent << "DefaultViewType: " << this->DefaultViewType << "\n";
  os << indent << "TransferFunctionResetMode: " << this->TransferFunctionResetMode << "\n";
  os << indent << "ScalarBarMode: " << this->ScalarBarMode << "\n";
  os << indent << "CacheGeometryForAnimation: " << this->CacheGeometryForAnimation << "\n";
  os << indent << "AnimationGeometryCacheLimit: " << this->AnimationGeometryCacheLimit << "\n";
  os << indent << "PropertiesPanelMode: " << this->PropertiesPanelMode << "\n";
  os << indent << "LockPanels: " << this->LockPanels << "\n";
}
