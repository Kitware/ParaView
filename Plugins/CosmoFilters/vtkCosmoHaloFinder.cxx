/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoHaloFinder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoHaloFinder.cxx

Copyright (c) 2007, 2009, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007, 2009. Los Alamos National Security, LLC. 
This software was produced under U.S. Government contract DE-AC52-06NA25396 
for Los Alamos National Laboratory (LANL), which is operated by 
Los Alamos National Security, LLC for the U.S. Department of Energy. 
The U.S. Government has rights to use, reproduce, and distribute this software. 
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  
If software is modified to produce derivative works, such modified software 
should be clearly marked, so as not to confuse it with the version available 
from LANL.
 
Additionally, redistribution and use in source and binary forms, with or 
without modification, are permitted provided that the following conditions 
are met:
-   Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer. 
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software 
    without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#include "vtkCosmoHaloFinder.h"

#include "vtkCellType.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkUnstructuredGrid.h"

#include "vtkIntArray.h"
#include "vtkLongArray.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkTimerLog.h"
#include "vtkMath.h"
#include "vtkDirectory.h"
#include "vtkXMLUnstructuredGridWriter.h"

#include "vtksys/ios/fstream"
#include "vtkstd/algorithm"

#define NUM_DATA_DIMS 3
#define DATA_X 0
#define DATA_Y 1
#define DATA_Z 2

vtkCxxRevisionMacro(vtkCosmoHaloFinder, "1.8");
vtkStandardNewMacro(vtkCosmoHaloFinder);

//----------------------------------------------------------------------------
class ValueIdPairLT
{
public:
  bool operator() (const ValueIdPair& p, const ValueIdPair& q) const
  {
    return p.value < q.value;
  }
};

/****************************************************************************/
vtkCosmoHaloFinder::vtkCosmoHaloFinder()
{
}

/****************************************************************************/
vtkCosmoHaloFinder::~vtkCosmoHaloFinder()
{
}

