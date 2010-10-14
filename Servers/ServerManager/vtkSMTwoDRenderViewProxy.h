/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTwoDRenderViewProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTwoDRenderViewProxy
// .SECTION Description
//

#ifndef __vtkSMTwoDRenderViewProxy_h
#define __vtkSMTwoDRenderViewProxy_h

#include "vtkSMRenderViewProxy.h"

class VTK_EXPORT vtkSMTwoDRenderViewProxy : public vtkSMRenderViewProxy
{
public:
  static vtkSMTwoDRenderViewProxy* New();
  vtkTypeMacro(vtkSMTwoDRenderViewProxy, vtkSMRenderViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(
    vtkSMProxy*, int);
//BTX
protected:
  vtkSMTwoDRenderViewProxy();
  ~vtkSMTwoDRenderViewProxy();

private:
  vtkSMTwoDRenderViewProxy(const vtkSMTwoDRenderViewProxy&); // Not implemented
  void operator=(const vtkSMTwoDRenderViewProxy&); // Not implemented
//ETX
};

#endif

