/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPrismCubeAxesRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMPrismCubeAxesRepresentationProxy - representation proxy for CubeAxes.
// .SECTION Description
// vtkSMPrismCubeAxesRepresentationProxy can be used to show a bounding cube axes to
// any dataset.

#ifndef __vtkSMPrismCubeAxesRepresentationProxy_h
#define __vtkSMPrismCubeAxesRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class VTK_EXPORT vtkSMPrismCubeAxesRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMPrismCubeAxesRepresentationProxy* New();
  vtkTypeMacro(vtkSMPrismCubeAxesRepresentationProxy, vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMPrismCubeAxesRepresentationProxy();
  ~vtkSMPrismCubeAxesRepresentationProxy();

  virtual void RepresentationUpdated();

private:
  vtkSMPrismCubeAxesRepresentationProxy(const vtkSMPrismCubeAxesRepresentationProxy&); // Not implemented
  void operator=(const vtkSMPrismCubeAxesRepresentationProxy&); // Not implemented
//ETX
};

#endif

