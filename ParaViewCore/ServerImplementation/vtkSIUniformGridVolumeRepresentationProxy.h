/*=========================================================================

  Program:   ParaView
  Module:    vtkSIUniformGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIUniformGridVolumeRepresentationProxy - representation that can be used to
// show a uniform grid volume in a render view.
// .SECTION Description
// vtkSIUniformGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the uniform grid volume in a vtkSMRenderViewProxy.
// It supports rendering the uniform grid volume data.

#ifndef __vtkSIUniformGridVolumeRepresentationProxy_h
#define __vtkSIUniformGridVolumeRepresentationProxy_h

#include "vtkSIProxy.h"

class VTK_EXPORT vtkSIUniformGridVolumeRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSIUniformGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSIUniformGridVolumeRepresentationProxy,
    vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIUniformGridVolumeRepresentationProxy();
  ~vtkSIUniformGridVolumeRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage*);

private:
  vtkSIUniformGridVolumeRepresentationProxy(const vtkSIUniformGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSIUniformGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif
