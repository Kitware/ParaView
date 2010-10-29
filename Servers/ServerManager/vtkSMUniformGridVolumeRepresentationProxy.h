/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUniformGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUniformGridVolumeRepresentationProxy - representation that can be used to
// show a uniform grid volume in a render view.
// .SECTION Description
// vtkSMUniformGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the uniform grid volume in a vtkSMRenderViewProxy.
// It supports rendering the uniform grid volume data.

#ifndef __vtkSMUniformGridVolumeRepresentationProxy_h
#define __vtkSMUniformGridVolumeRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class VTK_EXPORT vtkSMUniformGridVolumeRepresentationProxy :
  public vtkSMRepresentationProxy
{
public:
  static vtkSMUniformGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSMUniformGridVolumeRepresentationProxy,
    vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMUniformGridVolumeRepresentationProxy();
  ~vtkSMUniformGridVolumeRepresentationProxy();

  // Description:
  virtual void CreateVTKObjects();

private:
  vtkSMUniformGridVolumeRepresentationProxy(const vtkSMUniformGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSMUniformGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif

