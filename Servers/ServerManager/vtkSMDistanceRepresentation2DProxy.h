/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDistanceRepresentation2DProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDistanceRepresentation2DProxy
// .SECTION Description
//

#ifndef __vtkSMDistanceRepresentation2DProxy_h
#define __vtkSMDistanceRepresentation2DProxy_h

#include "vtkSMWidgetRepresentationProxy.h"

class VTK_EXPORT vtkSMDistanceRepresentation2DProxy : public vtkSMWidgetRepresentationProxy
{
public:
  static vtkSMDistanceRepresentation2DProxy* New();
  vtkTypeMacro(vtkSMDistanceRepresentation2DProxy, vtkSMWidgetRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMDistanceRepresentation2DProxy();
  ~vtkSMDistanceRepresentation2DProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMDistanceRepresentation2DProxy(const vtkSMDistanceRepresentation2DProxy&); // Not implemented
  void operator=(const vtkSMDistanceRepresentation2DProxy&); // Not implemented
//ETX
};

#endif

