// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMXYChartRepresentationInitializationHelper.h"

#include "vtkObjectFactory.h"
#include "vtkSMChartSeriesSelectionDomain.h"
#include "vtkSMDomain.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewProxy.h"

#include <cassert>

vtkStandardNewMacro(vtkSMXYChartRepresentationInitializationHelper);
//----------------------------------------------------------------------------
vtkSMXYChartRepresentationInitializationHelper::vtkSMXYChartRepresentationInitializationHelper() =
  default;

//----------------------------------------------------------------------------
vtkSMXYChartRepresentationInitializationHelper::~vtkSMXYChartRepresentationInitializationHelper() =
  default;

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationInitializationHelper::PostInitializeProxy(
  vtkSMProxy* proxy, vtkPVXMLElement*, vtkMTimeType vtkNotUsed(ts))
{
  assert(proxy != nullptr);

  vtkSMSessionProxyManager* pxm = proxy->GetSessionProxyManager();
  vtkSMViewProxy* activeView = nullptr;
  if (vtkSMProxySelectionModel* viewSM = pxm->GetSelectionModel("ActiveView"))
  {
    activeView = vtkSMViewProxy::SafeDownCast(viewSM->GetCurrentProxy());
  }

  vtkSMPropertyHelper input(proxy, "Input");
  if (activeView &&
    (!strcmp(activeView->GetXMLName(), "XYBarChartView") ||
      !strcmp(activeView->GetXMLName(), "XYHistogramChartView")) &&
    input.GetAsProxy())
  {
    if (vtkSMRepresentationProxy* repr = vtkSMRepresentationProxy::SafeDownCast(proxy))
    {
      if (!strcmp(repr->GetXMLName(), "XYChartRepresentation") &&
        repr->GetProperty("SeriesPlotCorner"))
      {
        vtkSMProperty* corner = repr->GetProperty("SeriesPlotCorner");
        auto cornerDomain = corner->FindDomain<vtkSMChartSeriesSelectionDomain>();
        cornerDomain->SetDefaultValue("2");
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMXYChartRepresentationInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
