/*=========================================================================

  Program:   ParaView
  Module:    vtkSIUnstructuredGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIUnstructuredGridVolumeRepresentationProxy - representation that can be used to
// show a unstructured grid volume in a render view.
// .SECTION Description
// vtkSIUnstructuredGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the unstructured grid volume in a vtkSIRenderViewProxy.

#ifndef __vtkSIUnstructuredGridVolumeRepresentationProxy_h
#define __vtkSIUnstructuredGridVolumeRepresentationProxy_h

#include "vtkSIProxy.h"

class VTK_EXPORT vtkSIUnstructuredGridVolumeRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSIUnstructuredGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSIUnstructuredGridVolumeRepresentationProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIUnstructuredGridVolumeRepresentationProxy();
  ~vtkSIUnstructuredGridVolumeRepresentationProxy();

  // Description:
  // Register the mappers.
  virtual void OnCreateVTKObjects();


private:
  vtkSIUnstructuredGridVolumeRepresentationProxy(const vtkSIUnstructuredGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSIUnstructuredGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif
