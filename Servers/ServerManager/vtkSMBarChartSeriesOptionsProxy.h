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

  void SetColor(const char* name, double r, double g, double b);

//BTX
protected:
  vtkSMBarChartSeriesOptionsProxy();
  ~vtkSMBarChartSeriesOptionsProxy();

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
  vtkSMBarChartSeriesOptionsProxy(const vtkSMBarChartSeriesOptionsProxy&); // Not implemented
  void operator=(const vtkSMBarChartSeriesOptionsProxy&); // Not implemented
//ETX
};

#endif