/****************************************************************************/
int vtkCosmoHaloFinder::RequestData(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "rL:       " << this->rL << "\n");
  vtkDebugMacro(<< "bb:       " << this->bb << "\n");
  vtkDebugMacro(<< "pmin:     " << this->pmin << "\n");
  vtkDebugMacro(<< "periodic: " << this->Periodic << "\n");

  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  // copy over the existing fields from input data
  output->ShallowCopy(input);

  this->npart = input->GetNumberOfPoints();
  vtkDebugMacro(<< "npart = " << this->npart);

  vtkDataArray* tag = input->GetPointData()->GetArray("tag");
  if (tag == NULL)
    {
    vtkErrorMacro("The input data set doesn't have tag field!");
    return 0;
    }

  // calculate np from data
  this->np = (int)(pow(this->npart, 1.0 / 3.0) + .5);
  vtkDebugMacro(<< "np = " << this->np);

  // normalize
  float xscal = this->rL / (1.0 * this->np);

  this->data = new float*[NUM_DATA_DIMS];
  for (int i=0; i<NUM_DATA_DIMS; i++)
    {
    this->data[i] = new float[this->npart];
    }

  for (int i=0; i<this->npart; i++)
    {
    double* point = input->GetPoint(i);
    float xx = (float) point[0];
    float yy = (float) point[1];
    float zz = (float) point[2];

    // sanity check
    if(xx > rL || yy > rL || zz > rL)
      {
      vtkErrorMacro(<< "rL is too small");
      for (int j=0; j<NUM_DATA_DIMS; j++)
        {
        delete [] this->data[j];
        }
      return 0;
      }

    this->data[DATA_X][i] = xx / xscal;
    this->data[DATA_Y][i] = yy / xscal;
    this->data[DATA_Z][i] = zz / xscal;
    }

  // reorder
  this->v = new ValueIdPair[this->npart];
  for (int i=0; i<this->npart; i++)
    {
    this->v[i].id = i;
    }

  this->Reorder(0, this->npart, DATA_X);
  delete [] this->v;

  this->seq = new int[this->npart];
  for (int i=0; i<this->npart; i++)
    {
    this->seq[i] = this->v[i].id;
    }

  this->lb = new float*[NUM_DATA_DIMS];
  this->ub = new float*[NUM_DATA_DIMS];
  for (int i=0; i<NUM_DATA_DIMS; i++)
    {
    this->lb[i] = new float[this->npart];
    this->ub[i] = new float[this->npart];
    }

  ComputeLU(0, this->npart);

  // finding halos
  this->ht = new int[this->npart];
  this->halo = new int[this->npart];
  this->nextp = new int[this->npart];
  // to make sure tag matches
  int* pt = new int[this->npart];

  for (int i = 0; i < this->npart; i++)
    {
    this->ht[i] = i;
    this->halo[i] = i;
    this->nextp[i] = -1;
    pt[i] = (int)tag->GetComponent(i, 0);
    }

  this->MyFOF(0, this->npart, DATA_X);

  // compute halos statistics
  int *hsize = new int[this->npart];
  for (int h=0; h<this->npart; h++)
    {
    hsize[h] = 0;
    }

  for (int i=0; i<this->npart; i++)
    {
    hsize[this->ht[i]] += 1;
    }

  int nhalo = 0;
  for (int h=0; h<this->npart; h++)
    {
    if (hsize[h] >= this->pmin)
      {
      nhalo++;
      }
    }

  vtkDebugMacro(<< nhalo << " halos\n");

  int nhalopart = 0;
  for (int i=0; i<this->npart; i++)
    {
    if (hsize[this->ht[i]] >= this->pmin)
      {
      nhalopart++;
      }
    }

  vtkDebugMacro(<< nhalopart << " halo particles\n");

  // put data in output
  vtkIntArray* hID = vtkIntArray::New();
  hID->SetName("hID");
  hID->SetNumberOfComponents(1);
  hID->SetNumberOfTuples(npart);

  vtkIntArray* haloSize = vtkIntArray::New();
  haloSize->SetName("haloSize");
  haloSize->SetNumberOfValues(this->npart);

  for (int i = 0; i < this->npart; i++)
    {
    hID->SetValue(i, 
                  ((hsize[this->ht[i]] < this->pmin) 
                   ? -1 : pt[this->ht[i]]));
    haloSize->SetValue(i, 
                       (hsize[this->ht[i]] < this->pmin) 
                       ? 0 : hsize[this->ht[i]]);
    }

  output->GetPointData()->AddArray(hID);
  output->GetPointData()->AddArray(haloSize);

  // clean up
  delete[] hsize;
  hID->Delete();
  haloSize->Delete();

  delete[] pt;
  delete[] this->ht;
  delete[] this->halo;
  delete[] this->nextp;

  for (int i = 0; i < NUM_DATA_DIMS; i++)
    {
    delete[] this->data[i];
    delete[] this->lb[i];
    delete[] this->ub[i];
    }
  delete[] this->data;
  delete[] this->lb;
  delete[] this->ub;

  delete[] this->seq;
  // delete[] this->v done earlier

  return 1;
}

/****************************************************************************/
void vtkCosmoHaloFinder::Reorder(int first, int last, 
                                 int dataFlag)
{
  int len = (last - first);

  // base case
  if (len <= 1)
    {
    double progress = .5 * (double)last / (double)this->npart;
    if (int(100 * progress) % 1 == 0)
      {
      this->UpdateProgress(progress);
      }

    return;
    }

  // non-base cases
  for (int i = first; i < last; i = i + 1)
    {
    this->v[i].value = this->data[dataFlag][this->v[i].id];
    }

  // divide
  int middle = first + len / 2;
  vtkstd::nth_element(&this->v[first], 
                      &this->v[middle], 
                      &this->v[last], ValueIdPairLT());

  dataFlag = (dataFlag + 1) % 3;

  this->Reorder(first, middle, dataFlag);
  this->Reorder(middle, last, dataFlag);

  return;
}

