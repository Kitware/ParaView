/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMChartRepresentationProxy - representation proxy used by most of
// the charts.
// .SECTION Description
// vtkSMChartRepresentationProxy is representation proxy used by bar chart,
// xy-line plot etc.

#ifndef __vtkSMChartRepresentationProxy_h
#define __vtkSMChartRepresentationProxy_h

#include "vtkSMClientDeliveryRepresentationProxy.h"

class VTK_EXPORT vtkSMChartRepresentationProxy :
  public vtkSMClientDeliveryRepresentationProxy
{
public:
  static vtkSMChartRepresentationProxy* New();
  vtkTypeRevisionMacro(vtkSMChartRepresentationProxy,
    vtkSMClientDeliveryRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called to update the Display. Default implementation does nothing.
  // Argument is the view requesting the update. Can be null in the
  // case when something other than a view is requesting the update.
  virtual void Update() { this->Superclass::Update(); }
  virtual void Update(vtkSMViewProxy* view);

//BTX
protected:
  vtkSMChartRepresentationProxy();
  ~vtkSMChartRepresentationProxy();

  // Description:
  // This method is called after CreateVTKObjects(). 
  // This gives subclasses an opportunity to do some post-creation
  // initialization.
  virtual bool EndCreateVTKObjects();

private:
  vtkSMChartRepresentationProxy(const vtkSMChartRepresentationProxy&); // Not implemented
  void operator=(const vtkSMChartRepresentationProxy&); // Not implemented
//ETX
};

#endif

