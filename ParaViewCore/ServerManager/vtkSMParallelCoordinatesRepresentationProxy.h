/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParallelCoordinatesRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMParallelCoordinatesRepresentationProxy
// .SECTION Description
//

#ifndef __vtkSMParallelCoordinatesRepresentationProxy_h
#define __vtkSMParallelCoordinatesRepresentationProxy_h

#include "vtkSMChartRepresentationProxy.h"

class VTK_EXPORT vtkSMParallelCoordinatesRepresentationProxy :
    public vtkSMChartRepresentationProxy
{
public:
  static vtkSMParallelCoordinatesRepresentationProxy* New();
  vtkTypeMacro(vtkSMParallelCoordinatesRepresentationProxy,
    vtkSMChartRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMParallelCoordinatesRepresentationProxy();
  ~vtkSMParallelCoordinatesRepresentationProxy();

  // Description:
  // Called to update the property information on the property. It is assured
  // that the property passed in as an argument is a self property. Both the
  // overloads of UpdatePropertyInformation() call this method, so subclass can
  // override this method to perform special tasks.
  virtual void UpdatePropertyInformationInternal(vtkSMProperty*);

private:
  vtkSMParallelCoordinatesRepresentationProxy(
      const vtkSMParallelCoordinatesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMParallelCoordinatesRepresentationProxy&); // Not implemented
//ETX
};

#endif

