/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoCorrelater.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoCorrelater.cxx

Copyright (c) 2007, Los Alamos National Security, LLC

All rights reserved.

Copyright 2007. Los Alamos National Security, LLC. 
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

#include "vtkCosmoCorrelater.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMath.h"

#include <vtkstd/string>
#include <vtkstd/algorithm>

vtkCxxRevisionMacro(vtkCosmoCorrelater, "1.3.4.1");
vtkStandardNewMacro(vtkCosmoCorrelater);

/****************************************************************************/
#define numDataDims 3
#define dataX 0
#define dataY 1
#define dataZ 2

typedef float* floatptr;

struct ValueIdPair
{
  float value;
  int id;
};

static float GetValue(const long long &pairParam)
{
  ValueIdPair &pair = (ValueIdPair &)pairParam;
  return pair.value;
}

static int GetId(const long long &pairParam)
{
  ValueIdPair &pair = (ValueIdPair &)pairParam;
  return pair.id;
}

static void SetValue(long long &pairParam, float value)
{
  ValueIdPair &pair = (ValueIdPair &)pairParam;
  pair.value = value;
}

static void SetId(long long &pairParam, int id)
{
  ValueIdPair &pair = (ValueIdPair &)pairParam;
  pair.id = id;
}

//----------------------------------------------------------------------------
class ValueIdPairLT
{
public:
  bool operator() (const long long& p, const long long& q) const
  {
    return GetValue(p) < GetValue(q);
  }
};

//----------------------------------------------------------------------------
class vtkCosmoCorrelater::vtkInternal
{
public:
  vtkstd::string FieldName;
};

/****************************************************************************/
vtkCosmoCorrelater::vtkCosmoCorrelater()
{
  this->Internal = new vtkInternal;
  this->Internal->FieldName = "NDF";
  this->SetNumberOfInputPorts(2);
}

/****************************************************************************/
vtkCosmoCorrelater::~vtkCosmoCorrelater()
{
  delete this->Internal;
}

