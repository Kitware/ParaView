/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCosmoHaloSorter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

  Program:   VTK/ParaView Los Alamos National Laboratory Modules (PVLANL)
  Module:    vtkCosmoHaloSorter.cxx

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
#include "vtkCosmoHaloSorter.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSortDataArray.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkCosmoHaloSorter, "1.1.4.1");
vtkStandardNewMacro(vtkCosmoHaloSorter);

/****************************************************************************/
vtkCosmoHaloSorter::vtkCosmoHaloSorter()
{
  //  Ith = 0;
  //  Jth = 0;
  this->Descending = false;
}

/****************************************************************************/
vtkCosmoHaloSorter::~vtkCosmoHaloSorter()
{
}

/****************************************************************************/
int vtkCosmoHaloSorter::RequestData(
                                    vtkInformation* vtkNotUsed(request),
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  //
  // Read in particles
  //
  int npart = input->GetNumberOfPoints();
  vtkDebugMacro(<< npart << " input particles\n");

  vtkIntArray* hID = vtkIntArray::SafeDownCast(input->GetPointData()->GetArray("hID"));
  if (hID == NULL)
    {
    vtkErrorMacro("The input data set doesn't have the hID field!");
    return 0;
    }

  vtkDataArray* dataArray = this->GetInputArrayToProcess(0, inputVector);
  if (dataArray == NULL)
    {
    vtkErrorMacro("No data array to sort!");
    return 0;
    }

  if (strcmp("haloSize", dataArray->GetName()) != 0)
    {
    vtkErrorMacro("Currently only sorting by haloSize is supported!");
    }

  vtkIntArray *haloSize = vtkIntArray::SafeDownCast(input->GetPointData()->GetArray("haloSize"));
  if (haloSize == NULL)
    {
    vtkErrorMacro("The input data set doesn't have the haloSize field!");
    return 0;
    }

  // compute halos statistics 
  double range[2];
  hID->GetRange(range);
  int maxht = (int)range[1];

  // compile halosize catalog 
  int *hsize = new int[maxht+1];
  memset(hsize, 0, sizeof(int)*(maxht+1));

  for (int i=0; i<npart; i++)
    {
    int id = hID->GetValue(i);
    if (id >= 0)
      {
      hsize[id] = haloSize->GetValue(i);
      }
    }

  // count number of halos
  int nhalo = 0;
  for (int h=0; h<=maxht; h++)
    {
    if (hsize[h] > 0)
      {
      nhalo++;
      }
    }

  vtkDebugMacro(<< nhalo << " halos\n");

  //
  // Sort hID based on hsize[]
  //

  // array compaction
  int *id = new int[nhalo];
  int *sz = new int[nhalo];

  for (int j=0,h=0; h<=maxht; h++)
    {
    if (hsize[h] > 0)
      {
      id[j] = h;
      sz[j] = hsize[h];
      j += 1;
      }
    }

  // sorting 
  vtkIntArray *key = vtkIntArray::New();
  key->SetNumberOfValues(nhalo);
  key->SetArray(sz, nhalo, 1);

  vtkIntArray *val = vtkIntArray::New();
  val->SetNumberOfValues(nhalo);
  val->SetArray(id, nhalo, 1);

  vtkSortDataArray::Sort(key, val);

  memset(hsize, 0, sizeof(int)*(maxht+1));

  // Compute the ranks of each halo. Halos with the same size will have a same rank.
  // When sorted in descending order, larger halos will have smaller ranks, otherwise
  // larger halos will have larger ranks.
  if (this->Descending)
    {
    int r = 1;
    hsize[id[nhalo-1]] = r;
    int count = 1;
    for (int i = nhalo - 2; i >= 0; i--)
      {
      if (sz[i] == sz[i+1])
        {
        hsize[id[i]] = r;
        count++;
        }
      else
        {
        r += count;
        hsize[id[i]] = r;
        count = 1;
        }
      }
    }
  else
    {
    int r = 1;
    hsize[id[0]] = r;
    int count = 1;
    for(int i = 1; i < nhalo; i++)
      {
      if (sz[i] == sz[i-1])
        {
        hsize[id[i]] = r;
        count++;
        }
      else
        {
        r += count;
        hsize[id[i]] = r;
        count = 1;
        }
      }
    }

  output->ShallowCopy(input);

  // Create the rank array for the output.
  vtkIntArray *rank = vtkIntArray::New();
  rank->SetName("rank");
  rank->SetNumberOfValues(npart);
  for (int i = 0; i < npart; i++)
    {
    int hid = hID->GetValue(i);
    if (hid >= 0)
      {
      rank->SetValue(i, hsize[hid]-1);
      }
    else
      {
      rank->SetValue(i, -1);
      }
    }

  output->GetPointData()->AddArray(rank);

  delete[] hsize;
  delete[] sz;
  delete[] id;

  return 1;
}

/****************************************************************************/
void vtkCosmoHaloSorter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Descending: " << (this->Descending?"ON":"OFF") << "\n";
}
