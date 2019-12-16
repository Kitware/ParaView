/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyInitializationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyInitializationHelper.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkSMProxyInitializationHelper::vtkSMProxyInitializationHelper()
{
}

//----------------------------------------------------------------------------
vtkSMProxyInitializationHelper::~vtkSMProxyInitializationHelper()
{
}

//----------------------------------------------------------------------------
void vtkSMProxyInitializationHelper::RegisterProxy(vtkSMProxy*, vtkPVXMLElement*)
{
}

//----------------------------------------------------------------------------
void vtkSMProxyInitializationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
