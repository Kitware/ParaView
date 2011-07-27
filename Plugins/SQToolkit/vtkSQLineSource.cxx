/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQLineSource.h"

#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <math.h>
vtkStandardNewMacro(vtkSQLineSource);

//-----------------------------------------------------------------------------
vtkSQLineSource::vtkSQLineSource(int res)
{
  this->Point1[0] = -0.5;
  this->Point1[1] =  0.0;
  this->Point1[2] =  0.0;

  this->Point2[0] =  0.5;
  this->Point2[1] =  0.0;
  this->Point2[2] =  0.0;

  this->Resolution = (res < 1 ? 1 : res);

  this->SetNumberOfInputPorts(0);
}

//-----------------------------------------------------------------------------
int vtkSQLineSource::RequestInformation(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{

  // let the pipeline know we can handle our own domain decomp
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(),-1);

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQLineSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output
    = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  if ( (pieceNo>=nPieces) || (pieceNo>=this->Resolution) )
    {
    output->Initialize();
    return 1;
    }

  // domain decomposition
  // note: resolution is the total number of cells (line segments)
  int nLocal=1;
  int startId=0;
  int endId=1;

  if (this->Resolution<=nPieces)
    {
    startId=pieceNo;
    endId=pieceNo+1;
    }
  else
    {
    int pieceSize=this->Resolution/nPieces;
    int nLarge=this->Resolution%nPieces;
    nLocal=pieceSize+(pieceNo<nLarge?1:0);
    startId=pieceSize*pieceNo+(pieceNo<nLarge?pieceNo:nLarge);
    endId=startId+nLocal;
    }

  // line equation paramteric in point id
  int t1=this->Resolution;
  float r0[3]={this->Point1[0], this->Point1[1], this->Point1[2]};
  float r1[3]={this->Point2[0], this->Point2[1], this->Point2[2]};
  float v[3]={(r1[0]-r0[0])/t1, (r1[1]-r0[1])/t1, (r1[2]-r0[2])/t1};

  int nPtsLocal=nLocal+1;

  vtkIdTypeArray *ca=vtkIdTypeArray::New();
  ca->SetNumberOfTuples(3*nLocal);
  vtkIdType *pca=ca->GetPointer(0);

  vtkFloatArray *pa=vtkFloatArray::New();
  pa->SetNumberOfComponents(3);
  pa->SetNumberOfTuples(nPtsLocal);
  float *ppa=pa->GetPointer(0);

  ppa[0]=r0[0]+startId*v[0];
  ppa[1]=r0[1]+startId*v[1];
  ppa[2]=r0[2]+startId*v[2];
  ppa+=3;

  for (int i=0,id=startId; id<endId; ++i,++id)
    {
    int segEnd=id+1;
    ppa[0]=r0[0]+segEnd*v[0];
    ppa[1]=r0[1]+segEnd*v[1];
    ppa[2]=r0[2]+segEnd*v[2];
    ppa+=3;

    pca[0]=2;
    pca[1]=i;
    pca[2]=i+1;
    pca+=3;
    }

  vtkCellArray *cells=vtkCellArray::New();
  cells->SetCells(nLocal,ca);
  ca->Delete();
  output->SetLines(cells);
  cells->Delete();

  vtkPoints *points=vtkPoints::New();
  points->SetData(pa);
  pa->Delete();
  output->SetPoints(points);
  points->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQLineSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", "
                               << this->Point1[1] << ", "
                               << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", "
                               << this->Point2[1] << ", "
                               << this->Point2[2] << ")\n";
}
