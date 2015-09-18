/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    int pieceSize=((int)this->NumberOfPoints)/nPieces;
    int nLarge=((int)this->NumberOfPoints)%nPieces;
    nLocal=pieceSize+(pieceNo<nLarge?1:0);
    }

  vtkFloatArray *pa=vtkFloatArray::New();
  pa->SetNumberOfComponents(3);
  pa->SetNumberOfTuples(nLocal);
  float *ppa=pa->GetPointer(0);

  vtkIdTypeArray *ca=vtkIdTypeArray::New();
  ca->SetNumberOfTuples(2*nLocal);
  vtkIdType *pca=ca->GetPointer(0);

  srand((unsigned int)pieceNo+(unsigned int)time(0));

  for (int i=0; i<nLocal; ++i)
    {
    float pi=3.14159265358979f;
    float rho=((float)this->Radius)*((float)rand())/((float)RAND_MAX);
    float theta=2.0f*pi*((float)rand())/((float)RAND_MAX);
    float phi=pi*((float)rand())/((float)RAND_MAX);
    float sin_theta=(float)sin(theta);
    float cos_theta=(float)cos(theta);
    float rho_sin_phi=rho*(float)sin(phi);
    ppa[0]=((float)this->Center[0])+rho_sin_phi*cos_theta;
    ppa[1]=((float)this->Center[1])+rho_sin_phi*sin_theta;
    ppa[2]=((float)this->Center[2])+rho*(float)cos(phi);
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
