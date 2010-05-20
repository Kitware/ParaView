/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUpdateSuppressorProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMUpdateSuppressorProxy - proxy for an update suppressor
// .SECTION Description
// This proxy class is an example of how vtkSMProxy can be subclassed
// to add functionality. 

#ifndef __vtkSMUpdateSuppressorProxy_h
#define __vtkSMUpdateSuppressorProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMUpdateSuppressorProxy : public vtkSMSourceProxy
{
public:
  static vtkSMUpdateSuppressorProxy* New();
  vtkTypeMacro(vtkSMUpdateSuppressorProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //// Description:
  //// Calls Update() on all sources. It also creates output ports if
  //// they are not already created.
  //virtual void UpdatePipeline();

  //// Description:
  //// Calls Update() on all sources with the given time request. 
  //// It also creates output ports if they are not already created.
  //virtual void UpdatePipeline(double time);

  // Description:
  // Results in calling ForceUpdate() if the pipeline is dirty. 
  void ForceUpdate();

protected:
  vtkSMUpdateSuppressorProxy();
  ~vtkSMUpdateSuppressorProxy();

private:
  vtkSMUpdateSuppressorProxy(const vtkSMUpdateSuppressorProxy&); // Not implemented
  void operator=(const vtkSMUpdateSuppressorProxy&); // Not implemented
};

#endif