/****************************************************************************/
void vtkCosmoHaloFinder::ComputeLU(int first, int last)
{
  int len = last - first;

  // book-keeping
  int middle  = first + len / 2;
  int middle1 = first + len / 4;
  int middle2 = first + 3 * len / 4;

  // base case
  if (len == 2)
    {
    int ii = this->seq[first+0];
    int jj = this->seq[first+1];

    this->lb[DATA_X][middle] = 
      vtkstd::min(this->data[DATA_X][ii], this->data[DATA_X][jj]);
    this->lb[DATA_Y][middle] = 
      vtkstd::min(this->data[DATA_Y][ii], this->data[DATA_Y][jj]);
    this->lb[DATA_Z][middle] = 
      vtkstd::min(this->data[DATA_Z][ii], this->data[DATA_Z][jj]);

    this->ub[DATA_X][middle] = 
      vtkstd::max(this->data[DATA_X][ii], this->data[DATA_X][jj]);
    this->ub[DATA_Y][middle] = 
      vtkstd::max(this->data[DATA_Y][ii], this->data[DATA_Y][jj]);
    this->ub[DATA_Z][middle] = 
      vtkstd::max(this->data[DATA_Z][ii], this->data[DATA_Z][jj]);

    return;
    }

  // this case is needed when npart is a non-power-of-two
  if (len == 3)
    {
    ComputeLU(first+1, last);

    int ii = this->seq[first+0];

    this->lb[DATA_X][middle] = 
      vtkstd::min(data[DATA_X][ii], this->lb[DATA_X][middle2]);
    this->lb[DATA_Y][middle] = 
      vtkstd::min(data[DATA_Y][ii], this->lb[DATA_Y][middle2]);
    this->lb[DATA_Z][middle] = 
      vtkstd::min(data[DATA_Z][ii], this->lb[DATA_Z][middle2]);

    this->ub[DATA_X][middle] = 
      vtkstd::max(data[DATA_X][ii], this->ub[DATA_X][middle2]);
    this->ub[DATA_Y][middle] = 
      vtkstd::max(data[DATA_Y][ii], this->ub[DATA_Y][middle2]);
    this->ub[DATA_Z][middle] = 
      vtkstd::max(data[DATA_Z][ii], this->ub[DATA_Z][middle2]);

    return;
    }

  // non-base cases

  ComputeLU(first, middle);
  ComputeLU(middle,  last);

  // compute LU at the bottom-up pass
  this->lb[DATA_X][middle] = 
    vtkstd::min(this->lb[DATA_X][middle1], this->lb[DATA_X][middle2]);
  this->lb[DATA_Y][middle] = 
    vtkstd::min(this->lb[DATA_Y][middle1], this->lb[DATA_Y][middle2]);
  this->lb[DATA_Z][middle] = 
    vtkstd::min(this->lb[DATA_Z][middle1], this->lb[DATA_Z][middle2]);

  this->ub[DATA_X][middle] = 
    vtkstd::max(this->ub[DATA_X][middle1], this->ub[DATA_X][middle2]);
  this->ub[DATA_Y][middle] = 
    vtkstd::max(this->ub[DATA_Y][middle1], this->ub[DATA_Y][middle2]);
  this->ub[DATA_Z][middle] = 
    vtkstd::max(this->ub[DATA_Z][middle1], this->ub[DATA_Z][middle2]);

  return;
}


/***************************************************************************/
void vtkCosmoHaloFinder::MyFOF(int first, int last, int dataFlag)
{
  int len = last - first;

  // base case
  if (len == 1)
    {
    double progress = .5 * (double)last / (double)this->npart + .5;
    if (int(100 * progress) % 1 == 0)
      {
      this->UpdateProgress(progress);
      }

    return;
    }

  // non-base cases

  // divide
  int middle = first + len / 2;

  // continue FOF at the next level
  int tempFlag = (dataFlag + 1) % 3;
  this->MyFOF(first, middle, tempFlag);
  this->MyFOF(middle,  last, tempFlag);

  // recursive merge
  this->Merge(first, middle, middle, last, dataFlag);

  return;
}