/****************************************************************************/
int vtkCosmoCorrelater::RequestData(
                                    vtkInformation* vtkNotUsed(request),
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkDataSet *input0 = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet *input1 = vtkDataSet::GetData(inputVector[1]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  // compute scaling factor
  float xscal = rL / (1.0*np);

  // read in all reference particles
  int npart0 = input0->GetNumberOfPoints();
  vtkDebugMacro(<< "npart0 = " << npart0);

  // create dataspace for particle location
  data = new floatptr[numDataDims];
  for (int i=0; i<numDataDims; i++)
    data[i] = new float[npart0];

  for (int i=0; i<npart0; i++)
    {
    double *point = input0->GetPoint(i);

    data[dataX][i] = (float) point[0] / xscal;
    data[dataY][i] = (float) point[1] / xscal;
    data[dataZ][i] = (float) point[2] / xscal;
    }

  // build the 3d-tree
  v = new long long[npart0];
  for (int i=0; i<npart0; i++)
    {
    SetValue(v[i], data[dataX][i]);
    SetId(v[i], i);
    }

  mediantest = new float[npart0];

  Reorder(v, v+npart0, dataX);

  seq = new int[npart0];
  for (int i=0; i<npart0; i++)
    seq[i] = GetId(v[i]);

  delete[] v;

  // iterate over all query particles
  int npart1 = input1->GetNumberOfPoints();
  vtkDebugMacro(<< "npart1 = " << npart1);

  // the new field to store the NDF values
  vtkIntArray *ndf = vtkIntArray::New();
  ndf->SetName(this->Internal->FieldName.c_str());
  ndf->SetNumberOfValues(npart1);

  // compute initial bounding box
  float *boxI = new float[2*numDataDims];
  for (int i=0; i<2*numDataDims; i+=2)
    {
    boxI[i+0] = 0.0;
    boxI[i+1] = (float) np;
    }

  // at each iteration, perform range search
  range = new float[numDataDims];

  for (int i=0; i<npart1; i++)
    {
    double *point = input1->GetPoint(i);

    // get query particle location
    range[dataX] = (float) point[0] / xscal;
    range[dataY] = (float) point[1] / xscal;
    range[dataZ] = (float) point[2] / xscal;

    // reset counter
    nmap = 0;

    // perform range search
    RangeSearch(0, npart0, dataX, boxI);

    ndf->SetValue(i, nmap);
    } // i-loop

  output->ShallowCopy(input1);
  output->GetPointData()->AddArray(ndf);
  ndf->Delete();

  // cleanup
  delete[] seq;
  delete[] boxI;
  delete[] mediantest;
  delete[] range;
  for (int i = 0; i < numDataDims; i++)
    {
    delete[] data[i];
    }
  delete[] data;

  return 1;
}

/****************************************************************************/
void vtkCosmoCorrelater::Reorder(long long *first, long long *last, int dataFlag)
{
  long long *i;

  int len = last - first;

  // base case
  if (len == 1)
    {
    return;
    }

  // non-base cases
  for (i = first; i < last; i++)
    {
    SetValue(*i, data[dataFlag][GetId(*i)]);
    }

  // divide
  long long *middle = first + len / 2;
  vtkstd::nth_element(first, middle, last, ValueIdPairLT());

  // book-keeping
  mediantest[(int)(middle-v)] = GetValue(*middle);

  Reorder(first, middle, (dataFlag+1)%3);
  Reorder(middle,  last, (dataFlag+1)%3);

  // done
  return;
}

/****************************************************************************/
void vtkCosmoCorrelater::RangeSearch(int first, int last, int dataFlag, float *box)
{

  int len = last - first;

  // base case
  if (len == 1)
    {
    // check whether to count or not
    float xx0 = data[dataX][seq[first]];
    float yy0 = data[dataY][seq[first]];
    float zz0 = data[dataZ][seq[first]];

    float xdist = fabs(xx0 - range[dataX]);
    float ydist = fabs(yy0 - range[dataY]);
    float zdist = fabs(zz0 - range[dataZ]);

    if (Periodic)
      {
      xdist = vtkstd::min(xdist, np-xdist);
      ydist = vtkstd::min(ydist, np-ydist);
      zdist = vtkstd::min(zdist, np-zdist);
      }

    if ((xdist<bb) && (ydist<bb) && (zdist<bb))
      {

      float dist = xdist*xdist + ydist*ydist + zdist*zdist;
      if (dist < bb*bb) 
        nmap += 1;
      } 

    return;
    }

  // non-base cases

  // divide
  int middle = first + len / 2 ;
  float median = mediantest[middle];

  // recursively search down kd-tree
  // prune if possible
  // left subtree
  // prune?
  float *boxL = new float[2*numDataDims];
  for (int i=0; i<2*numDataDims; i++)
    {
    boxL[i] = box[i];
    }

  boxL[2*dataFlag+1] = median; 

  float dL = boxL[2*dataFlag+1] - boxL[2*dataFlag];
  float dc = vtkstd::max(boxL[2*dataFlag+1], range[dataFlag]) - 
    vtkstd::min(boxL[2*dataFlag+0], range[dataFlag]);

  float dist = dc - dL;
  if (Periodic)
    {
    dist = vtkstd::min(dist, np-dc);
    }

  if (dist <= bb)
    {
    RangeSearch(first, middle, (dataFlag+1)%3, boxL);
    }

  delete[] boxL;

  // right subtree
  // prune?
  float *boxR = new float[2*numDataDims];
  for (int i=0; i<2*numDataDims; i++)
    {
    boxR[i] = box[i];
    }

  boxR[2*dataFlag+0] = median;

  dL = boxR[2*dataFlag+1] - boxR[2*dataFlag];
  dc = vtkstd::max(boxR[2*dataFlag+1], range[dataFlag]) - 
    vtkstd::min(boxR[2*dataFlag+0], range[dataFlag]);

  dist = dc - dL;
  if (Periodic)
    {
    dist = vtkstd::min(dist, np-dc);
    }

  if (dist <= bb)
    {
    RangeSearch(middle,  last, (dataFlag+1)%3, boxR);
    }

  delete[] boxR;

  // done
  return;
}

/****************************************************************************/
void vtkCosmoCorrelater::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "np: " << this->np << endl;
  os << indent << "bb: " << this->bb << endl;
  os << indent << "rL: " << this->rL << endl;
}

void vtkCosmoCorrelater::SetFieldName(const char* field_name)
{
  this->Internal->FieldName = vtkstd::string(field_name);
}

void vtkCosmoCorrelater::SetQueryConnection(vtkAlgorithmOutput *algOutput)
{
  this->SetInputConnection(1, algOutput);
}
