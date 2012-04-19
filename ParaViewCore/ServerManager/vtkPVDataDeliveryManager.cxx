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
#include "vtkPVDataDeliveryManager.h"

#include "vtkObjectFactory.h"
#include "vtkPVView.h"

vtkCxxSetObjectMacro(vtkPVDataDeliveryManager, View, vtkPVView);
//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::vtkPVDataDeliveryManager()
{
  this->View = 0;
}

//----------------------------------------------------------------------------
vtkPVDataDeliveryManager::~vtkPVDataDeliveryManager()
{
  this->SetView(NULL);
}

//----------------------------------------------------------------------------
void vtkPVDataDeliveryManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "View: " << this->View << endl;
}
