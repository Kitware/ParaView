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
#include "vtkSMUncheckedPropertyHelper.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkSMUncheckedPropertyHelper::vtkSMUncheckedPropertyHelper(
  vtkSMProxy* proxy, const char* name, bool quiet /* = false*/)
  : vtkSMPropertyHelper(proxy, name, quiet)
{
  this->setUseUnchecked(true);
}

//----------------------------------------------------------------------------
vtkSMUncheckedPropertyHelper::vtkSMUncheckedPropertyHelper(
  vtkSMProperty* property, bool quiet /* = false*/)
  : vtkSMPropertyHelper(property, quiet)
{
  this->setUseUnchecked(true);
}
