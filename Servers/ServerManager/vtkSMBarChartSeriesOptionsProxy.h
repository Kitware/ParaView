/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBarChartSeriesOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBarChartSeriesOptionsProxy - series options proxy for
// vtkSMBarChartViewProxy.
// .SECTION Description
// vtkSMBarChartSeriesOptionsProxy is a proxy for managing the series options
// for vtkSMBarChartViewProxy.

#ifndef __vtkSMBarChartSeriesOptionsProxy_h
#define __vtkSMBarChartSeriesOptionsProxy_h

#include "vtkSMChartNamedOptionsModelProxy.h"

class VTK_EXPORT vtkSMBarChartSeriesOptionsProxy : public vtkSMChartNamedOptionsModelProxy
{
public:
  static vtkSMBarChartSeriesOptionsProxy* New();
  vtkTypeRevisionMacro(vtkSMBarChartSeriesOptionsProxy, vtkSMChartNamedOptionsModelProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMBarChartSeriesOptionsProxy();
  ~vtkSMBarChartSeriesOptionsProxy();

  // Description:
  // Subclasses must override this method to create the right type of options
  // for the type of layer/view/representation they are to be used for.
  virtual vtkQtChartSeriesOptions* NewOptions();

private:
  vtkSMBarChartSeriesOptionsProxy(const vtkSMBarChartSeriesOptionsProxy&); // Not implemented
  void operator=(const vtkSMBarChartSeriesOptionsProxy&); // Not implemented
//ETX
};

#endif

