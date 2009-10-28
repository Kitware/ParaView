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

#include <vtksys/ios/fstream>
#include <vtkstd/string>
#include <vtkstd/algorithm>

#define numDataDims 3
#define dataX 0
#define dataY 1
#define dataZ 2

vtkCxxRevisionMacro(vtkCosmoHaloFinder, "1.3");
vtkStandardNewMacro(vtkCosmoHaloFinder);

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
class vtkCosmoHaloFinder::vtkInternal
{
public:
  vtkstd::string outputDir;
};

/****************************************************************************/
vtkCosmoHaloFinder::vtkCosmoHaloFinder() : Superclass()
{
  this->Internal = new vtkInternal;
  this->CurrentTimeIndex = 0;
  this->NumberOfTimeSteps = 0;
  this->BatchMode = false;
  this->Internal->outputDir = "./halo/";
}

/****************************************************************************/
vtkCosmoHaloFinder::~vtkCosmoHaloFinder()
{
  delete this->Internal;
}

void vtkCosmoHaloFinder::SetOutputDirectory(const char *dir)
{
  this->Internal->outputDir = vtkstd::string(dir);
}

//----------------------------------------------------------------------------
int vtkCosmoHaloFinder::RequestInformation(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector* outputVector)
{
  // if not in batch mode, time steps information is not needed
  if (!this->BatchMode)
    {
    return this->Superclass::RequestInformation(request, inputVector, outputVector);
    }

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  if ( inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) )
    {
    this->NumberOfTimeSteps = 
      inInfo->Length( vtkStreamingDemandDrivenPipeline::TIME_STEPS() );
    }
  else
    {
    this->NumberOfTimeSteps = 0;
    }
  // The output of this filter does not contain a specific time, rather 
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    }
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_RANGE()))
    {
    outInfo->Remove(vtkStreamingDemandDrivenPipeline::TIME_RANGE());
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCosmoHaloFinder::RequestUpdateExtent(vtkInformation* request,
                                            vtkInformationVector** inputVector,
                                            vtkInformationVector* outputVector)
{
  // get the requested update extent
  double *inTimes = inputVector[0]->GetInformationObject(0)
    ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->BatchMode)
    {
    double timeReq[1];
    timeReq[0] = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set
      ( vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEPS(), 
        timeReq, 1);
    return 1;
    }
  else
    {
    return this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);
    }

}

