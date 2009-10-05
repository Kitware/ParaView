/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSGeometryInformation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSGeometryInformation.h"

#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkPVSGeometryInformation);
vtkCxxRevisionMacro(vtkPVSGeometryInformation, "1.1");

//----------------------------------------------------------------------------
vtkPVSGeometryInformation::vtkPVSGeometryInformation()
{
}

//----------------------------------------------------------------------------
vtkPVSGeometryInformation::~vtkPVSGeometryInformation()
{
}

//----------------------------------------------------------------------------
void vtkPVSGeometryInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVSGeometryInformation::CopyFromDataSet(vtkDataSet* data)
{
  this->Superclass::CopyFromDataSet(data);
  // Get the bounds from the reader meta-data if available
  if (data->GetProducerPort() &&
      data->GetProducerPort()->GetProducer() &&
      data->GetProducerPort()->GetProducer()->GetExecutive())
    {
    vtkStreamingDemandDrivenPipeline *sddp =
        vtkStreamingDemandDrivenPipeline::SafeDownCast(
          data->GetProducerPort()->GetProducer()->GetExecutive());
    if (sddp)
      {
      sddp->GetWholeBoundingBox(0, this->Bounds);
      }
    }

  //TODO: Use FIELD_RANGES and use them for data attribute ranges instead
  //of current piece's data range
  //Of course, source has to fill them in too.
}

