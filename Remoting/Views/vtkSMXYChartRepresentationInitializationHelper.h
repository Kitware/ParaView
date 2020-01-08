/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXYChartRepresentationInitializationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