/****************************************************************************/
int vtkCosmoHaloFinder::RequestData(vtkInformation* request,
                                    vtkInformationVector** inputVector,
                                    vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "np:       " << np << "\n");
  vtkDebugMacro(<< "rL:       " << rL << "\n");
  vtkDebugMacro(<< "bb:       " << bb << "\n");
  vtkDebugMacro(<< "pmin:     " << pmin << "\n");
  vtkDebugMacro(<< "periodic: " << Periodic << "\n");

  if (this->BatchMode && this->NumberOfTimeSteps == 0)
    {
    vtkErrorMacro("No time steps in input data!");
    return 0;
    }

  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkUnstructuredGrid *output = vtkUnstructuredGrid::GetData(outputVector);

  // temporarily save the output dataset in this variable
  // if we operate on the output directly, when it's passed to the writer, it will cause
  // infinite recursive update of the pipeline
  vtkUnstructuredGrid *tmp_output = vtkUnstructuredGrid::New();

  if (this->BatchMode && !this->CurrentTimeIndex)
    {
    // check the existence of the output directory
    vtkDirectory *directory = vtkDirectory::New();
    if (!directory->Open(this->Internal->outputDir.c_str()))
      {
      if (!vtkDirectory::MakeDirectory(this->Internal->outputDir.c_str()))
        {
        vtkErrorMacro(<< "Invalid output directory: " << this->Internal->outputDir << "\n");
        return 0;
        }
      }

    directory->Delete();
    this->WritePVDFile(inputVector);
    // Tell the pipeline to start looping.
    request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }

  // copy over the existing fields from input data
  tmp_output->ShallowCopy(input);

  npart = input->GetNumberOfPoints();
  vtkDebugMacro(<< "npart = " << npart);

  vtkDataArray* tag = input->GetPointData()->GetArray("tag");
  if (tag == NULL)
    {
    vtkErrorMacro("The input data set doesn't have tag field!");
    return 0;
    }

  // normalize
  float xscal = rL / (1.0*np);

  // create workspace for halo finding.  
  ht = new int[npart];
  halo = new int[npart];
  nextp = new int[npart];
  pt = new int[npart];

  for (int i = 0; i < npart; i++)
    {
    ht[i] = i;
    halo[i] = i;
    nextp[i] = -1;
    pt[i] = (int)tag->GetComponent(i, 0);
    }

  // preprocess
  typedef float* floatptr;
  data = new floatptr[numDataDims];
  for (int i=0; i<numDataDims; i++)
    {
    data[i] = new float[npart];
    }


  for (int i=0; i<npart; i++)
    {
    double* point = input->GetPoint(i);
    float xx = (float) point[0];
    float yy = (float) point[1];
    float zz = (float) point[2];

    // FIXME: a quick fix
    if (xx == rL)
      {
      xx = 0.0;
      }
    if (yy == rL)
      {
      yy = 0.0;
      }
    if (zz == rL)
      {
      zz = 0.0;
      }

    data[dataX][i] = xx / xscal;
    data[dataY][i] = yy / xscal;
    data[dataZ][i] = zz / xscal;
    }

  v = new long long[npart];
  for (int i=0; i<npart; i++)
    {
    SetValue(v[i], data[dataX][i]);
    SetId(v[i], i);
    }

  lb = new floatptr[numDataDims];
  for (int i=0; i<numDataDims; i++)
    {
    lb[i] = new float[npart];
    }

  ub = new floatptr[numDataDims];
  for (int i=0; i<numDataDims; i++)
    {
    ub[i] = new float[npart];
    }

  // reorder
  Reorder(v, v+npart, dataX);

  // recording
  seq = new int[npart];
  for (int i=0; i<npart; i++)
    {
    seq[i] = GetId(v[i]);
    }

  // finding halos
  myFOF(0, npart, dataX);

  // compute halos statistics
  int *hsize = new int[npart];
  for (int h=0; h<npart; h++)
    {
    hsize[h] = 0;
    }

  for (int i=0; i<npart; i++)
    {
    hsize[ht[i]] += 1;
    }

  nhalo = 0;
  for (int h=0; h<npart; h++)
    {
    if (hsize[h] >= pmin)
      {
      nhalo++;
      }
    }

  vtkDebugMacro(<< nhalo << " halos\n");

  nhalopart = 0;
  for (int i=0; i<npart; i++)
    {
    if (hsize[ht[i]] >= pmin)
      {
      nhalopart++;
      }
    }

  vtkDebugMacro(<< nhalopart << " halo particles\n");

  vtkIntArray* hID = vtkIntArray::New();
  hID->SetName("hID");
  hID->SetNumberOfComponents(1);
  hID->SetNumberOfTuples(npart);

  vtkIntArray* haloSize = vtkIntArray::New();
  haloSize->SetName("haloSize");
  haloSize->SetNumberOfValues(npart);

  for (int i = 0; i < npart; i++)
    {
    hID->SetValue(i, ((hsize[ht[i]] < pmin) ? -1 : pt[ht[i]]));
    haloSize->SetValue(i, (hsize[ht[i]] < pmin) ? 0 : hsize[ht[i]]);
    }


  tmp_output->GetPointData()->AddArray(hID);
  tmp_output->GetPointData()->AddArray(haloSize);
  hID->Delete();
  haloSize->Delete();

  // clean up
  delete[] pt;
  delete[] ht;
  delete[] halo;
  delete[] nextp;

  for (int i = 0; i < numDataDims; i++)
    {
    delete[] data[i];
    delete[] lb[i];
    delete[] ub[i];
    }
  delete[] data;
  delete[] lb;
  delete[] ub;
  delete[] v;
  delete[] seq;
  delete[] hsize;

  // if in batch mode, write out the new dataset to file
  if (this->BatchMode) 
    {
    double *inTimes = inputVector[0]->GetInformationObject(0)
      ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    char buffer[64];
    sprintf(buffer, "/part_%08.4f.vtu", fabs(inTimes[this->CurrentTimeIndex]));
    //cout << "Current time step "<< inTimes[this->CurrentTimeIndex] << endl;
    vtkstd::string file_name = this->Internal->outputDir + buffer;
    //cout << "Writing file " << file_name << endl;

    vtkXMLUnstructuredGridWriter* writer = vtkXMLUnstructuredGridWriter::New();
    writer->SetInput(tmp_output);
    writer->SetDataModeToBinary();
    writer->SetFileName(file_name.c_str());
    writer->Write();
    writer->Delete();

    this->CurrentTimeIndex++;
    if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
      {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->CurrentTimeIndex = 0;
      }
    }

  // copy the data from temorary variable to the real output
  output->ShallowCopy(tmp_output);
  tmp_output->Delete();

  return 1;
}

