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
// .NAME vtkSMQuadViewProxy
// .SECTION Description
//

#ifndef __vtkSMQuadViewProxy_h
#define __vtkSMQuadViewProxy_h

#include "vtkSMRenderViewProxy.h"

class vtkSMQuadViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMQuadViewProxy* New();
  vtkTypeMacro(vtkSMQuadViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMQuadViewProxy();
  ~vtkSMQuadViewProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMQuadViewProxy(const vtkSMQuadViewProxy&); // Not implemented
  void operator=(const vtkSMQuadViewProxy&); // Not implemented
//ETX
};

#endif
