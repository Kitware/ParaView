/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBoxChartRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMBoxChartRepresentationProxy
// .SECTION Description
// Representation proxy for the Box Chart.

#ifndef __vtkSMBoxChartRepresentationProxy_h
#define __vtkSMBoxChartRepresentationProxy_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMChartRepresentationProxy.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMBoxChartRepresentationProxy :
    public vtkSMChartRepresentationProxy
{
public:
  static vtkSMBoxChartRepresentationProxy* New();
  vtkTypeMacro(vtkSMBoxChartRepresentationProxy,
    vtkSMChartRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMBoxChartRepresentationProxy();
  ~vtkSMBoxChartRepresentationProxy();

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);

private:
  vtkSMBoxChartRepresentationProxy(
      const vtkSMBoxChartRepresentationProxy&); // Not implemented
  void operator=(const vtkSMBoxChartRepresentationProxy&); // Not implemented
//ETX
};

#endif
