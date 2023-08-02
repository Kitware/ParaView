// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSMXYChartRepresentationInitializationHelper
 * @brief   initialization helper for XYChartRepresentation proxy.
 *
 * vtkSMXYChartRepresentationInitializationHelper is an initialization helper for
 * the XYChartRepresentation proxy that change default PlotCorner values
 * in case the representation is added to a XYBarCharView.
 */

#ifndef vtkSMXYChartRepresentationInitializationHelper_h
#define vtkSMXYChartRepresentationInitializationHelper_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSMProxyInitializationHelper.h"

class VTKREMOTINGVIEWS_EXPORT vtkSMXYChartRepresentationInitializationHelper
  : public vtkSMProxyInitializationHelper
{
public:
  static vtkSMXYChartRepresentationInitializationHelper* New();
  vtkTypeMacro(vtkSMXYChartRepresentationInitializationHelper, vtkSMProxyInitializationHelper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void PostInitializeProxy(vtkSMProxy*, vtkPVXMLElement*, vtkMTimeType) override;

protected:
  vtkSMXYChartRepresentationInitializationHelper();
  ~vtkSMXYChartRepresentationInitializationHelper() override;

private:
  vtkSMXYChartRepresentationInitializationHelper(
    const vtkSMXYChartRepresentationInitializationHelper&) = delete;
  void operator=(const vtkSMXYChartRepresentationInitializationHelper&) = delete;
};

#endif
