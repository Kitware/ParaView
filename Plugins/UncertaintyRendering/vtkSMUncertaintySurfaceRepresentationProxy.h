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
#ifndef _vtkSMUncertaintySurfaceRepresentationProxy_h
#define _vtkSMUncertaintySurfaceRepresentationProxy_h

#include "vtkSMRepresentationProxy.h"

class vtkSMUncertaintySurfaceRepresentationProxy : public vtkSMRepresentationProxy
{
public:
  static vtkSMUncertaintySurfaceRepresentationProxy* New();
  vtkTypeMacro(vtkSMUncertaintySurfaceRepresentationProxy, vtkSMRepresentationProxy)
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSMUncertaintySurfaceRepresentationProxy();
  ~vtkSMUncertaintySurfaceRepresentationProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMUncertaintySurfaceRepresentationProxy(const vtkSMUncertaintySurfaceRepresentationProxy&);
  void operator=(const vtkSMUncertaintySurfaceRepresentationProxy&);
};

#endif // _vtkSMUncertaintySurfaceRepresentationProxy_h
