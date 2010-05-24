/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMRenderView2Proxy
// .SECTION Description
//

#ifndef __vtkSMRenderView2Proxy_h
#define __vtkSMRenderView2Proxy_h

#include "vtkSMViewProxy.h"

class VTK_EXPORT vtkSMRenderView2Proxy : public vtkSMViewProxy
{
public:
  static vtkSMRenderView2Proxy* New();
  vtkTypeRevisionMacro(vtkSMRenderView2Proxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMRenderView2Proxy();
  ~vtkSMRenderView2Proxy();

  // Description:
  // Called at the end of CreateVTKObjects().
  virtual void EndCreateVTKObjects();

private:
  vtkSMRenderView2Proxy(const vtkSMRenderView2Proxy&); // Not implemented
  void operator=(const vtkSMRenderView2Proxy&); // Not implemented
//ETX
};

#endif
