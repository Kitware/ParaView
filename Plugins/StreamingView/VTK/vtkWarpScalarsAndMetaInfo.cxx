/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpScalarsAndMetaInfo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpScalarsAndMetaInfo.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkBoundingBox.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkWarpScalarsAndMetaInfo);

//----------------------------------------------------------------------------
vtkWarpScalarsAndMetaInfo::vtkWarpScalarsAndMetaInfo()
{
  this->GetInformation()->Set(vtkAlgorithm::MANAGES_METAINFORMATION(), 1);
}

//----------------------------------------------------------------------------
vtkWarpScalarsAndMetaInfo::~vtkWarpScalarsAndMetaInfo()
{
}

//----------------------------------------------------------------------------
void vtkWarpScalarsAndMetaInfo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
int vtkWarpScalarsAndMetaInfo::ProcessRequest(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if(!request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_MANAGE_INFORMATION()))
    {
    return this->Superclass::ProcessRequest(request, inputVector, outputVector);
    }

  // copy attributes across unmodified
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  if (inInfo->Has(vtkDataObject::CELL_DATA_VECTOR()))
    {
    outInfo->CopyEntry(inInfo, vtkDataObject::CELL_DATA_VECTOR(), 1);
    }
  if (inInfo->Has(vtkDataObject::POINT_DATA_VECTOR()))
    {
    outInfo->CopyEntry(inInfo, vtkDataObject::POINT_DATA_VECTOR(), 1);
    }

  if (!this->XYPlane && !this->UseNormal)
    {
    //can't predict what point normals are going to do to the bounds,
    //so don't try
    static double emptyBoundingBox[6] = {0,-1,0,-1,0,-1};
    outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(),
                 emptyBoundingBox, 6);
    return 1;
    }

  vtkSmartPointer<vtkPoints> inPts = vtkSmartPointer<vtkPoints>::New();
  vtkDataArray *inScalars;
  int i;
  vtkIdType ptId, numPts;
  double x[3], *n, s, newX[3];

  double *pbounds = inInfo->Get(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX());
  //convert to 8 corner Points
  for (int a=0; a<2; a++)
    {
    for (int b=0; b<2; b++)
      {
      for (int c=0; c<2; c++)
        {
        inPts->InsertNextPoint(pbounds[a], pbounds[2+b], pbounds[4+c]);
        }
      }
    }

  // get the range of the input
  inScalars = this->GetInputArrayToProcess(0,inputVector);
  if ( !inPts || !inScalars )
    {
    vtkDebugMacro(<<"No data to warp");
    return 1;
    }
  vtkInformation *fInfo = NULL;
  vtkInformationVector *miv = inInfo->Get(vtkDataObject::POINT_DATA_VECTOR());
  for (int index = 0; index < miv->GetNumberOfInformationObjects(); index++)
    {
    vtkInformation *mInfo = miv->GetInformationObject(index);
    const char *minfo_arrayname =
      mInfo->Get(vtkDataObject::FIELD_ARRAY_NAME());
    if (minfo_arrayname && !strcmp(minfo_arrayname, inScalars->GetName()))
      {
      fInfo = mInfo;
      break;
      }
    }
  if (!fInfo)
    {
    return 1;
    }

  double *range = fInfo->Get(vtkDataObject::PIECE_FIELD_RANGE());
  if ( this->XYPlane )
    {
    this->PointNormal = &vtkWarpScalarsAndMetaInfo::ZNormal;
    }
  else
    {
    this->PointNormal = &vtkWarpScalarsAndMetaInfo::InstanceNormal;
    }

  numPts = 8;
  vtkBoundingBox bbox;
  // Loop over 8 corners, adjusting locations by min attribute
  for (ptId=0; ptId < numPts; ptId++)
    {
    inPts->GetPoint(ptId, x);
    n = (this->*(this->PointNormal))(ptId,NULL);
    if ( this->XYPlane )
      {
      s = x[2];
      }
    else
      {
      s = range[0]; //min
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
      }
    bbox.AddPoint(newX);
    }
  // Loop over 8 corners, adjusting locations by max attribute
  for (ptId=0; ptId < numPts; ptId++)
    {
    inPts->GetPoint(ptId, x);
    n = (this->*(this->PointNormal))(ptId,NULL);
    if ( this->XYPlane )
      {
      s = x[2];
      }
    else
      {
      s = range[1]; //max
      }
    for (i=0; i<3; i++)
      {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
      }
    bbox.AddPoint(newX);
    }

  double obounds[6];
  bbox.GetBounds(obounds);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), obounds, 6);
  return 1;
}
