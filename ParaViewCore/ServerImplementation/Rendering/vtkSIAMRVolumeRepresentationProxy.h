/*=========================================================================

  Program:   ParaView
  Module:    vtkSIAMRVolumeRepresentationProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSIAMRVolumeRepresentationProxy - representation that can be used to
// show an Overlapping AMR Volume in a render view.
// .SECTION Description
// vtkSIAMRVolumeRepresentationProxy is a concrete representation that can be used
// to render an overlapping AMR volume in a vtkSMRenderViewProxy.

#ifndef __vtkSIAMRVolumeRepresentationProxy_h
#define __vtkSIAMRVolumeRepresentationProxy_h

#include "vtkSIProxy.h"

class VTK_EXPORT vtkSIAMRVolumeRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSIAMRVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSIAMRVolumeRepresentationProxy,
    vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSIAMRVolumeRepresentationProxy();
  ~vtkSIAMRVolumeRepresentationProxy();

  // Description:
  virtual bool CreateVTKObjects(vtkSMMessage*);

private:
  vtkSIAMRVolumeRepresentationProxy(const vtkSIAMRVolumeRepresentationProxy&); // Not implemented
  void operator=(const vtkSIAMRVolumeRepresentationProxy&); // Not implemented
//ETX
};

#endif