/****************************************************************************************/
void vtkCosmoHaloFinder::Reorder(long long *first, long long *last, int dataFlag)
{
  long long *i;

  int len = last - first;

  // base case
  if (len == 1)
    {
    return;
    }

  // non-base cases
  // preprocessing
  for (i = first; i < last; i++)
    {
    SetValue(*i, data[dataFlag][GetId(*i)]);
    }

  // divide
  int half = len >> 1;
  vtkstd::nth_element(first, first+half, last, ValueIdPairLT());

  Reorder(first, first+half, (dataFlag+1)%3);
  Reorder(first+half,  last, (dataFlag+1)%3);

  // book-keeping
  int middle  = (int) (first + len/2 - v);
  int middle1 = (int) (first + len/4 - v);
  int middle2 = (int) (first + 3*len/4 - v);

  // base case
  if (len == 2)
    {
    int ii = GetId(*first);
    int jj = GetId(*(first+1));

    lb[dataX][middle] = vtkstd::min(data[dataX][ii], data[dataX][jj]);
    lb[dataY][middle] = vtkstd::min(data[dataY][ii], data[dataY][jj]);
    lb[dataZ][middle] = vtkstd::min(data[dataZ][ii], data[dataZ][jj]);

    ub[dataX][middle] = vtkstd::max(data[dataX][ii], data[dataX][jj]);
    ub[dataY][middle] = vtkstd::max(data[dataY][ii], data[dataY][jj]);
    ub[dataZ][middle] = vtkstd::max(data[dataZ][ii], data[dataZ][jj]);

    return;
    }

  // base case
  // this case is needed when len is a non-power-of-two
  if (len == 3)
    {
    int ii = GetId(*first);

    lb[dataX][middle] = vtkstd::min(data[dataX][ii], lb[dataX][middle2]);
    lb[dataY][middle] = vtkstd::min(data[dataY][ii], lb[dataY][middle2]);
    lb[dataZ][middle] = vtkstd::min(data[dataZ][ii], lb[dataZ][middle2]);

    ub[dataX][middle] = vtkstd::max(data[dataX][ii], ub[dataX][middle2]);
    ub[dataY][middle] = vtkstd::max(data[dataY][ii], ub[dataY][middle2]);
    ub[dataZ][middle] = vtkstd::max(data[dataZ][ii], ub[dataZ][middle2]);

    return;
    }

  // non-base case
  lb[dataX][middle] = vtkstd::min(lb[dataX][middle1], lb[dataX][middle2]);
  lb[dataY][middle] = vtkstd::min(lb[dataY][middle1], lb[dataY][middle2]);
  lb[dataZ][middle] = vtkstd::min(lb[dataZ][middle1], lb[dataZ][middle2]);

  ub[dataX][middle] = vtkstd::max(ub[dataX][middle1], ub[dataX][middle2]);
  ub[dataY][middle] = vtkstd::max(ub[dataY][middle1], ub[dataY][middle2]);
  ub[dataZ][middle] = vtkstd::max(ub[dataZ][middle1], ub[dataZ][middle2]);

  return;
}


/****************************************************************************************/
void vtkCosmoHaloFinder::myFOF(int first, int last, int dataFlag)
{

  //cout << "myFOF [" << first << "," << last << ")" << " " << dataFlag << endl;

  int len = last - first;

  // base case
  if (len == 1)
    {
    return;
    }

  // non-base cases

  // divide
  int half = len >> 1;

  // continue FOF at the next level
  myFOF(first, first+half, (dataFlag+1)%3);
  myFOF(first+half,  last, (dataFlag+1)%3);

  // recursive merge
  Merge(first, first+half, first+half, last, dataFlag);

  return;
}


