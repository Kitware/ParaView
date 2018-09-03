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
#include "vtkPVXYChartView.h"
#include "vtkProcessModuleAutoMPI.h"
#include "vtkSISourceProxy.h"
#include "vtkSMArraySelectionDomain.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMParaViewPipelineControllerWithRendering.h"
#include "vtkSMTrace.h"
#include "vtkSMTransferFunctionManager.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"

#include <cassert>

vtkSmartPointer<vtkPVGeneralSettings> vtkPVGeneralSettings::Instance;

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
  : BlockColorsDistinctValues(7)
  , AutoApply(false)
  , AutoApplyActiveOnly(false)
  , DefaultViewType(NULL)
  , TransferFunctionResetMode(vtkSMTransferFunctionManager::GROW_ON_APPLY)
  , ScalarBarMode(vtkPVGeneralSettings::AUTOMATICALLY_HIDE_SCALAR_BARS)
  , CacheGeometryForAnimation(false)
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
  , AnimationTimeNotation('g')
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
  vtkCacheSizeKeeper::GetInstance()->SetCacheLimit(val ? this->AnimationGeometryCacheLimit : 0);
  if (this->CacheGeometryForAnimation != val)
  {
    this->CacheGeometryForAnimation = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetAnimationGeometryCacheLimit(unsigned long val)
{
  vtkCacheSizeKeeper::GetInstance()->SetCacheLimit(this->CacheGeometryForAnimation ? val : 0);
  if (this->AnimationGeometryCacheLimit != val)
  {
    this->AnimationGeometryCacheLimit = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetIgnoreNegativeLogAxisWarning(bool val)
{
  if (vtkPVXYChartView::GetIgnoreNegativeLogAxisWarning() != val)
  {
    vtkPVXYChartView::SetIgnoreNegativeLogAxisWarning(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetIgnoreNegativeLogAxisWarning()
{
  return vtkPVXYChartView::GetIgnoreNegativeLogAxisWarning();
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetScalarBarMode(int val)
{
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

  if (val != this->ScalarBarMode)
  {
    this->ScalarBarMode = val;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetInheritRepresentationProperties(bool val)
{
  if (val != vtkSMParaViewPipelineControllerWithRendering::GetInheritRepresentationProperties())
  {
    vtkSMParaViewPipelineControllerWithRendering::SetInheritRepresentationProperties(val);
    this->Modified();
  }
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
  if (val != vtkSMChartSeriesSelectionDomain::GetLoadNoChartVariables())
  {
    vtkSMChartSeriesSelectionDomain::SetLoadNoChartVariables(val);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
bool vtkPVGeneralSettings::GetLoadNoChartVariables()
{
  return vtkSMChartSeriesSelectionDomain::GetLoadNoChartVariables();
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

//----------------------------------------------------------------------------
void vtkPVGeneralSettings::SetAnimationTimeNotation(int notation)
{
  switch (notation)
  {
    case vtkPVGeneralSettings::SCIENTIFIC:
      this->SetAnimationTimeNotation('e');
      break;
    case vtkPVGeneralSettings::FIXED:
      this->SetAnimationTimeNotation('f');
      break;
    case vtkPVGeneralSettings::MIXED:
    default:
      this->SetAnimationTimeNotation('g');
  }
}
