/*=========================================================================

  Program:   ParaView
  Module:    vtkSMParaViewPipelineControllerWithRendering.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMParaViewPipelineControllerWithRendering
// .SECTION Description
// vtkSMParaViewPipelineControllerWithRendering overrides
// vtkSMParaViewPipelineController to add support for initializing rendering
// proxies appropriately. vtkSMParaViewPipelineControllerWithRendering uses
// vtkObjectFactory mechanisms to override vtkSMParaViewPipelineController's
// creation. One should not need to create or use this class directly (excepting
// when needing to subclass). Simply create vtkSMParaViewPipelineController. If
// the application is linked with the rendering module, then this class will be
// instantiated instead of vtkSMParaViewPipelineController automatically.

#ifndef __vtkSMParaViewPipelineControllerWithRendering_h
#define __vtkSMParaViewPipelineControllerWithRendering_h

#include "vtkPVServerManagerRenderingModule.h" //needed for exports
#include "vtkSMParaViewPipelineController.h"

class VTKPVSERVERMANAGERRENDERING_EXPORT vtkSMParaViewPipelineControllerWithRendering :
  public vtkSMParaViewPipelineController
{
public:
  static vtkSMParaViewPipelineControllerWithRendering* New();
  vtkTypeMacro(vtkSMParaViewPipelineControllerWithRendering, vtkSMParaViewPipelineController);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overridden to create color and opacity transfer functions if applicable.
  // While it is tempting to add any default property setup logic in such
  // overrides, we must keep such overrides to a minimal and opting for domains
  // that set appropriate defaults where as much as possible.
  virtual bool RegisterRepresentationProxy(vtkSMProxy* proxy);
//BTX
protected:
  vtkSMParaViewPipelineControllerWithRendering();
  ~vtkSMParaViewPipelineControllerWithRendering();

private:
  vtkSMParaViewPipelineControllerWithRendering(const vtkSMParaViewPipelineControllerWithRendering&); // Not implemented
  void operator=(const vtkSMParaViewPipelineControllerWithRendering&); // Not implemented
//ETX
};

#endif