/****************************************************************************/
void vtkCosmoHaloFinder::Merge(int first1, int last1, int first2, int last2, int dataFlag)
{
  // cout << "Merge [" << first1 << "," << last1 << ")[" << first2 << "," << last2 << ") " << dataFlag << endl;

  int len1 = last1 - first1;
  int len2 = last2 - first2;

  // three base cases
  // len1 == 1 && len2 == 1
  // len1 == 1 && len2 == 2
  // len1 == 2 && len2 == 1
  // the latter two are for n being a non-power of 2.

  // base cases
  if (len1 == 1 && len2 == 1)
    {
    basicMerge(seq[first1], seq[first2]);
    return;
    }

  if (len1 == 1 && len2 == 2) 
    {
    basicMerge(seq[first1], seq[first2]);
    basicMerge(seq[first1], seq[first2+1]);
    basicMerge(seq[first2], seq[first2+1]);
    return;
    }

  if (len1 == 2 && len2 == 1)
    {
    basicMerge(seq[first1], seq[first2]);
    basicMerge(seq[first1+1], seq[first2]);
    basicMerge(seq[first1], seq[first1+1]);
    return;
    }

  // non-base case

  // pruning?
  int middle1 = first1 + len1 / 2;
  int middle2 = first2 + len2 / 2;

  float lL = lb[dataFlag][middle1];
  float uL = ub[dataFlag][middle1];
  float lR = lb[dataFlag][middle2];
  float uR = ub[dataFlag][middle2];

  float dL = uL - lL;
  float dR = uR - lR;
  float dc = vtkstd::max(uL,uR) - vtkstd::min(lL,lR);

  float dist = dc - dL - dR;
  if (Periodic) 
    {
    dist = vtkstd::min(dist, np-dc);
    }

  if (dist >= bb)
    {
    return;
    }

  // continue merging at the next level
  Merge(first1, middle1,  first2, middle2, (dataFlag+1)%3);
  Merge(first1, middle1, middle2,   last2, (dataFlag+1)%3);
  Merge(middle1,  last1,  first2, middle2, (dataFlag+1)%3);
  Merge(middle1,  last1, middle2,   last2, (dataFlag+1)%3);

  return;
}

/****************************************************************************/
void vtkCosmoHaloFinder::basicMerge(int ii, int jj)
{

  // fast exit
  if (ht[ii] == ht[jj])
    {
    return;
    }

  // ht[ii] != ht[jj]
  float xdist = fabs(data[dataX][jj] - data[dataX][ii]);
  float ydist = fabs(data[dataY][jj] - data[dataY][ii]);
  float zdist = fabs(data[dataZ][jj] - data[dataZ][ii]);

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
      {

      // union two halos to one
      int newHaloId = (ht[ii] < ht[jj]) ? ht[ii] : ht[jj];
      int oldHaloId = (ht[ii] < ht[jj]) ? ht[jj] : ht[ii];

      // update particles with oldHaloId
      int last = -1;
      int ith = halo[oldHaloId];
      while (ith != -1)
        {
        ht[ith] = newHaloId;
        last = ith;
        ith = nextp[ith];
        }

      // update halo's linked list
      nextp[last] = halo[newHaloId];
      halo[newHaloId] = halo[oldHaloId];
      halo[oldHaloId] = -1;
      }
    }

  return;
}

/****************************************************************************/

void vtkCosmoHaloFinder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "np: " << this->np << endl;
  os << indent << "bb: " << this->bb << endl;
  os << indent << "pmin: " << this->pmin << endl;
  os << indent << "rL: " << this->rL << endl;
  os << indent << "Periodic: " << (this->Periodic?"ON":"OFF") << endl;
  os << indent << "BatchMode: " << (this->BatchMode?"ON":"OFF") << endl;
}

int vtkCosmoHaloFinder::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  return 1;
}

// write the pvd file of the time series data
void vtkCosmoHaloFinder::WritePVDFile(vtkInformationVector** inputVector)
{
  double *inTimes = inputVector[0]->GetInformationObject(0)
    ->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

  vtkstd::string file_name = this->Internal->outputDir + "/halo.pvd";
  ofstream ofile(file_name.c_str());
  if (!ofile)
    {
    vtkErrorMacro("Failed to open pvd file for writing!");
    return;
    }

  ofile <<"<?xml version=\"1.0\"?>\n"
        <<"<VTKFile type=\"Collection\" version=\"0.1\" byte_order=\"LittleEndian\">\n"
        <<"<Collection>\n";
  char buffer[64];

  for (int i = 0; i < this->NumberOfTimeSteps; i++)
    {
    sprintf(buffer, "part_%08.4f.vtu", fabs(inTimes[i])); 
    ofile << "<DataSet timestep=\"" << inTimes[i] << "\" file=\"" << buffer << "\"/>\n";
    }

  ofile << "</Collection>\n</VTKFile>";

  ofile.close();
}
