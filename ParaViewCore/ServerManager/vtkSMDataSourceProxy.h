/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDataSourceProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMDataSourceProxy - "data-centric" proxy for VTK source on a server
// .SECTION Description
// vtkSMDataSourceProxy adds a CopyData method to the vtkSMSourceProxy API
// to give a "data-centric" behaviour; the output data of the input
// vtkSMSourceProxy (to CopyData) is copied by the VTK object managed
// by the vtkSMDataSourceProxy.
// .SECTION See Also
// vtkSMSourceProxy

#ifndef __vtkSMDataSourceProxy_h
#define __vtkSMDataSourceProxy_h

#include "vtkSMSourceProxy.h"

class VTK_EXPORT vtkSMDataSourceProxy : public vtkSMSourceProxy
{
public:
  static vtkSMDataSourceProxy* New();
  vtkTypeMacro(vtkSMDataSourceProxy, vtkSMSourceProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copies data from source proxy object to object represented by this
  // source proxy object.
  void CopyData(vtkSMSourceProxy *sourceProxy);

//BTX
protected:
  vtkSMDataSourceProxy();
  ~vtkSMDataSourceProxy();

private:
  vtkSMDataSourceProxy(const vtkSMDataSourceProxy&); // Not implemented
  void operator=(const vtkSMDataSourceProxy&); // Not implemented
//ETX
};

#endif