/****************************************************************************/
void vtkCosmoHaloFinder::Merge(int first1, int last1, 
                               int first2, int last2, int dataFlag)
{
  int len1 = last1 - first1;
  int len2 = last2 - first2;

  // base cases
  // len1 == 1 || len2 == 1
  // len1 == 1,2 && len2 == 1,2 (2 for non-power-of-two case)
  if (len1 == 1 || len2 == 1) {
    for (int i=0; i<len1; i++)
    for (int j=0; j<len2; j++) {
      int ii = this->seq[first1+i];
      int jj = this->seq[first2+j];
  
      // fast exit
      if (this->ht[ii] == this->ht[jj])
        {
        continue;
        }

      // ht[ii] != ht[jj]
      float xdist = fabs(this->data[DATA_X][jj] - this->data[DATA_X][ii]);
      float ydist = fabs(this->data[DATA_Y][jj] - this->data[DATA_Y][ii]);
      float zdist = fabs(this->data[DATA_Z][jj] - this->data[DATA_Z][ii]);
  
      if (this->Periodic) 
        {
        xdist = vtkstd::min(xdist, this->np-xdist);
        ydist = vtkstd::min(ydist, this->np-ydist);
        zdist = vtkstd::min(zdist, this->np-zdist);
        }
  
      if ((xdist<this->bb) && (ydist<this->bb) && (zdist<this->bb)) {

        float dist = xdist*xdist + ydist*ydist + zdist*zdist;
        if (dist < this->bb*this->bb) {
  
          // union two halos to one
          int newHaloId = vtkstd::min(this->ht[ii], this->ht[jj]);
          int oldHaloId = vtkstd::max(this->ht[ii], this->ht[jj]);
  
          // update particles with oldHaloId
          int last = -1;
          int ith = this->halo[oldHaloId];
          while (ith != -1) {
            this->ht[ith] = newHaloId;
            last = ith;
            ith = this->nextp[ith];
          }
  
          // update halo's linked list
          this->nextp[last] = this->halo[newHaloId];
          this->halo[newHaloId] = this->halo[oldHaloId];
          this->halo[oldHaloId] = -1;
        }
      }
    } // (i,j)-loop

    return;
  }

  // non-base case

  // pruning?
  int middle1 = first1 + len1 / 2;
  int middle2 = first2 + len2 / 2;

  float lL = this->lb[dataFlag][middle1];
  float uL = this->ub[dataFlag][middle1];
  float lR = this->lb[dataFlag][middle2];
  float uR = this->ub[dataFlag][middle2];

  float dL = uL - lL;
  float dR = uR - lR;
  float dc = vtkstd::max(uL,uR) - vtkstd::min(lL,lR);

  float dist = dc - dL - dR;
  if (this->Periodic) 
    {
    dist = vtkstd::min(dist, np-dc);
    }

  if (dist >= this->bb)
    {
    return;
    }

  // continue merging at the next level
  dataFlag = (dataFlag + 1) % 3;

  this->Merge(first1, middle1,  first2, middle2, dataFlag);
  this->Merge(first1, middle1, middle2,   last2, dataFlag);
  this->Merge(middle1,  last1,  first2, middle2, dataFlag);
  this->Merge(middle1,  last1, middle2,   last2, dataFlag);

  return;
}

/****************************************************************************/

void vtkCosmoHaloFinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "bb: " << this->bb << endl;
  os << indent << "pmin: " << this->pmin << endl;
  os << indent << "rL: " << this->rL << endl;
  os << indent << "Periodic: " << (this->Periodic?"ON":"OFF") << endl;
}
