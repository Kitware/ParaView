/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUnstructuredGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUnstructuredGridVolumeRepresentationProxy - representation that can be used to
// show a unstructured grid volume in a render view.
// .SECTION Description
// vtkSMUnstructuredGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the unstructured grid volume in a vtkSMRenderViewProxy.

#ifndef __vtkSMUnstructuredGridVolumeRepresentationProxy_h
#define __vtkSMUnstructuredGridVolumeRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class VTK_EXPORT vtkSMUnstructuredGridVolumeRepresentationProxy :
  public vtkSMRepresentationProxy
{
public:
  static vtkSMUnstructuredGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSMUnstructuredGridVolumeRepresentationProxy,
    vtkSMRepresentationProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMUnstructuredGridVolumeRepresentationProxy();
  ~vtkSMUnstructuredGridVolumeRepresentationProxy();

  // Description:
  // Register the mappers.
  virtual void CreateVTKObjects();

private:
  vtkSMUnstructuredGridVolumeRepresentationProxy(const vtkSMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif

