/*=========================================================================

  Program:   ParaView
  Module:    vtkSMUncheckedPropertyHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkSMUncheckedPropertyHelper_h
#define __vtkSMUncheckedPropertyHelper_h

#include "vtkSMPropertyHelper.h"

class VTK_EXPORT vtkSMUncheckedPropertyHelper : public vtkSMPropertyHelper
{
public:
  vtkSMUncheckedPropertyHelper(vtkSMProxy *proxy, const char *name, bool quiet = false)
    : vtkSMPropertyHelper(proxy, name, quiet)
  {
    setUseUnchecked(true);
  }

  vtkSMUncheckedPropertyHelper(vtkSMProperty *property, bool quiet = false)
    : vtkSMPropertyHelper(property, quiet)
  {
    setUseUnchecked(true);
  }

private:
  vtkSMUncheckedPropertyHelper(const vtkSMUncheckedPropertyHelper&); // Not implemented
  void operator=(const vtkSMUncheckedPropertyHelper&); // Not implemented
};

#endif
