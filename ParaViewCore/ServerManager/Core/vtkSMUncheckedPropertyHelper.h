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

#ifndef vtkSMUncheckedPropertyHelper_h
#define vtkSMUncheckedPropertyHelper_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkSMPropertyHelper.h"

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMUncheckedPropertyHelper : public vtkSMPropertyHelper
{
public:
  vtkSMUncheckedPropertyHelper(vtkSMProxy* proxy, const char* name, bool quiet = false);
  vtkSMUncheckedPropertyHelper(vtkSMProperty* property, bool quiet = false);

private:
  vtkSMUncheckedPropertyHelper(const vtkSMUncheckedPropertyHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSMUncheckedPropertyHelper&) VTK_DELETE_FUNCTION;
};

#endif

// VTK-HeaderTest-Exclude: vtkSMUncheckedPropertyHelper.h
