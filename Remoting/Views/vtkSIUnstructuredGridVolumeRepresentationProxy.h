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
/**
 * @class   vtkSIUnstructuredGridVolumeRepresentationProxy
 * @brief   representation that can be used to
 * show a unstructured grid volume in a render view.
 *
 * vtkSIUnstructuredGridVolumeRepresentationProxy is a concrete representation that can be used
 * to render the unstructured grid volume in a vtkSIRenderViewProxy.
*/

#ifndef vtkSIUnstructuredGridVolumeRepresentationProxy_h
#define vtkSIUnstructuredGridVolumeRepresentationProxy_h

#include "vtkRemotingViewsModule.h" //needed for exports
#include "vtkSIProxy.h"

class VTKREMOTINGVIEWS_EXPORT vtkSIUnstructuredGridVolumeRepresentationProxy : public vtkSIProxy
{
public:
  static vtkSIUnstructuredGridVolumeRepresentationProxy* New();
  vtkTypeMacro(vtkSIUnstructuredGridVolumeRepresentationProxy, vtkSIProxy);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkSIUnstructuredGridVolumeRepresentationProxy();
  ~vtkSIUnstructuredGridVolumeRepresentationProxy() override;

  /**
   * Register the mappers
   */
  bool CreateVTKObjects() override;

private:
  vtkSIUnstructuredGridVolumeRepresentationProxy(
    const vtkSIUnstructuredGridVolumeRepresentationProxy&) = delete;
  void operator=(const vtkSIUnstructuredGridVolumeRepresentationProxy&) = delete;
};

#endif
