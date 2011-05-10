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

#include "vtkAlgorithmOutput.h"
#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointSet.h"
#include "vtkPrismView.h"
#include "vtkPVGeometryFilter.h"

vtkStandardNewMacro(vtkPrismRepresentation);

//----------------------------------------------------------------------------
vtkPrismRepresentation::vtkPrismRepresentation()
{

}

//----------------------------------------------------------------------------
vtkPrismRepresentation::~vtkPrismRepresentation()
{

}

//----------------------------------------------------------------------------
bool vtkPrismRepresentation::GenerateMetaData(vtkInformation *, vtkInformation* outInfo)
{
  //generate the bounds of this data object
  if (this->GeometryFilter->GetNumberOfInputConnections(0) > 0)
    {
    vtkDataObject* geom = this->GeometryFilter->GetOutputDataObject(0);
    if (geom)
      {
      vtkPointSet *ps = vtkPointSet::SafeDownCast(geom);
      if ( ps && ps->GetNumberOfPoints() > 0 )
        {
        //for now lets send bounds without talking to other processes
        double bounds[6];
        ps->GetBounds(bounds);
        outInfo->Set(vtkPrismView::PRISM_GEOMETRY_BOUNDS(), bounds, 6);
        }
      }    
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPrismRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
