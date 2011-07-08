/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQPointSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSQPointSource.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

#include <float.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

vtkStandardNewMacro(vtkSQPointSource);

//----------------------------------------------------------------------------
vtkSQPointSource::vtkSQPointSource()
{
  this->NumberOfPoints=1;

  this->Center[0]=
  this->Center[1]=
  this->Center[2]=0.0;

  this->Radius=1;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
int vtkSQPointSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output 
    = dynamic_cast<vtkPolyData*>(outInfo->Get(vtkDataObject::DATA_OBJECT()));


  // paralelize by piece information.
  int pieceNo
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int nPieces
    = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  // sanity - the requst cannot be fullfilled
  if ( (pieceNo>=nPieces) || (pieceNo>=this->NumberOfPoints) )
    {
    output->Initialize();
    return 1;
    }

  // domain decomposition
  int nLocal=1;
  if (this->NumberOfPoints>nPieces)
    {
    int pieceSize=this->NumberOfPoints/nPieces;
    int nLarge=this->NumberOfPoints%nPieces;
    nLocal=pieceSize+(pieceNo<nLarge?1:0);
    }

  vtkFloatArray *pa=vtkFloatArray::New();
  pa->SetNumberOfComponents(3);
  pa->SetNumberOfTuples(nLocal);
  float *ppa=pa->GetPointer(0);

  vtkIdTypeArray *ca=vtkIdTypeArray::New();
  ca->SetNumberOfTuples(2*nLocal);
  vtkIdType *pca=ca->GetPointer(0);

  srand(pieceNo+time(0));

  for (int i=0; i<nLocal; ++i)
    {
    double pi=3.14159265358979;
    double rho=this->Radius*((double)rand())/((double)RAND_MAX);
    double theta=2.0*pi*((double)rand())/((double)RAND_MAX);
    double phi=pi*((double)rand())/((double)RAND_MAX);
    double sin_theta=sin(theta);
    double cos_theta=cos(theta);
    double rho_sin_phi=rho*sin(phi);
    ppa[0]=this->Center[0]+rho_sin_phi*cos_theta;
    ppa[1]=this->Center[1]+rho_sin_phi*sin_theta;
    ppa[2]=this->Center[2]+rho*cos(phi);
    ppa+=3;

    pca[0]=1;
    pca[1]=i;
    pca+=2;
    }

  vtkCellArray *cells=vtkCellArray::New();
  cells->SetCells(nLocal,ca);
  ca->Delete();
  output->SetVerts(cells);
  cells->Delete();

  vtkPoints *points=vtkPoints::New();
  points->SetData(pa);
  pa->Delete();
  output->SetPoints(points);
  points->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQPointSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number Of Points: " << this->NumberOfPoints << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
                              << this->Center[1] << ", "
                              << this->Center[2] << ")\n";
}
