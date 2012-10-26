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
#include "vtkSMUncertaintySurfaceRepresentationProxy.h"

#include "vtkObjectFactory.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkPiecewiseFunction.h"

vtkStandardNewMacro(vtkSMUncertaintySurfaceRepresentationProxy)

vtkSMUncertaintySurfaceRepresentationProxy::vtkSMUncertaintySurfaceRepresentationProxy()
{
}

vtkSMUncertaintySurfaceRepresentationProxy::~vtkSMUncertaintySurfaceRepresentationProxy()
{
}

void vtkSMUncertaintySurfaceRepresentationProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();

  vtkSMProxy *transferFunctionProxy = this->GetSubProxy("TransferFunction");
  double values[] = { 0, 0, 0.5, 0, 1, 1, 0.5, 0 };
  vtkSMPropertyHelper(transferFunctionProxy, "Points").Set(values, 8);
  vtkSMPropertyHelper(this, "UncertaintyTransferFunction").Set(transferFunctionProxy);
}

void vtkSMUncertaintySurfaceRepresentationProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
