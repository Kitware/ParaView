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
#include "vtkPrismRepresentation.h"

#include "vtkCompositePolyDataMapper2.h"
#include "vtkInformation.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVRenderView.h"
#include "vtkQuadricClustering.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredDataDeliveryFilter.h"

vtkStandardNewMacro(vtkPrismRepresentation);

//----------------------------------------------------------------------------
vtkPrismRepresentation::vtkPrismRepresentation()
{
  this->PrismRange[0] = 0;
  this->PrismRange[1] = 0;
  this->PrismRange[2] = 0;

  this->ScaleFactor[0] = 1;
  this->ScaleFactor[1] = 1;
  this->ScaleFactor[2] = 1;
}

//----------------------------------------------------------------------------
vtkPrismRepresentation::~vtkPrismRepresentation()
{

}

//----------------------------------------------------------------------------
bool vtkPrismRepresentation::AddToView(vtkView* view)
{
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkPrismRepresentation::RemoveFromView(vtkView* view)
{
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
int vtkPrismRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  return this->Superclass::RequestData(request, inputVector, outputVector);
}



//----------------------------------------------------------------------------
void vtkPrismRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
