/*=========================================================================

  Program:   ParaView
  Module:    vtkSMBlockDeliveryStrategy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMBlockDeliveryStrategy.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMBlockDeliveryStrategy);
//----------------------------------------------------------------------------
vtkSMBlockDeliveryStrategy::vtkSMBlockDeliveryStrategy()
{
}

//----------------------------------------------------------------------------
vtkSMBlockDeliveryStrategy::~vtkSMBlockDeliveryStrategy()
{
}

//----------------------------------------------------------------------------
void vtkSMBlockDeliveryStrategy::BeginCreateVTKObjects()
{
  this->Superclass::BeginCreateVTKObjects();

  this->UpdateSuppressor->SetServers(vtkProcessModule::DATA_SERVER);
  if (this->UpdateSuppressorLOD)
    {
    this->UpdateSuppressorLOD->SetServers(vtkProcessModule::DATA_SERVER);
    }
}


//----------------------------------------------------------------------------
void vtkSMBlockDeliveryStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


