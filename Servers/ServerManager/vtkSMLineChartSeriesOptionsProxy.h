/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLineChartSeriesOptionsProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLineChartSeriesOptionsProxy - series options proxy for
// vtkSMLineChartViewProxy.
// .SECTION Description
// vtkSMLineChartSeriesOptionsProxy is a proxy for managing the series options
// for vtkSMLineChartViewProxy.

#ifndef __vtkSMLineChartSeriesOptionsProxy_h
#define __vtkSMLineChartSeriesOptionsProxy_h

#include "vtkSMChartNamedOptionsModelProxy.h"

class VTK_EXPORT vtkSMLineChartSeriesOptionsProxy : public vtkSMChartNamedOptionsModelProxy
{
public:
  static vtkSMLineChartSeriesOptionsProxy* New();
  vtkTypeRevisionMacro(vtkSMLineChartSeriesOptionsProxy, vtkSMChartNamedOptionsModelProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetColor(const char* name, double r, double g, double b);
  void SetAxisCorner(const char* name, int corner);
  void SetMarkerStyle(const char* name, int style);

//BTX
protected:
  vtkSMLineChartSeriesOptionsProxy();
  ~vtkSMLineChartSeriesOptionsProxy();

  // Description:
  // Subclasses must override this method to create the right type of options
  // for the type of layer/view/representation they are to be used for.
  virtual vtkQtChartSeriesOptions* NewOptions();

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);

private:
  vtkSMLineChartSeriesOptionsProxy(const vtkSMLineChartSeriesOptionsProxy&); // Not implemented
  void operator=(const vtkSMLineChartSeriesOptionsProxy&); // Not implemented
//ETX
};

#endif

