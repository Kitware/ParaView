/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLookupTableProxy.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMLookupTableProxy -
// .SECTION Description

#ifndef __vtkSMLookupTableProxy_h
#define __vtkSMLookupTableProxy_h

#include "vtkSMProxy.h"

class VTK_EXPORT vtkSMLookupTableProxy : public vtkSMProxy
{
public:
  static vtkSMLookupTableProxy* New();
  vtkTypeRevisionMacro(vtkSMLookupTableProxy, vtkSMProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  void Build();

protected:
  vtkSMLookupTableProxy();
  ~vtkSMLookupTableProxy();

private:
  vtkSMLookupTableProxy(const vtkSMLookupTableProxy&); // Not implemented
  void operator=(const vtkSMLookupTableProxy&); // Not implemented
};

#endif
