/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQSurfaceVectors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQSurfaceVectors.h"


#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkDataSet.h"
#include "vtkPointData.h"
#include "vtkDoubleArray.h"
#include "vtkIdList.h"
#include "vtkCellType.h"

#include <vector>
#include <sstream>


//*****************************************************************************
template<typename T>
void AccumulateNormal(T p1[3], T p2[3], T p3[3], T n[3])
{
  T v1[3]={
      p2[0]-p1[0],
      p2[1]-p1[1],
      p2[2]-p1[2]
      };

  T v2[3]={
      p3[0]-p1[0],
      p3[1]-p1[1],
      p3[2]-p1[2]
      };

  T v3[3]={
    v1[1]*v2[2]-v2[1]*v1[2],
    v2[0]*v1[2]-v1[0]*v2[2],
    v1[0]*v2[1]-v2[0]*v1[1]
    };

  n[0]+=v3[0];
  n[1]+=v3[1];
  n[2]+=v3[2];
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQSurfaceVectors);

//-----------------------------------------------------------------------------
vtkSQSurfaceVectors::vtkSQSurfaceVectors()
{
}

//-----------------------------------------------------------------------------
vtkSQSurfaceVectors::~vtkSQSurfaceVectors()
{
}

// //-----------------------------------------------------------------------------
// int vtkSQSurfaceVectors::RequestUpdateExtent(
//       vtkInformation * vtkNotUsed(request),
//       vtkInformationVector **inputVector,
//       vtkInformationVector *outputVector)
// {
//   // get the info objects
//   vtkInformation* outInfo = outputVector->GetInformationObject(0);
//   vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
//
//   inInfo->Set(
//     vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
//     outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS())+1);
//
//   return 1;
// }

//-----------------------------------------------------------------------------
int vtkSQSurfaceVectors::RequestData(
      vtkInformation *vtkNotUsed(request),
      vtkInformationVector **inputVector,
      vtkInformationVector *outputVector)
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkDataSet *input
    = dynamic_cast<vtkDataSet*>(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  // sanity -- empty input
  if (input==0)
    {
    vtkErrorMacro("Empty input.");
    return 1;
    }

  vtkDataSet *output
    = dynamic_cast<vtkDataSet*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // sanity -- empty output
  if (output==0)
    {
    vtkErrorMacro("Empty output.");
    return 1;
    }

  // sanity -- surface has points
  vtkIdType nPoints=input->GetNumberOfPoints();
  if (nPoints==0)
    {
    vtkErrorMacro("No points.");
    return 1;
    }

  // copy geometry and data
  output->ShallowCopy(input);

  // locate all available vectors
  std::vector<vtkDataArray *>vectors;
  int nArrays=input->GetPointData()->GetNumberOfArrays();
  for (int i=0; i<nArrays; ++i)
    {
    vtkDataArray *array=input->GetPointData()->GetArray(i);
    if (array->GetNumberOfComponents()==3)
      {
      vectors.push_back(array);
      }
    }

  double prog=0.0;
  double progInc=1.0/((double)nPoints);
  double progRep=0.1;

  vtkIdList* cIds = vtkIdList::New();
  vtkIdList* ptIds = vtkIdList::New();

  // for each vector on the input project onto the surface
  size_t nVectors=vectors.size();
  for (size_t i=0; i<nVectors; ++i)
    {
    // progress
    prog+=progInc;
    if (prog>=progRep)
      {
      this->UpdateProgress(prog);
      progRep+=0.1;
      }

    std::ostringstream os;

    vtkDataArray *proj=vectors[i]->NewInstance();
    proj->SetNumberOfComponents(3);
    proj->SetNumberOfTuples(nPoints);
    os << vectors[i]->GetName() << "_par";
    proj->SetName(os.str().c_str());
    output->GetPointData()->AddArray(proj);
    proj->Delete();

    vtkDataArray *perp=vectors[i]->NewInstance();
    perp->SetNumberOfComponents(3);
    perp->SetNumberOfTuples(nPoints);
    os.str("");
    os << vectors[i]->GetName() << "_perp";
    perp->SetName(os.str().c_str());
    output->GetPointData()->AddArray(perp);
    perp->Delete();

    for (vtkIdType pid=0; pid<nPoints; ++pid)
      {
      input->GetPointCells(pid,cIds);
      vtkIdType nCIds=cIds->GetNumberOfIds();

      // Compute the point normal.
      double n[3]={0.0};
      for (vtkIdType j=0; j<nCIds; ++j)
        {
        vtkIdType cid=cIds->GetId(j);
        switch (input->GetCellType(cid))
          {
          case VTK_PIXEL:
          case VTK_POLYGON:
          case VTK_TRIANGLE:
          case VTK_QUAD:
            {
            input->GetCellPoints(cid,ptIds);

            double p1[3],p2[3],p3[3];
            input->GetPoint(ptIds->GetId(0),p1);
            input->GetPoint(ptIds->GetId(1),p2);
            input->GetPoint(ptIds->GetId(2),p3);

            AccumulateNormal<double>(p1,p2,p3,n);
            }
            break;

          default:
            vtkErrorMacro(
              << "Unsuported cell type "
              << input->GetCellType(cid)
              << ".");
            return 1;
            break;
          }
        }
      //n[0]/=nCids;
      //n[1]/=nCids;
      //n[2]/=nCids;

      double m=sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
      n[0]/=m;
      n[1]/=m;
      n[2]/=m;

      double v[3];
      vectors[i]->GetTuple(pid,v);

      // v.n
      double v_perp=n[0]*v[0]+n[1]*v[1]+n[2]*v[2];

      // perpendicular component
      v[0]=n[0]*v_perp;
      v[1]=n[1]*v_perp;
      v[2]=n[2]*v_perp;
      perp->InsertTuple(pid,v);

      // parallel component
      vectors[i]->GetTuple(pid,v);
      v[0]=v[0]-(n[0]*v_perp);
      v[1]=v[1]-(n[1]*v_perp);
      v[2]=v[2]-(n[2]*v_perp);
      proj->InsertTuple(pid,v);
      }
    }
  cIds->Delete();
  ptIds->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQSurfaceVectors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
