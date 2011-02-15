/*=========================================================================

  Program:   ParaView
  Module:    vtkPMUnstructuredGridVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPMUnstructuredGridVolumeRepresentationProxy - representation that can be used to
// show a unstructured grid volume in a render view.
// .SECTION Description
// vtkPMUnstructuredGridVolumeRepresentationProxy is a concrete representation that can be used
// to render the unstructured grid volume in a vtkPMRenderViewProxy.

#ifndef __vtkPMUnstructuredGridVolumeRepresentationProxy_h
#define __vtkPMUnstructuredGridVolumeRepresentationProxy_h

#include "vtkPMProxy.h"

class VTK_EXPORT vtkPMUnstructuredGridVolumeRepresentationProxy : public vtkPMProxy
{
public:
  static vtkPMUnstructuredGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkPMUnstructuredGridVolumeRepresentationProxy, vtkPMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkPMUnstructuredGridVolumeRepresentationProxy();
  ~vtkPMUnstructuredGridVolumeRepresentationProxy();

  // Description:
  // Register the mappers.
  virtual void OnCreateVTKObjects();


private:
  vtkPMUnstructuredGridVolumeRepresentationProxy(const vtkPMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkPMUnstructuredGridVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif
