/*=========================================================================

  Program:   ParaView
  Module:    vtkPMUniformGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMUniformGridVolumeRepresentationProxy - representation that can be used to
// show a uniform grid volume in a render view.
// .SECTION Description
// vtkPMUniformGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the uniform grid volume in a vtkSMRenderViewProxy.
// It supports rendering the uniform grid volume data.

#ifndef __vtkPMUniformGridVolumeRepresentationProxy_h
#define __vtkPMUniformGridVolumeRepresentationProxy_h

#include "vtkPMProxy.h"

class VTK_EXPORT vtkPMUniformGridVolumeRepresentationProxy : public vtkPMProxy
{
public:
  static vtkPMUniformGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkPMUniformGridVolumeRepresentationProxy,
    vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMUniformGridVolumeRepresentationProxy();
  ~vtkPMUniformGridVolumeRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage*);

private:
  vtkPMUniformGridVolumeRepresentationProxy(const vtkPMUniformGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkPMUniformGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif
