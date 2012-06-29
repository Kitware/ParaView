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
// .NAME vtkSMTimeKeeperProxy
// .SECTION Description
// We simply pass the TimestepValues and TimeRange properties to the client-side
// vtkSMTimeKeeper instance so that it can keep those up-to-date.

#ifndef __vtkSMTimeKeeperProxy_h
#define __vtkSMTimeKeeperProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMTimeKeeperProxy : public vtkSMProxy
{
public:
  static vtkSMTimeKeeperProxy* New();
  vtkTypeMacro(vtkSMTimeKeeperProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
protected:
  vtkSMTimeKeeperProxy();
  ~vtkSMTimeKeeperProxy();

  virtual void CreateVTKObjects();

private:
  vtkSMTimeKeeperProxy(const vtkSMTimeKeeperProxy&); // Not implemented
  void operator=(const vtkSMTimeKeeperProxy&); // Not implemented
//ETX
};

#endif
